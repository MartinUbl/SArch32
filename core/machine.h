#pragma once

#include "isa.h"
#include <fstream>

namespace sarch32 {

	// reset vector - the initial setting of PC register after start
	constexpr uint32_t Reset_Vector = 0x00001000;

	// default size of main memory
	constexpr uint32_t Default_Memory_Size = 2 * 1024 * 1024; // 2 MiB

	// default assumed CPI for every instruction - this is just a mean that is used to approximate the CPI for simulated clock source
	constexpr uint32_t Default_Mean_CPI = 8; // qualified guess

	template<typename T>
	concept Child_Of_IPeripheral = std::derived_from<T, IPeripheral>;

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

			// structure for peripheral memory mapping
			struct TPeripheral_Mapping {
				std::shared_ptr<IPeripheral> peripheral;
				uint32_t addressStart;
				uint32_t length;
			};

			// a vector of peripheral memory mapping
			std::vector<TPeripheral_Mapping> mPeripheral_Memory;

		public:
			CMemory_Bus(const uint32_t memSize);

			// loads bytes given as argument to given address
			bool Load_Bytes_To(const std::vector<uint8_t>& bytes, uint32_t address);
			// clears main memory
			void Clear_Main_Memory();

			// IBus iface
			virtual void Read(uint32_t address, void* target, uint32_t size) const override;
			virtual void Write(uint32_t address, const void* source, uint32_t size) override;
			virtual bool Map_Peripheral(std::shared_ptr<IPeripheral> peripheral, uint32_t address, uint32_t length) override;
			virtual bool Unmap_Peripheral(std::shared_ptr<IPeripheral> peripheral, uint32_t address, uint32_t length) override;
	};

	/*
	 * Used interrupt controller
	 * 
	 * This model includes just a single flag for holding IRQ indication
	 * Future models may include IRQ queuing, precedence, channels, etc.
	 */
	class CInterrupt_Controller : public IInterrupt_Controller
	{
		private:
			bool mIRQ_Pending = false;

		public:
			CInterrupt_Controller();

			// IInterrupt_Controller iface
			virtual void Signalize_IRQ(int16_t channel) override;
			virtual bool Has_Pending_IRQ(int16_t channel) const override;
			virtual void Clear_IRQ_Flag(int16_t channel) override;
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
			// interrupt controller
			std::shared_ptr<CInterrupt_Controller> mInterrupt_Ctl;

			std::list<std::shared_ptr<IPeripheral>> mPeripherals;

		public:
			CMachine(uint32_t memory_size = Default_Memory_Size);
			virtual ~CMachine() = default;

			// initializes memory from object file
			bool Init_Memory_From_File(const std::string& sobjFile);
			// resets the CPU
			void Reset(bool warm = true);

			// steps the CPU by given number of steps
			void Step(size_t numberOfSteps = 1, bool handleIRQs = false);

			// retrieves CPU context (read only)
			const CCPU_Context& Get_CPU_Context() const {
				return mContext;
			}

			// retrieves memory bus (read only)
			CMemory_Bus& Get_Memory_Bus() {
				return mMem_Bus;
			}

			// retrieves an interrupt controller
			std::shared_ptr<CInterrupt_Controller>& Get_Interrupt_Controller() {
				return mInterrupt_Ctl;
			}

			template<Child_Of_IPeripheral T, typename... Args>
			std::shared_ptr<T> Attach_Peripheral(Args... args) {
				std::shared_ptr<T> peripheral = std::make_shared<T>(args...);

				peripheral->Attach(mMem_Bus, mInterrupt_Ctl);

				mPeripherals.push_back(peripheral);

				return peripheral;
			}
	};

}
