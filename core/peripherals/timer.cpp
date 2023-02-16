#include "timer.h"

namespace sarch32 {

	CSystem_Timer::CSystem_Timer() : mTimer_Memory{},
		mControl_Reg(reinterpret_cast<TSystem_Timer_Control*>(&mTimer_Memory[static_cast<size_t>(NSystem_Timer_Regs::Control)])),
		mStatus_Reg(reinterpret_cast<TSystem_Timer_Status*>(&mTimer_Memory[static_cast<size_t>(NSystem_Timer_Regs::Status)])) {
		//
	}

	void CSystem_Timer::Attach(IBus& bus, std::shared_ptr<IInterrupt_Controller> interruptCtl) {
		bus.Map_Peripheral(shared_from_this(), Timer_Memory_Start, Timer_Memory_End - Timer_Memory_Start);

		mInterrupt_Ctl = interruptCtl;
	}

	void CSystem_Timer::Detach(IBus& bus, std::shared_ptr<IInterrupt_Controller> /*interruptCtl*/) {
		bus.Unmap_Peripheral(shared_from_this(), Timer_Memory_Start, Timer_Memory_End - Timer_Memory_Start);
	}

	void CSystem_Timer::Clock_Cycles_Passed(uint32_t count) {

		for (size_t i = 0; i < Timer_Channel_Count; i++) {
			if ((mControl_Reg->enable >> i) & 0x1) {

				uint32_t multiplier = ((mControl_Reg->multiplier >> (2 * i)) & 0b11) * 4;
				if (multiplier == 0)
					multiplier = 1;

				uint32_t ctrIncrement = count * multiplier;

				// should we trigger compare assertion?
				const bool trigger_compare = (mTimer_Memory[static_cast<size_t>(NSystem_Timer_Regs::Counter_0) + i] < mTimer_Memory[static_cast<size_t>(NSystem_Timer_Regs::Compare_0) + i]) &&
					(mTimer_Memory[static_cast<size_t>(NSystem_Timer_Regs::Counter_0) + i] + ctrIncrement >= mTimer_Memory[static_cast<size_t>(NSystem_Timer_Regs::Compare_0) + i]);
				// should we trigger overflow assertion?
				const bool trigger_overflow = (mTimer_Memory[static_cast<size_t>(NSystem_Timer_Regs::Counter_0) + i] + ctrIncrement) < mTimer_Memory[static_cast<size_t>(NSystem_Timer_Regs::Counter_0) + i];

				// increment the counter
				mTimer_Memory[static_cast<size_t>(NSystem_Timer_Regs::Counter_0) + i] += ctrIncrement;

				// counter just reached or exceeded compare
				if (trigger_compare) {
					if (!((mStatus_Reg->event_compare >> i) & 0x1)) {

						mStatus_Reg->event_compare |= 1ULL << i;

						// should we raise an interrupt on compare assertion?
						if ((mControl_Reg->irq_on_compare >> i) & 0x1) {
							if (auto ctl = mInterrupt_Ctl.lock()) {
								ctl->Signalize_IRQ(Timer_IRQ_Number);
							}
						}

						// should we reset the timer on compare assertion?
						if ((mControl_Reg->reset_on_compare >> i) & 0x1) {
							mTimer_Memory[static_cast<size_t>(NSystem_Timer_Regs::Counter_0) + i] = 0;
						}
					}
				}

				// counter just recorded an overflow
				if (trigger_overflow) {
					if (!((mStatus_Reg->event_overflow >> i) & 0x1)) {

						mStatus_Reg->event_overflow |= 1ULL << i;

						if ((mControl_Reg->irq_on_overflow >> i) & 0x1) {
							if (auto ctl = mInterrupt_Ctl.lock()) {
								ctl->Signalize_IRQ(Timer_IRQ_Number);
							}
						}
					}
				}
			}
		}

	}

	void CSystem_Timer::Read_Memory(uint32_t address, void* target, uint32_t size) const {

		if (size != 4 || !target) {
			return;
		}

		// system timer memory connected to bus
		if (address >= Timer_Memory_Start && address + size <= Timer_Memory_End) {

			const uint32_t offset = (address - Timer_Memory_Start);
			const size_t registerIdx = static_cast<size_t>(offset / 4);

			*static_cast<uint32_t*>(target) = mTimer_Memory[registerIdx];
		}

	}

	void CSystem_Timer::Write_Memory(uint32_t address, const void* source, uint32_t size) {

		if (size != 4 || !source) {
			return;
		}

		// system timer memory connected to bus
		if (address >= Timer_Memory_Start && address + size < Timer_Memory_End) {
			const uint32_t offset = (address - Timer_Memory_Start);
			const size_t registerIdx = static_cast<size_t>(offset / 4);

			// attempt to write to a read-only register
			if (registerIdx >= static_cast<size_t>(NSystem_Timer_Regs::Counter_0) && registerIdx <= static_cast<size_t>(NSystem_Timer_Regs::Counter_3)) {
				return;
			}

			mTimer_Memory[registerIdx] = *static_cast<const uint32_t*>(source);
		}

	}

}
