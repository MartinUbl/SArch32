#include "uart_widget.h"

#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QPushButton>

void CUART_Widget::Setup_GUI() {

	QVBoxLayout* lay = new QVBoxLayout(this);
	setLayout(lay);

	// console output
	mConsole = new QTextEdit(this);
	mConsole->setFixedHeight(128);
	mConsole->setReadOnly(true);
	lay->addWidget(mConsole);

	// input from user
	mInput = new QLineEdit(this);
	lay->addWidget(mInput);

	// send button
	QPushButton* send = new QPushButton("Send", this);
	lay->addWidget(send);

	connect(this, SIGNAL(Request_Console_Refresh()), this, SLOT(On_Console_Refresh()));
	connect(send, SIGNAL(clicked()), this, SLOT(On_Send_Button_Clicked()));
}

void CUART_Widget::Update_Console() {

	// try to obtain a character from UART, if there is any
	if (auto ctl = mUART_Ctl.lock()) {
		bool success = false;
		char c = ctl->Get_Char(success);

		// update console on success
		if (success) {
			mConsole_Text += c;
			emit Request_Console_Refresh();
		}
	}
}

void CUART_Widget::On_Console_Refresh() {
	mConsole->setText(QString::fromStdString(mConsole_Text));
}

void CUART_Widget::On_Send_Button_Clicked() {

	// extract the string, empty the input
	const std::string text = mInput->text().toStdString();
	mInput->setText("");

	if (auto ctl = mUART_Ctl.lock()) {
		// put every character to the transmission
		for (const char c : text) {
			ctl->Put_Char(c);
		}
	}
}
