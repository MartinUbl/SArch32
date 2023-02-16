#pragma once

#include <vector>
#include <memory>

#include <QtWidgets/QWidget>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QTextEdit>

#include "../../core/isa.h"
#include "../../core/peripherals/uart.h"

/*
 * UART widget
 */
class CUART_Widget : public QWidget
{
	Q_OBJECT

	private:
		// reference to UART controller
		std::weak_ptr<sarch32::IUART_Controller> mUART_Ctl;

		// console output from UART
		QTextEdit* mConsole = nullptr;
		// line input for UART
		QLineEdit* mInput = nullptr;

		// cached console text
		std::string mConsole_Text{};

	signals:
		void Request_Console_Refresh();

	protected slots:
		void On_Console_Refresh();
		void On_Send_Button_Clicked();

	public:
		explicit CUART_Widget(std::shared_ptr<sarch32::IUART_Controller> uartCtl, QWidget* parent = nullptr) : QWidget(parent) {
			//
		}

		// sets up the UART console GUI
		void Setup_GUI();

		void Update_Console();
};
