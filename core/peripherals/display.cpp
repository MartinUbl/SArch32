#include "display.h"
#include "../isa.h"

namespace sarch32 {

	CDisplay_300x200::CDisplay_300x200() noexcept : mVideo_Memory{} {
		//
	}

	void CDisplay_300x200::Clear_Video_Memory() {
		// black screen
		std::fill(mVideo_Memory.begin(), mVideo_Memory.end(), 0);
	}

	bool CDisplay_300x200::Is_Video_Memory_Changed() const {
		return mVideo_Mem_Changed;
	}

	void CDisplay_300x200::Clear_Video_Memory_Changed_Flag() {
		mVideo_Mem_Changed = false;
	}

	void CDisplay_300x200::Attach(IBus& bus, std::shared_ptr<IInterrupt_Controller> /*interruptCtl*/) {
		bus.Map_Peripheral(shared_from_this(), Video_Memory_Start, Video_Memory_End - Video_Memory_Start);
	}

	void CDisplay_300x200::Detach(IBus& bus, std::shared_ptr<IInterrupt_Controller> /*interruptCtl*/) {
		bus.Unmap_Peripheral(shared_from_this(), Video_Memory_Start, Video_Memory_End - Video_Memory_Start);
	}

	void CDisplay_300x200::Read_Memory(uint32_t address, void* target, uint32_t size) const {

		// video memory connected to bus
		if (address >= Video_Memory_Start && address + size <= Video_Memory_End) {
			std::copy_n(mVideo_Memory.begin() + (address - Video_Memory_Start), size, static_cast<uint8_t*>(target));
		}

	}

	void CDisplay_300x200::Write_Memory(uint32_t address, const void* source, uint32_t size) {

		// video memory connected to bus
		if (address >= Video_Memory_Start && address + size < Video_Memory_End) {
			std::copy_n(static_cast<const uint8_t*>(source), size, mVideo_Memory.begin() + (address - Video_Memory_Start));
			mVideo_Mem_Changed = true;
		}

	}

}