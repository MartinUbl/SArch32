#include "machine.h"
#include "sobjfile.h"

#include <algorithm>
#include <iterator>
#include <random>
#include <iostream>

namespace sarch32 {

	/***********************************************************************************
	 * Memory bus
	 ***********************************************************************************/

	CMemory_Bus::CMemory_Bus(const uint32_t memSize) : mMain_Memory(memSize) {
		//
	}

	void CMemory_Bus::Read(uint32_t address, void* target, uint32_t size) const {

		// TODO: this should be modular

		// video memory connected to bus
		if (address >= Video_Memory_Start && address + size <= Video_Memory_End) {
			std::copy_n(mVideo_Memory.begin() + (address - Video_Memory_Start), size, reinterpret_cast<uint8_t*>(target));
			return;
		}

		if (address + size > mMain_Memory.size()) {
			// TODO: raise abort
			return;
		}

		std::copy_n(mMain_Memory.begin() + address, size, reinterpret_cast<uint8_t*>(target));
	}

	void CMemory_Bus::Write(uint32_t address, const void* source, uint32_t size) {

		// TODO: this should be modular

		// video memory connected to bus
		if (address >= Video_Memory_Start && address + size < Video_Memory_End) {
			std::copy_n(reinterpret_cast<const uint8_t*>(source), size, mVideo_Memory.begin() + (address - Video_Memory_Start));
			return;
		}

		if (address + size > mMain_Memory.size()) {
			// TODO: raise abort
			return;
		}

		std::copy_n(reinterpret_cast<const uint8_t*>(source), size, mMain_Memory.begin() + address);
	}

	bool CMemory_Bus::Load_Bytes_To(const std::vector<uint8_t>& bytes, uint32_t address) {

		if (address + static_cast<uint32_t>(bytes.size()) > mMain_Memory.size()) {
			return false;
		}

		std::copy(bytes.begin(), bytes.end(), mMain_Memory.begin() + address);
		return true;
	}

	void CMemory_Bus::Clear_Main_Memory() {

		// fill with zeroes - this is here for easier debugging
		std::fill(mMain_Memory.begin(), mMain_Memory.end(), 0);

		// fill with random data - more likely to be the real scenario, disabled during debugging phase
		/*
		std::random_device r;
		std::default_random_engine randomEngine(r());
		std::uniform_int_distribution<int> uniformDist(std::numeric_limits<uint8_t>::min(), std::numeric_limits<uint8_t>::max());

		std::generate(mMain_Memory.begin(), mMain_Memory.end(), [&uniformDist, &randomEngine]() {
			return (uint8_t)uniformDist(randomEngine);
		});
		*/
	}

	void CMemory_Bus::Clear_Video_Memory() {
		// black screen
		std::fill(mVideo_Memory.begin(), mVideo_Memory.end(), 0);
	}

	/***********************************************************************************
	 * Exceptions
	 ***********************************************************************************/

	/***********************************************************************************
	 * Machine
	 ***********************************************************************************/

	CMachine::CMachine(uint32_t memory_size) : mMem_Bus(memory_size), mContext(mMem_Bus) {
		//
	}

	bool CMachine::Init_Memory_From_File(const std::string& sobjFile) {

		// load object file
		SObj::CSObj_File infile;
		if (!infile.Load_From_File(sobjFile)) {
			return false;
		}

		// retrieve sections
		auto sections = infile.Get_Sections();

		for (auto& s : sections) {
			// map all sections to their respective addresses
			if (!mMem_Bus.Load_Bytes_To(s.second.data, s.second.startAddr)) {
				return false;
			}
		}

		return true;
	}

	void CMachine::Reset(bool warm) {

		mContext.Reg(NRegister::PC) = Reset_Vector;		// reset PC to a reset vector
		mContext.Reg(NRegister::FLG) = 0;				// reset flags

		// cold reset erases memory (or at least generates a garbagge or zeroes)
		if (!warm) {
			mMem_Bus.Clear_Main_Memory();
		}

		// always clear video memory
		mMem_Bus.Clear_Video_Memory();
	}

	void CMachine::Step(size_t numberOfSteps) {

		for (size_t i = 0; i < numberOfSteps; i++) {

			uint32_t encoded;

			if ((mContext.Reg(NRegister::PC) & 0b11) != 0) {
				throw unaligned_exception("The program counter is not aligned to 4 bytes");
			}

			// 1) fetch
			try {
				encoded = mContext.Mem_Read_Scalar<uint32_t>(mContext.Reg(NRegister::PC));
				mContext.Reg(NRegister::PC) += 4;
			}
			catch (std::exception& /*ex*/) {
				// TODO: handle fetch aborts - jump to IVT[...] and handle
				return;
			}

			// 2) decode
			auto instr = CInstruction::Build_From_Binary(encoded);
			if (instr) {
				//std::cout << "EXEC: " << instr->Generate_String() << std::endl;

				// 3) execute + writeback
				if (!instr->Execute(mContext)) {
					std::cerr << "Could not execute instruction: " << instr->Generate_String() << std::endl;
				}
			}
			else {
				// TODO: raise unknown instruction interrupt
			}
		}

	}

}
