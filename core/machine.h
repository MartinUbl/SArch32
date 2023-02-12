#pragma once

#include "isa.h"
#include <fstream>

namespace sarch32 {

	// reset vector - the initial setting of PC register after start
	constexpr uint32_t Reset_Vector = 0x00001000;

	// default display width and height - the display is monochromatic
	constexpr size_t Display_Width = 300;
	constexpr size_t Display_Height = 200;

	// start of video memory
	constexpr uint32_t Video_Memory_Start = 0xA0000000;
	// end of video memory; in fact, it ends at A0001D4C (start + 300*200/8), but round it up
	constexpr uint32_t Video_Memory_End = 0xA0001E00;

	// default size of main memory
	constexpr uint32_t Default_Memory_Size = 2 * 1024 * 1024; // 2 MiB

	/*
	 * Used memory bus
	 * 
	 * This model includes main memory, video memory
	 */
	class CMemory_Bus : public IBus
	{
		private:
			// main memory vector
			std::vector<uint8_t> mMain_Memory;

			// video memory mapping
			std::array<uint8_t, Video_Memory_End - Video_Memory_Start> mVideo_Memory;

		public:
			CMemory_Bus(const uint32_t memSize);

			// loads bytes given as argument to given address
			bool Load_Bytes_To(const std::vector<uint8_t>& bytes, uint32_t address);
			// clears main memory
			void Clear_Main_Memory();
			// clears video memory
			void Clear_Video_Memory();

			// IBus iface
			virtual void Read(uint32_t address, void* target, uint32_t size) const override;
			virtual void Write(uint32_t address, const void* source, uint32_t size) override;
	};

	// exception thrown by CPU when unaligned access is detected
	class unaligned_exception : public std::runtime_error {
		public:
			using std::runtime_error::runtime_error;
	};

	/*
	 * Default reference SArch32 machine
	 */
	class CMachine {
		private:
			// memory bus instance
			CMemory_Bus mMem_Bus;
			// CPU context instance
			CCPU_Context mContext;

		public:
			CMachine(uint32_t memory_size = Default_Memory_Size);
			virtual ~CMachine() = default;

			// initializes memory from object file
			bool Init_Memory_From_File(const std::string& sobjFile);
			// resets the CPU
			void Reset(bool warm = true);

			// steps the CPU by given number of steps
			void Step(size_t numberOfSteps = 1);

			// retrieves CPU context (read only)
			const CCPU_Context& Get_CPU_Context() const {
				return mContext;
			}

			// retrieves memory bus (read only)
			const CMemory_Bus& Get_Memory_Bus() const {
				return mMem_Bus;
			}
	};

}
