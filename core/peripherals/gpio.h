#pragma once

#include "../machine.h"

#include <bitset>

namespace sarch32 {

	// count of general purpose I/O pins
	constexpr size_t GPIO_Count = 64;
	// IRQ number of GPIO controller
	constexpr size_t GPIO_IRQ_Number = 2;

	/*
	 * Interface enumerator - the GUI usually needs to know, whether the pin is in input, output or alternate mode
	 */
	enum class NGPIO_Mode_Generic {
		Input,
		Output,
		Alt,
	};

	/*
	 * Enumerator of controller-specific pin modes
	 */
	enum class NGPIO_Mode {
		Input  = 0b00,
		Output = 0b01,
		Alt_0  = 0b10,
		Alt_1  = 0b11,
	};

	/*
	 * Enumerator of controller-specific pin registers
	 */
	enum class NGPIO_Registers {
		// mode registers - 2 bits per GPIO => 128 mode flags
		Mode_0  = 0, // R/W
		Mode_1  = 1, // R/W
		Mode_2  = 2, // R/W
		Mode_3  = 3, // R/W
		// level registers (input) - 1 bit per GPIO => 64 value bits
		Level_0 = 4, // R
		Level_1 = 5, // R
		// set and clear registers (output) - 1 bit per GPIO => 64 value bits
		Set_0   = 6, // W
		Set_1   = 7, // W
		Clear_0 = 8, // W
		Clear_1 = 9, // W
		// event detect status (input) - 1 bit per GPIO => 64 value bits
		Detect_0 = 10, // R/W
		Detect_1 = 11, // R/W
		// event detect setting (input), detect rising or falling edge - 1 bit per GPIO => 64 value bits
		Rising_0 = 12,  // R/W
		Rising_1 = 13,  // R/W
		Falling_0 = 14, // R/W
		Falling_1 = 15, // R/W

		count,

		// meta registers - used to address register sets in generic context
		_Mode,
		_Level,
		_Set,
		_Clear,
		_Detect,
		_Rising,
		_Falling,
	};

	// convenience constant for GPIO registers count
	constexpr size_t GPIO_Register_Count = static_cast<size_t>(NGPIO_Registers::count);

	// start of GPIO mapped memory
	constexpr uint32_t GPIO_Memory_Start = 0x90000000;
	// end of GPIO mapped memory
	constexpr uint32_t GPIO_Memory_End = GPIO_Memory_Start + 4 * GPIO_Register_Count; // count * size of register (4 bytes)

	/*
	 * GPIO controller interface
	 */
	class IGPIO_Controller {
		public:
			virtual void Set_State(uint32_t pin, bool state) = 0;
			virtual bool Get_State(uint32_t pin) const = 0;
			virtual NGPIO_Mode_Generic Get_Mode(uint32_t pin) const = 0;

			virtual uint32_t Get_Pin_Count() const = 0;
	};

	/*
	 * Default GPIO controller
	 */
	class CGPIO_Controller : public IPeripheral, public IGPIO_Controller, public std::enable_shared_from_this<CGPIO_Controller> {

		private:
			// video memory mapping
			std::array<uint32_t, (GPIO_Memory_End - GPIO_Memory_Start) / 4> mGPIO_Memory;
			// was there a write access to video memory?
			bool mGPIO_Mem_Changed = false;

			// GPIO states
			std::bitset<GPIO_Count> mGPIO_States;

			// weak reference to interrupt controller
			std::weak_ptr<IInterrupt_Controller> mInterrupt_Ctl;

		protected:
			// retrieves register index for given generic register and pin
			uint32_t Get_Reg_Idx_For(NGPIO_Registers reg, uint32_t pinNo) const;

			// retrieves register pin state for pin number
			bool Get_Reg_State(NGPIO_Registers reg, uint32_t pinNo) const;
			// sets register pin state for given pin number
			void Set_Reg_State(NGPIO_Registers reg, uint32_t pinNo, bool state);
			// retrieves pin mode
			NGPIO_Mode Get_Pin_Mode(uint32_t pinNo) const;
			// sets pin mode for given pin number
			void Set_Pin_Mode(uint32_t pinNo, NGPIO_Mode mode);

		public:
			CGPIO_Controller();

			// clears video memory
			void Clear_GPIO_Memory();

			// was there a change to GPIO state?
			bool Is_GPIO_Memory_Changed() const;
			// clear GPIO memory change flag
			void Clear_GPIO_Memory_Changed_Flag();

			// IPeripheral iface
			virtual void Attach(IBus& bus, std::shared_ptr<IInterrupt_Controller> interruptCtl) override;
			virtual void Detach(IBus& bus, std::shared_ptr<IInterrupt_Controller> interruptCtl) override;
			virtual void Read_Memory(uint32_t address, void* target, uint32_t size) const override;
			virtual void Write_Memory(uint32_t address, const void* source, uint32_t size) override;

			// IGPIO_Controller iface
			virtual void Set_State(uint32_t pin, bool state) override;
			virtual bool Get_State(uint32_t pin) const override;
			virtual NGPIO_Mode_Generic Get_Mode(uint32_t pin) const override;
			virtual uint32_t Get_Pin_Count() const override {
				return GPIO_Count;
			}
	};

}
