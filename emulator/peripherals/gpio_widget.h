#pragma once

#include <vector>
#include <memory>

#include <QtWidgets/QWidget>
#include <QtWidgets/QPushButton>
#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>

#include "../../core/isa.h"
#include "../../core/peripherals/gpio.h"

class CGPIO_Widget;

/*
 * GPIO pin widget, controllable
 */
class CGPIO_Pin_Button_Widget : public QWidget
{
	private:
		// pin number
		uint32_t mPin_No;
		// pin mode
		sarch32::NGPIO_Mode_Generic mPin_Mode = sarch32::NGPIO_Mode_Generic::Input;
		// current pin state
		bool mPin_State = false;

		// parent widget for controlling
		CGPIO_Widget* mParent;

	protected:
		void mouseReleaseEvent(QMouseEvent* event) override;

	public:
		explicit CGPIO_Pin_Button_Widget(uint32_t pinNo, CGPIO_Widget* parent = nullptr);

		// sets up the GUI
		void Setup_GUI();
		// sets the internal cached pin state
		void Set_State(sarch32::NGPIO_Mode_Generic mode, bool level);
		// triggers repainting
		void Trigger_Repaint();

		void paintEvent(QPaintEvent* event) override;
};

/*
 * GPIO widget (64 GPIO pins)
 */
class CGPIO_Widget : public QWidget
{
	private:
		// reference to GPIO controller
		std::weak_ptr<sarch32::IGPIO_Controller> mGPIO_Ctl;
		// vector of GPIO pin widgets
		std::vector<CGPIO_Pin_Button_Widget*> mGPIO_Btns;

	public:
		explicit CGPIO_Widget(std::shared_ptr<sarch32::IGPIO_Controller> gpioCtl, QWidget* parent = nullptr) : QWidget(parent), mGPIO_Ctl(gpioCtl) {
			//
		}

		// updates pin state in controller and redraws the widget accordingly
		void Set_Pin_Output(uint32_t pinNo, bool state);

		// sets up the GUI
		void Setup_GUI();
		// triggers repainting of all child widgets
		void Trigger_Repaint(bool force = false);
};
