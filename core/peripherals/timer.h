#pragma once

#include "../machine.h"

namespace sarch32 {

	// system timer channels
	constexpr size_t Timer_Channel_Count = 4;
	// IRQ number of system timer
	constexpr size_t Timer_IRQ_Number = 3;

#pragma pack(push, 1)
	struct TSystem_Timer_Control {
		uint8_t enable : 4;				// 1 bit per channel
		uint8_t multiplier : 8;			// 2 bits per channel, divisors: 00 = 1x, 01 = 4x, 10 = 16x, 11 = 64x
		uint8_t irq_on_compare : 4;		// 1 bit per channel, raise interrupt on compare value reach
		uint8_t irq_on_overflow : 4;	// 1 bit per channel, raise interrupt on timer overflow
		uint8_t reset_on_compare : 4;	// 1 bit per channel, reset to 0 when compare value reached
	};

	struct TSystem_Timer_Status {
		uint8_t event_compare : 4;		// detected compare reach on channel
		uint8_t event_overflow : 4;		// detectet timer value overflow on channel
		//
	};
#pragma pack(pop)

	/*
	 * System timer register enumerator
	 */
	enum class NSystem_Timer_Regs {
		Control,		// R/W, see TSystem_Timer_Control
		Status,			// R/W, see TSystem_Timer_Status
		Counter_0,		// R
		Counter_1,		// R
		Counter_2,		// R
		Counter_3,		// R
		Compare_0,		// R/W
		Compare_1,		// R/W
		Compare_2,		// R/W
		Compare_3,		// R/W

		count
	};

	// convenience constant for system timer register count
	constexpr size_t System_Timer_Regs_Count = static_cast<size_t>(NSystem_Timer_Regs::count);

	// start of timer memory
	constexpr uint32_t Timer_Memory_Start = 0x90000080;
	// end of timer memory
	constexpr uint32_t Timer_Memory_End = Timer_Memory_Start + System_Timer_Regs_Count * 4;

	class ITimer {
		public:
			virtual void Register_Tick_Callback() = 0;
	};

	/*
	 * Default system timer
	 */
	class CSystem_Timer : public IPeripheral, public std::enable_shared_from_this<CSystem_Timer> {

		private:
			// timer memory mapping
			std::array<uint32_t, (Timer_Memory_End - Timer_Memory_Start) / 4> mTimer_Memory;

			// weak reference to interrupt controller
			std::weak_ptr<IInterrupt_Controller> mInterrupt_Ctl;

			// reference to control register (helper)
			TSystem_Timer_Control* mControl_Reg;
			// reference to status register (helper)
			TSystem_Timer_Status* mStatus_Reg;

		public:
			CSystem_Timer();

			// IPeripheral iface
			virtual void Attach(IBus& bus, std::shared_ptr<IInterrupt_Controller> interruptCtl) override;
			virtual void Detach(IBus& bus, std::shared_ptr<IInterrupt_Controller> interruptCtl) override;
			virtual void Clock_Cycles_Passed(uint32_t count) override;
			virtual void Read_Memory(uint32_t address, void* target, uint32_t size) const override;
			virtual void Write_Memory(uint32_t address, const void* source, uint32_t size) override;
	};

}
