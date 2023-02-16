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

	class IDisplay : public IMemory_Change_Notifier {
		public:
			virtual ~IDisplay() = default;

			// nothing for now
	};

	/*
	 * Default 300x200 monochromatic display
	 */
	class CDisplay_300x200 : public IPeripheral, public IDisplay, public std::enable_shared_from_this<CDisplay_300x200> {

		private:
			// video memory mapping
			std::array<uint8_t, Video_Memory_End - Video_Memory_Start> mVideo_Memory;
			// was there a write access to video memory?
			bool mVideo_Mem_Changed = false;

		public:
			CDisplay_300x200() noexcept;

			// clears video memory
			void Clear_Video_Memory();

			// IPeripheral iface
			virtual void Attach(IBus& bus, std::shared_ptr<IInterrupt_Controller> interruptCtl) override;
			virtual void Detach(IBus& bus, std::shared_ptr<IInterrupt_Controller> interruptCtl) override;
			virtual void Read_Memory(uint32_t address, void* target, uint32_t size) const override;
			virtual void Write_Memory(uint32_t address, const void* source, uint32_t size) override;

			// IMemory_Change_Notifier iface
			virtual bool Is_Memory_Changed() const override;
			virtual void Clear_Memory_Changed_Flag() override;
	};

}
