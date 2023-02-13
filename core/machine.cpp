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
			throw abort_exception{ address };
		}

		std::copy_n(mMain_Memory.begin() + address, size, reinterpret_cast<uint8_t*>(target));
	}

	void CMemory_Bus::Write(uint32_t address, const void* source, uint32_t size) {

		// TODO: this should be modular

		// video memory connected to bus
		if (address >= Video_Memory_Start && address + size < Video_Memory_End) {
			std::copy_n(reinterpret_cast<const uint8_t*>(source), size, mVideo_Memory.begin() + (address - Video_Memory_Start));
			mVideo_Mem_Changed = true;
			return;
		}

		if (address + size > mMain_Memory.size()) {
			throw abort_exception{ address };
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

	bool CMemory_Bus::Is_Video_Memory_Changed() const {
		return mVideo_Mem_Changed;
	}

	void CMemory_Bus::Clear_Video_Memory_Changed_Flag() {
		mVideo_Mem_Changed = false;
	}

	/***********************************************************************************
	 * Interrupt controller
	 ***********************************************************************************/

	CInterrupt_Controller::CInterrupt_Controller() {
		//
	}

	void CInterrupt_Controller::Signalize_IRQ() {
		mIRQ_Pending = true;
	}

	bool CInterrupt_Controller::Has_Pending_IRQ() const {
		return mIRQ_Pending;
	}

	void CInterrupt_Controller::Clear_IRQ_Flag() {
		mIRQ_Pending = false;
	}

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

		mInterrupt_Ctl.Clear_IRQ_Flag();

		// cold reset erases memory (or at least generates a garbagge or zeroes)
		if (!warm) {
			mMem_Bus.Clear_Main_Memory();
		}

		// always clear video memory
		mMem_Bus.Clear_Video_Memory();
	}

	void CMachine::Step(size_t numberOfSteps, bool handleIRQs) {

		for (size_t i = 0; i < numberOfSteps; i++) {

			try {

				// has pending IRQ? signalize
				if (handleIRQs && mInterrupt_Ctl.Has_Pending_IRQ()) {
					throw irq_exception();
				}

				uint32_t encoded;

				if ((mContext.Reg(NRegister::PC) & 0b11) != 0) {
					throw unaligned_exception();
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
					// NOTE: instruction execute may throw an exception - one of those listed below
					if (!instr->Execute(mContext)) {
						std::cerr << "Could not execute instruction: " << instr->Generate_String() << std::endl;
					}
				}
				else {
					throw undefined_instruction_exception();
				}

			}
			catch (reset_exception& /*ex*/) {
				mContext.Reg(NRegister::RA) = mContext.Reg(NRegister::PC);

				// load interrupt vector from memory
				uint32_t addr = 0;
				mMem_Bus.Read(Get_IVT_Vector_Address(NIVT_Entry::Reset), &addr, sizeof(uint32_t));
				mContext.Reg(NRegister::PC) = addr;
			}
			catch (undefined_instruction_exception& /*ex*/) {
				mContext.Reg(NRegister::RA) = mContext.Reg(NRegister::PC);

				// load interrupt vector from memory
				uint32_t addr = 0;
				mMem_Bus.Read(Get_IVT_Vector_Address(NIVT_Entry::Undefined), &addr, sizeof(uint32_t));
				mContext.Reg(NRegister::PC) = addr;
			}
			catch (abort_exception& /*ex*/) {
				mContext.Reg(NRegister::RA) = mContext.Reg(NRegister::PC);

				// load interrupt vector from memory
				uint32_t addr = 0;
				mMem_Bus.Read(Get_IVT_Vector_Address(NIVT_Entry::Abort), &addr, sizeof(uint32_t));
				mContext.Reg(NRegister::PC) = addr;
			}
			catch (unaligned_exception& /*ex*/) {
				mContext.Reg(NRegister::RA) = mContext.Reg(NRegister::PC);

				// load interrupt vector from memory
				uint32_t addr = 0;
				mMem_Bus.Read(Get_IVT_Vector_Address(NIVT_Entry::Unaligned), &addr, sizeof(uint32_t));
				mContext.Reg(NRegister::PC) = addr;
			}
			catch (irq_exception& /*ex*/) {
				mContext.Reg(NRegister::RA) = mContext.Reg(NRegister::PC);

				// load interrupt vector from memory
				uint32_t addr = 0;
				mMem_Bus.Read(Get_IVT_Vector_Address(NIVT_Entry::IRQ), &addr, sizeof(uint32_t));
				mContext.Reg(NRegister::PC) = addr;
			}
			catch (supervisor_call_exception& /*ex*/) {
				mContext.Reg(NRegister::RA) = mContext.Reg(NRegister::PC);

				// load interrupt vector from memory
				uint32_t addr = 0;
				mMem_Bus.Read(Get_IVT_Vector_Address(NIVT_Entry::Supervisor_Call), &addr, sizeof(uint32_t));
				mContext.Reg(NRegister::PC) = addr;
			}
		}

	}

}
