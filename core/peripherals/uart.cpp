#include "uart.h"

namespace sarch32 {

	CMiniUART::CMiniUART() : mMiniUART_Memory{},
		mControl_Reg(reinterpret_cast<TMiniUART_Control*>(&mMiniUART_Memory[static_cast<size_t>(NMiniUART_Regs::Control)])),
		mStatus_Reg(reinterpret_cast<TMiniUART_Status*>(&mMiniUART_Memory[static_cast<size_t>(NMiniUART_Regs::Status)])) {
		//
	}

	void CMiniUART::Attach(IBus& bus, std::shared_ptr<IInterrupt_Controller> interruptCtl) {
		bus.Map_Peripheral(shared_from_this(), MiniUART_Memory_Start, MiniUART_Memory_End - MiniUART_Memory_Start);

		mInterrupt_Ctl = interruptCtl;
	}

	void CMiniUART::Detach(IBus& bus, std::shared_ptr<IInterrupt_Controller> /*interruptCtl*/) {
		bus.Unmap_Peripheral(shared_from_this(), MiniUART_Memory_Start, MiniUART_Memory_End - MiniUART_Memory_Start);
	}

	void CMiniUART::Clock_Cycles_Passed(uint32_t count) {

		if (!mControl_Reg->enable) {
			return;
		}

		std::unique_lock<std::mutex> lck(mFIFO_Mtx);

		// increase cycle count, emulate character sending through the emulated peripheral after given cycle count
		mCycles_Counter += count;
		while (mCycles_Counter >= MiniUART_Cycles_Per_Character) {

			// reduce simulation counter
			mCycles_Counter -= MiniUART_Cycles_Per_Character;

			// if there is something to transmit...
			if (!mTx_FIFO.empty()) {
				// pop the character
				const char c = mTx_FIFO.front();
				mTx_FIFO.pop();

				// always clear the FIFO overrun and FIFO full flags
				mStatus_Reg->tx_fifo_overrun = 0;
				mStatus_Reg->tx_fifo_full = 0;

				// this just emulates the outer world receiver
				mSent_Characters.push(c);

				// if the TX FIFO is empty now, we may signalize IRQ
				if (mTx_FIFO.empty()) {
					mStatus_Reg->tx_fifo_empty = 1;

					if (mControl_Reg->tx_fifo_empty_irq_enable) {
						if (auto ctl = mInterrupt_Ctl.lock()) {
							ctl->Signalize_IRQ(MiniUART_IRQ_Number);
						}
					}
				}
			}

			// if the outer world sends us some characters...
			if (!mReceived_Characters.empty()) {
				// receive it
				const char c = mReceived_Characters.front();
				mReceived_Characters.pop();

				const bool wasEmpty = mRx_FIFO.empty();

				// check the boundaries - if the FIFO is full, signalize overrun
				if (mRx_FIFO.size() >= MiniUART_FIFO_Size) {
					mStatus_Reg->rx_fifo_overrun = 1;
				}
				else { // otherwise push the character to FIFO and signalize IRQ if applicable
					mRx_FIFO.push(c);

					mStatus_Reg->rx_data_ready = 1;

					if (wasEmpty) {
						if (mControl_Reg->rx_fifo_data_ready_irq_enable) {
							if (auto ctl = mInterrupt_Ctl.lock()) {
								ctl->Signalize_IRQ(MiniUART_IRQ_Number);
							}
						}
					}
				}
			}

		}

	}

	void CMiniUART::Read_Memory(uint32_t address, void* target, uint32_t size) const {

		if (size != 4 || !target) {
			return;
		}

		// MiniUART memory connected to bus
		if (address >= MiniUART_Memory_Start && address + size <= MiniUART_Memory_End) {

			const uint32_t offset = (address - MiniUART_Memory_Start);
			const size_t registerIdx = static_cast<size_t>(offset / 4);

			// data register - points to RX FIFO for "Read" operation
			if (registerIdx == static_cast<size_t>(NMiniUART_Regs::Data)) {
				std::unique_lock<std::mutex> lck(mFIFO_Mtx);

				// there is something in RX FIFO - pop it, change status flags
				if (!mRx_FIFO.empty()) {
					*static_cast<uint32_t*>(target) = mRx_FIFO.front();
					mRx_FIFO.pop();

					if (mRx_FIFO.empty()) {
						mStatus_Reg->rx_data_ready = 0;
					}

					mStatus_Reg->rx_fifo_overrun = 0;
				}

				return;
			}

			*static_cast<uint32_t*>(target) = mMiniUART_Memory[registerIdx];
		}

	}

	void CMiniUART::Write_Memory(uint32_t address, const void* source, uint32_t size) {

		if (size != 4 || !source) {
			return;
		}

		// MiniUART memory connected to bus
		if (address >= MiniUART_Memory_Start && address + size < MiniUART_Memory_End) {
			const uint32_t offset = (address - MiniUART_Memory_Start);
			const size_t registerIdx = static_cast<size_t>(offset / 4);

			// data register - points to TX FIFO for "Write" operation
			if (registerIdx == static_cast<size_t>(NMiniUART_Regs::Data)) {
				std::unique_lock<std::mutex> lck(mFIFO_Mtx);

				// the TX FIFO is full - we don't have enough space to store the character - signalize overrun
				if (mTx_FIFO.size() >= MiniUART_FIFO_Size) {
					mStatus_Reg->tx_fifo_overrun = 1;
				}
				else { // otherwise push the character to TX FIFO and check for boundaries
					mTx_FIFO.push(*static_cast<const uint32_t*>(source));

					if (mTx_FIFO.size() >= MiniUART_FIFO_Size) {
						mStatus_Reg->tx_fifo_full = 1;
					}
				}

				return;
			}

			mMiniUART_Memory[registerIdx] = *static_cast<const uint32_t*>(source);
		}

	}

	void CMiniUART::Put_Char(char c) {

		// UART must be enabled for any data transmission
		if (!mControl_Reg->enable || !mControl_Reg->rx_enable) {
			return;
		}

		std::unique_lock<std::mutex> lck(mFIFO_Mtx);

		mReceived_Characters.push(c);
	}

	char CMiniUART::Get_Char(bool& success) {

		// UART must be enabled for any data transmission
		if (!mControl_Reg->enable || !mControl_Reg->tx_enable) {
			success = false;
			return '\0';
		}

		std::unique_lock<std::mutex> lck(mFIFO_Mtx);

		// no characters available
		if (mSent_Characters.empty()) {
			success = false;
			return '\0';
		}

		// pop character and return it
		const char c = mSent_Characters.front();
		mSent_Characters.pop();

		success = true;
		return c;
	}

}
