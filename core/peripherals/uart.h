#pragma once

#include "../machine.h"

#include <queue>
#include <mutex>

namespace sarch32 {

	// IRQ number of system timer
	constexpr size_t MiniUART_IRQ_Number = 4;
	// how many cycles are needed to transmit a character (intentionally higher, will be reduced after debugging phase)
	constexpr uint32_t MiniUART_Cycles_Per_Character = 64;

	// MiniUART TX/RX FIFO size in characters
	constexpr size_t MiniUART_FIFO_Size = 16;

#pragma pack(push, 1)
	struct TMiniUART_Control {
		uint8_t enable : 1;							// is the MiniUART enabled?
		uint8_t rx_enable : 1;						// is receiver enabled?
		uint8_t tx_enable : 1;						// is transmitter enabled?
		uint8_t rx_fifo_data_ready_irq_enable : 1;	// should we trigger the IRQ on data ready?
		uint8_t tx_fifo_empty_irq_enable : 1;		// should we trigged the IRQ on TX FIFO empty?
	};

	struct TMiniUART_Status {
		uint8_t rx_data_ready : 1;					// RX FIFO contains at least 1 character
		uint8_t tx_fifo_empty : 1;					// TX FIFO is empty
		uint8_t rx_fifo_overrun : 1;				// RX FIFO would exceed the capacity
		uint8_t tx_fifo_overrun : 1;				// TX FIFO would exceed the capacity
		uint8_t tx_fifo_full : 1;					// TX FIFO is full (sending another character would cause overrun)
	};
#pragma pack(pop)

	/*
	 * System timer register enumerator
	 */
	enum class NMiniUART_Regs {
		Control,		// R/W, see TMiniUART_Control
		Status,			// R/W, see TMiniUART_Status
		Data,			// R/W, in fact, this does not connect to any existing register, but rather to FIFO (RX for R, TX for W) pop routine
		Baud_Rate,		// R/W

		count
	};

	// convenience constant for MiniUART register count
	constexpr size_t MiniUART_Regs_Count = static_cast<size_t>(NMiniUART_Regs::count);

	// start of MiniUART memory
	constexpr uint32_t MiniUART_Memory_Start = 0x900000C0;
	// end of MiniUART memory
	constexpr uint32_t MiniUART_Memory_End = MiniUART_Memory_Start + MiniUART_Regs_Count * 4;

	/*
	 * Interface for UART controller
	 */
	class IUART_Controller {
		public:
			virtual ~IUART_Controller() = default;

			// puts char to the UART FIFO - emulates the character transmission by regular means
			virtual void Put_Char(char c) = 0;
			// retrieves char from the UART FIFO - emulates the character receipt by regular means
			virtual char Get_Char(bool& success) = 0;
	};

	/*
	 * Default MiniUART controller
	 */
	class CMiniUART : public IPeripheral, public IUART_Controller, public std::enable_shared_from_this<CMiniUART> {

		private:
			// MiniUART memory mapping
			std::array<uint32_t, (MiniUART_Memory_End - MiniUART_Memory_Start) / 4> mMiniUART_Memory;

			// weak reference to interrupt controller
			std::weak_ptr<IInterrupt_Controller> mInterrupt_Ctl;

			// reference to control register (helper)
			TMiniUART_Control* mControl_Reg;
			// reference to status register (helper)
			TMiniUART_Status* mStatus_Reg;

			// TX FIFO (device to outer world)
			std::queue<char> mTx_FIFO;
			// RX FIFO (outer world to device); mutable to conform to const-ness of Read_Memory contract, but allow modifications
			mutable std::queue<char> mRx_FIFO;

			// characters sent from UART ready to be retrieved by the outer world - this is there to properly emulate the speed of transmission
			std::queue<char> mSent_Characters;
			// characters received from UART ready to be retrieved by the device - this is there to properly emulate the speed of transmission
			std::queue<char> mReceived_Characters;

			// cycles counter to emulate TX and RX FIFO synchronization
			uint32_t mCycles_Counter = 0;

			// mutex to synchronize queues
			mutable std::mutex mFIFO_Mtx;

		public:
			CMiniUART();

			// IPeripheral iface
			virtual void Attach(IBus& bus, std::shared_ptr<IInterrupt_Controller> interruptCtl) override;
			virtual void Detach(IBus& bus, std::shared_ptr<IInterrupt_Controller> interruptCtl) override;
			virtual void Clock_Cycles_Passed(uint32_t count) override;
			virtual void Read_Memory(uint32_t address, void* target, uint32_t size) const override;
			virtual void Write_Memory(uint32_t address, const void* source, uint32_t size) override;

			// IUART_Controller iface
			virtual void Put_Char(char c) override;
			virtual char Get_Char(bool& success) override;
	};

}
