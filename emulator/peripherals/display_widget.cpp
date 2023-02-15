#include "display_widget.h"

void CDisplay_Widget::paintEvent(QPaintEvent* event)
{
	// prepare painter
	QPainter painter;
	painter.begin(this);

	// fill background - default color is black
	QRect r(0, 0, width(), height());
	painter.fillRect(r, Qt::black);

	// prepare pen (white)
	setAttribute(Qt::WA_OpaquePaintEvent);
	QPen linepen(Qt::red);
	linepen.setWidth(1);
	linepen.setColor(Qt::white);
	painter.setRenderHint(QPainter::Antialiasing, false);
	painter.setPen(linepen);

	const size_t maxOffset = static_cast<size_t>(width() * height());

	// go through the video memory
	for (size_t i = 0; i < mDisplay_Buffer.size() && i < maxOffset; i++) {

		uint8_t b = mDisplay_Buffer[i];

		// 1 byte = 8 pixels (set = white, clear = black)
		for (int j = 0; j < 8; j++) {
			if (!(b & (1 << j))) {
				continue;
			}

			int row = (static_cast<int>(i) * 8 + j) / width();
			int col = (static_cast<int>(i) * 8 + j) % width();

			painter.drawPoint(col, row);
		}
	}

	QWidget::paintEvent(event);
	painter.end();
}

void CDisplay_Widget::Trigger_Repaint(std::shared_ptr<sarch32::CDisplay_300x200>& display, sarch32::CMemory_Bus& bus) {

	// TODO: solve this better, this is obviously ineffective, but serves as a starting point

	if (!display->Is_Video_Memory_Changed()) {
		return;
	}

	display->Clear_Video_Memory_Changed_Flag();

	mDisplay_Buffer.resize(sarch32::Video_Memory_End - sarch32::Video_Memory_Start);
	bus.Read(sarch32::Video_Memory_Start, &mDisplay_Buffer[0], sarch32::Video_Memory_End - sarch32::Video_Memory_Start);

	emit repaint();
}
