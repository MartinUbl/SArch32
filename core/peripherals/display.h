#pragma once

#include "../machine.h"

namespace sarch32 {

	// default display width and height - the display is monochromatic
	constexpr size_t Display_Width = 300;
	constexpr size_t Display_Height = 200;

	// start of video memory
	constexpr uint32_t Video_Memory_Start = 0xA0000000;
	// end of video memory; in fact, it ends at A0001D4C (start + 300*200/8), but round it up
	constexpr uint32_t Video_Memory_End = 0xA0001E00;

	/*
	 * Default 300x200 monochromatic display
	 */
	class CDisplay_300x200 : public IPeripheral, public std::enable_shared_from_this<CDisplay_300x200> {

		private:
			// video memory mapping
			std::array<uint8_t, Video_Memory_End - Video_Memory_Start> mVideo_Memory;
			// was there a write access to video memory?
			bool mVideo_Mem_Changed = false;

		public:
			CDisplay_300x200();

			// clears video memory
			void Clear_Video_Memory();

			// was there a change to video memory?
			bool Is_Video_Memory_Changed() const;
			// clear video memory change flag
			void Clear_Video_Memory_Changed_Flag();

			// IPeripheral iface
			virtual void Attach(IBus& bus, IInterrupt_Controller& interruptCtl) override;
			virtual void Detach(IBus& bus, IInterrupt_Controller& interruptCtl) override;
			virtual void Read_Memory(uint32_t address, void* target, uint32_t size) const override;
			virtual void Write_Memory(uint32_t address, const void* source, uint32_t size) override;
	};

}
