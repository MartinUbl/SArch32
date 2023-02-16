#include "gpio.h"
#include "../isa.h"

namespace sarch32 {

	CGPIO_Controller::CGPIO_Controller() : mGPIO_Memory{} {
		//
	}

	void CGPIO_Controller::Clear_GPIO_Memory() {
		// black screen
		std::fill(mGPIO_Memory.begin(), mGPIO_Memory.end(), 0);
	}

	bool CGPIO_Controller::Is_GPIO_Memory_Changed() const {
		return mGPIO_Mem_Changed;
	}

	void CGPIO_Controller::Clear_GPIO_Memory_Changed_Flag() {
		mGPIO_Mem_Changed = false;
	}

	void CGPIO_Controller::Attach(IBus& bus, std::shared_ptr<IInterrupt_Controller> interruptCtl) {
		bus.Map_Peripheral(shared_from_this(), GPIO_Memory_Start, GPIO_Memory_End - GPIO_Memory_Start);

		mInterrupt_Ctl = interruptCtl;
	}

	void CGPIO_Controller::Detach(IBus& bus, std::shared_ptr<IInterrupt_Controller> interruptCtl) {
		bus.Unmap_Peripheral(shared_from_this(), GPIO_Memory_Start, GPIO_Memory_End - GPIO_Memory_Start);
	}

	void CGPIO_Controller::Read_Memory(uint32_t address, void* target, uint32_t size) const {

		if (size != 4) {
			return;
		}

		// GPIO memory connected to bus
		if (address >= GPIO_Memory_Start && address + size <= GPIO_Memory_End) {

			const uint32_t offset = (address - GPIO_Memory_Start);
			const size_t registerIdx = static_cast<size_t>(offset / 4);

			// attempt to read write-only registers
			if (registerIdx >= static_cast<size_t>(NGPIO_Registers::Set_0) && registerIdx <= static_cast<size_t>(NGPIO_Registers::Clear_1)) {
				return;
			}

			// level registers are special - the do not have physical memory assigned, but rather represents a set of pin states
			if (registerIdx >= static_cast<size_t>(NGPIO_Registers::Level_0) && registerIdx <= static_cast<size_t>(NGPIO_Registers::Level_1)) {
				const uint32_t pinBank = static_cast<uint32_t>((registerIdx - static_cast<size_t>(NGPIO_Registers::Level_0)) * 32);

				uint32_t val = 0;
				for (uint32_t i = 0; i < 32; i++) {
					val <<= 1;
					val |= (mGPIO_States[pinBank + i] ? 0b1 : 0b0);
				}

				*reinterpret_cast<uint32_t*>(target) = val;
			}
			else {
				*reinterpret_cast<uint32_t*>(target) = mGPIO_Memory[registerIdx];
			}
		}

	}

	void CGPIO_Controller::Write_Memory(uint32_t address, const void* source, uint32_t size) {

		if (size != 4) {
			return;
		}

		// GPIO memory connected to bus
		if (address >= GPIO_Memory_Start && address + size < GPIO_Memory_End) {
			const uint32_t offset = (address - GPIO_Memory_Start);
			const size_t registerIdx = static_cast<size_t>(offset / 4);

			const uint32_t setvalue = *reinterpret_cast<const uint32_t*>(source);

			// attempt to write to read-only registers
			if (registerIdx >= static_cast<size_t>(NGPIO_Registers::Level_0) && registerIdx <= static_cast<size_t>(NGPIO_Registers::Level_1)) {
				return;
			}

			bool isClear = true;

			// set and clear registers are special - they are "routed" to pin state
			switch (static_cast<NGPIO_Registers>(registerIdx)) {
				case NGPIO_Registers::Set_0:
					isClear = false;
					[[fallthrough]];
				case NGPIO_Registers::Clear_0:
				{
					for (uint32_t i = 0; i < 32; i++) {
						if (setvalue & (1ULL << i)) {
							Set_State(i, isClear);
						}
					}
					break;
				}
				case NGPIO_Registers::Set_1:
					isClear = false;
					[[fallthrough]];
				case NGPIO_Registers::Clear_1:
				{
					for (uint32_t i = 0; i < 32; i++) {
						if (setvalue & (1ULL << i)) {
							Set_State(32 + i, isClear);
						}
					}
					break;
				}
				default:
					mGPIO_Memory[registerIdx] = setvalue;
					break;
			}

			mGPIO_Mem_Changed = true;
		}

	}

	void CGPIO_Controller::Set_State(uint32_t pin, bool state) {

		// output pin - just set state
		if (Get_Pin_Mode(pin) == NGPIO_Mode::Output) {
			mGPIO_States[pin] = state;
		}
		else if (Get_Pin_Mode(pin) == NGPIO_Mode::Input) {

			bool change = (mGPIO_States[pin] != state);

			mGPIO_States[pin] = state;

			if (change) {
				if (state && Get_Reg_State(NGPIO_Registers::_Rising, pin)) {
					Set_Reg_State(NGPIO_Registers::_Detect, pin, true);

					if (auto intctl = mInterrupt_Ctl.lock()) {
						intctl->Signalize_IRQ(GPIO_IRQ_Number);
					}
				}
				else if (!state && Get_Reg_State(NGPIO_Registers::_Falling, pin)) {
					Set_Reg_State(NGPIO_Registers::_Detect, pin, true);

					if (auto intctl = mInterrupt_Ctl.lock()) {
						intctl->Signalize_IRQ(GPIO_IRQ_Number);
					}
				}
			}
		}

	}

	bool CGPIO_Controller::Get_State(uint32_t pin) const {

		if (Get_Pin_Mode(pin) == NGPIO_Mode::Input) {
			return mGPIO_States[pin];
		}

		return false;
	}

	NGPIO_Mode_Generic CGPIO_Controller::Get_Mode(uint32_t pin) const {
		auto mode = Get_Pin_Mode(pin);

		if (mode == NGPIO_Mode::Input)
			return NGPIO_Mode_Generic::Input;
		else if (mode == NGPIO_Mode::Output)
			return NGPIO_Mode_Generic::Output;

		return NGPIO_Mode_Generic::Alt;
	}

	uint32_t CGPIO_Controller::Get_Reg_Idx_For(NGPIO_Registers reg, uint32_t pinNo) const {

		switch (reg) {
			case NGPIO_Registers::_Mode:
				return static_cast<uint32_t>(NGPIO_Registers::Mode_0) + ((pinNo * 2) / 32);
			case NGPIO_Registers::_Level:
				return static_cast<uint32_t>(NGPIO_Registers::Level_0) + (pinNo / 32);
			case NGPIO_Registers::_Set:
				return static_cast<uint32_t>(NGPIO_Registers::Set_0) + (pinNo / 32);
			case NGPIO_Registers::_Clear:
				return static_cast<uint32_t>(NGPIO_Registers::Clear_0) + (pinNo / 32);
			case NGPIO_Registers::_Detect:
				return static_cast<uint32_t>(NGPIO_Registers::Detect_0) + (pinNo / 32);
			case NGPIO_Registers::_Rising:
				return static_cast<uint32_t>(NGPIO_Registers::Rising_0) + (pinNo / 32);
			case NGPIO_Registers::_Falling:
				return static_cast<uint32_t>(NGPIO_Registers::Falling_0) + (pinNo / 32);

			default:
				return 0;
		}

	}

	bool CGPIO_Controller::Get_Reg_State(NGPIO_Registers reg, uint32_t pinNo) const {

		const uint32_t actRegIdx = Get_Reg_Idx_For(reg, pinNo);
		const uint32_t actOffset = (pinNo % 32);

		return (mGPIO_Memory[actRegIdx] >> actOffset) & 0b1;
	}

	void CGPIO_Controller::Set_Reg_State(NGPIO_Registers reg, uint32_t pinNo, bool state) {

		const uint32_t actRegIdx = Get_Reg_Idx_For(reg, pinNo);
		const uint32_t actOffset = (pinNo % 32);

		if (state) {
			mGPIO_Memory[actRegIdx] |= (1ULL << actOffset);
		}
		else {
			mGPIO_Memory[actRegIdx] &= ~(1ULL << actOffset);
		}
	}

	NGPIO_Mode CGPIO_Controller::Get_Pin_Mode(uint32_t pinNo) const {
		const uint32_t actRegIdx = Get_Reg_Idx_For(NGPIO_Registers::_Mode, pinNo);

		return static_cast<NGPIO_Mode>( (mGPIO_Memory[actRegIdx] >> (((pinNo*2) % 32))) & 0b11 );
	}

	void CGPIO_Controller::Set_Pin_Mode(uint32_t pinNo, NGPIO_Mode mode) {
		const uint32_t actRegIdx = Get_Reg_Idx_For(NGPIO_Registers::_Mode, pinNo);

		mGPIO_Memory[actRegIdx] &= ~(0b11ULL << ((pinNo * 2) % 32) );
		mGPIO_Memory[actRegIdx] |= ~(static_cast<uint32_t>(mode) << ((pinNo * 2) % 32));
	}

}
