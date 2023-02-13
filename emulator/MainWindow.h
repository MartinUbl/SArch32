#pragma once

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QWidget>
#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>
#include <QtWidgets/QRadioButton>

#include <array>
#include <memory>
#include <thread>
#include <condition_variable>
#include <mutex>

#include "../core/isa.h"
#include "../core/machine.h"

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
		void Trigger_Repaint(const IBus& bus);

		void paintEvent(QPaintEvent* event) override;
};

class CEvent_Proxy : public QObject {
	Q_OBJECT

	public:
		//
};

/*
 * Emulator main window
 */
class CMain_Window : public QMainWindow {

	Q_OBJECT

	private:
		// widgets

		// register labels
		std::array<QLabel*, Register_Count> mReg_Labels;
		// disassembly text area
		QTextEdit* mDisassembly;
		// 300x200 display
		CDisplay_Widget* mDisplay_Widget;
		// step button
		QPushButton* mStep_Button;
		// run button
		QPushButton* mRun_Button;
		// pause button
		QPushButton* mPause_Button;

		// machine-related attributes

		// machine object
		std::unique_ptr<sarch32::CMachine> mMachine;
		// path to the memory object file used
		std::string mObject_File;

		// structure helper for finding the location of PC
		struct TSection_Break {
			uint32_t startAddr;
			size_t lineIdx;
		};

		// vector of section breaks
		std::vector<TSection_Break> mSection_Breaks;

		// should we print numbers in hex format?
		bool mIs_Hexa = false;
		// is the machine running freely?
		bool mIs_Running = false;

		// run thread for free running
		std::unique_ptr<std::thread> mRun_Thread;

	protected:
		void Run_Thread_Fnc();
		void Update_Button_State();

	signals:
		void Refresh_Registers();
		void Refresh_Disassembly();
		void Update_View_PC();
		void Request_Update_Button_State();
		void Request_Repaint();

	protected slots:
		// view related slots
		void On_Refresh_Registers();
		void On_Refresh_Disassembly();
		void On_Update_View_PC();
		void On_Update_Button_State();
		void On_Request_Repaint();

		// control related slots
		void On_Step_Requested();
		void On_Run_Requested();
		void On_Pause_Requested();
		void On_Decimal_Fmt_Selected();
		void On_Hexadecimal_Fmt_Selected();

	public:
		CMain_Window(const std::string& memSObjFile);

		// sets up the GUI for work
		void Setup_GUI();
};
