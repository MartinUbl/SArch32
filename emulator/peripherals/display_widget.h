#pragma once

#include <vector>
#include <memory>

#include <QtWidgets/QWidget>
#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>

#include "../../core/isa.h"
#include "../../core/peripherals/display.h"

/*
 * Display widget (300x200 "display" of the machine)
 */
class CDisplay_Widget : public QWidget
{
	private:
		// copy of a display buffer
		std::vector<uint8_t> mDisplay_Buffer;

	public:
		explicit CDisplay_Widget(QWidget* parent = nullptr) : QWidget(parent) {
			//
		}

		// triggers repainting, considering the memory obtained through the bus
		void Trigger_Repaint(std::shared_ptr<sarch32::CDisplay_300x200>& display, sarch32::CMemory_Bus& bus);

		void paintEvent(QPaintEvent* event) override;
};
