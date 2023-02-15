#include "gpio_widget.h"

#include <QtWidgets/QGridLayout>

CGPIO_Pin_Button_Widget::CGPIO_Pin_Button_Widget(uint32_t pinNo, CGPIO_Widget* parent) : QWidget(parent), mPin_No(pinNo), mParent(parent) {
	//
}

void CGPIO_Pin_Button_Widget::Setup_GUI() {
	setFixedWidth(20);
	setFixedHeight(20);
}

void CGPIO_Pin_Button_Widget::Set_State(sarch32::NGPIO_Mode_Generic mode, bool level) {
	mPin_Mode = mode;
	mPin_State = level;
}

void CGPIO_Pin_Button_Widget::paintEvent(QPaintEvent* event)
{
	// prepare painter
	QPainter painter;
	painter.begin(this);

	// fill background - default color is black
	QRect r(0, 0, width(), height());
	painter.fillRect(r, Qt::transparent);

	// prepare pen
	setAttribute(Qt::WA_OpaquePaintEvent);
	QPen linepen;
	linepen.setWidth(2);

	// try to antialias outputs
	painter.setRenderHint(QPainter::Antialiasing, true);

	// depending on pin mode, draw the circle accordingly
	switch (mPin_Mode) {
		// input - outline
		case sarch32::NGPIO_Mode_Generic::Input:
			linepen.setColor(mPin_State ? Qt::green : Qt::red);
			painter.setPen(linepen);
			painter.drawEllipse(QPoint(width() / 2, height() / 2), (width() / 2) - 1, (height() / 2) - 1);
			break;
		// output - filled circle
		case sarch32::NGPIO_Mode_Generic::Output:
			linepen.setColor(mPin_State ? Qt::green : Qt::red);
			painter.setPen(linepen);
			painter.setBrush(QBrush(mPin_State ? Qt::green : Qt::red, Qt::SolidPattern));
			painter.drawEllipse(QPoint(width() / 2, height() / 2), (width() / 2) - 1, (height() / 2) - 1);
			break;
		// alt pins - we are not interested in alternate pin function states
		case sarch32::NGPIO_Mode_Generic::Alt:
			linepen.setColor(Qt::blue);
			painter.setPen(linepen);
			painter.drawEllipse(QPoint(width() / 2, height() / 2), (width() / 2) - 1, (height() / 2) - 1);
			break;
	}

	auto str = QString::number(mPin_No);

	// text - pin number
	QFont fnt(painter.font().family(), 7, 99, false);
	painter.setFont(fnt);
	linepen.setColor(Qt::black);
	painter.setPen(linepen);

	QFontMetrics fm(painter.font());
	auto tw = fm.horizontalAdvance(str);

	painter.drawText((width() / 2) - (tw / 2), (height() / 2) + 4, str);

	QWidget::paintEvent(event);
	painter.end();
}

void CGPIO_Pin_Button_Widget::mouseReleaseEvent(QMouseEvent* event) {
	// user clicked with left button - toggle in if it is in input mode
	if (mPin_Mode == sarch32::NGPIO_Mode_Generic::Input && event->button() == Qt::LeftButton) {
		if (mParent) {
			mPin_State = !mPin_State;
			mParent->Set_Pin_Output(mPin_No, mPin_State);
		}
	}
}

void CGPIO_Pin_Button_Widget::Trigger_Repaint() {
	emit repaint();
}

void CGPIO_Widget::Setup_GUI() {

	if (auto ctl = mGPIO_Ctl.lock()) {

		const auto cnt = ctl->Get_Pin_Count();

		QGridLayout* lay = new QGridLayout(this);
		setLayout(lay);

		// add pin widgets
		for (uint32_t i = 0; i < cnt; i++) {

			CGPIO_Pin_Button_Widget* btn = new CGPIO_Pin_Button_Widget(i, this);
			btn->Setup_GUI();

			btn->Set_State(ctl->Get_Mode(i), ctl->Get_State(i));

			mGPIO_Btns.push_back(btn);

			lay->addWidget(btn, i / 16, i % 16);
		}
	}

	Trigger_Repaint();
}

void CGPIO_Widget::Trigger_Repaint() {

	// refresh state
	if (auto ctl = mGPIO_Ctl.lock()) {
		for (uint32_t i = 0; i < ctl->Get_Pin_Count(); i++) {
			if (mGPIO_Btns[i])
				mGPIO_Btns[i]->Set_State(ctl->Get_Mode(i), ctl->Get_State(i));
		}
	}

	// repaing widgets
	for (auto btn : mGPIO_Btns) {
		btn->Trigger_Repaint();
	}
}

void CGPIO_Widget::Set_Pin_Output(uint32_t pinNo, bool state) {

	// set pin output in controller
	if (auto ctl = mGPIO_Ctl.lock()) {
		ctl->Set_State(pinNo, state);

		// refresh and redraw widget
		if (mGPIO_Btns[pinNo]) {
			mGPIO_Btns[pinNo]->Set_State(ctl->Get_Mode(pinNo), ctl->Get_State(pinNo));
			mGPIO_Btns[pinNo]->Trigger_Repaint();
		}
	}

}
