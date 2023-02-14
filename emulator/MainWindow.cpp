#include "MainWindow.h"

#include "../core/sobjfile.h"

#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QMenuBar>
#include <QtGui/QTextBlock>
#include <QtGui/QTextLayout>
#include <QtGui/QAbstractTextDocumentLayout>

#include <iostream>

CMain_Window::CMain_Window()
	: QMainWindow() {
	//
}

bool CMain_Window::Setup_Machine(const CConfig& config) {

	mObject_File = config.Get_Memory_Image();

	// create machine
	if (config.Get_Machine_Name() == "default" || config.Get_Machine_Name() == "sarch32_001") {
		mMachine = std::make_unique<sarch32::CMachine>(config.Get_Memory_Size());
	}
	else {
		QMessageBox::critical(nullptr, "Error", tr("Unknown machine type: {}").arg(QString::fromStdString(config.Get_Machine_Name())));
		return false;
	}

	mMachine->Reset(false);

	// init memory from given file
	if (!mMachine->Init_Memory_From_File(config.Get_Memory_Image())) {
		QMessageBox::critical(nullptr, "Error", "Could not load memory object file");
	}

	for (const auto& peripherals = config.Get_Peripherals(); auto &p : peripherals) {

		if (p.first == "display") {
			if (p.second == "default" || p.second == "d1_monochromatic") {
				mDisplay = mMachine->Attach_Peripheral<sarch32::CDisplay_300x200>();
			}
			else {
				QMessageBox::critical(nullptr, "Error", tr("Unknown display: {}").arg(QString::fromStdString(p.second)));
				return false;
			}
		}
		else {
			QMessageBox::critical(nullptr, "Error", tr("Unknown peripheral: {}").arg(QString::fromStdString(p.first)));
			return false;
		}
	}

	return true;
}

void CMain_Window::On_Refresh_Registers() {

	// retrieve machine context and set register labels to their respective values

	const auto& ctx = mMachine->Get_CPU_Context();

	for (size_t i = 0; i < Register_Count; i++) {
		mReg_Labels[i]->setText(QString::number(ctx.Reg(static_cast<NRegister>(i)), mIs_Hexa ? 16 : 10));
	};
}

void CMain_Window::On_Refresh_Disassembly() {

	// clear extra selections and contents
	mDisassembly->setExtraSelections({});
	mDisassembly->clear();

	// disassembly is read-only and not accepting rich text - the formatting is done after the text is inserted
	mDisassembly->setReadOnly(true);
	mDisassembly->setAcceptRichText(false);
	mDisassembly->setStyleSheet("font-size: 16px; font-family: Consolas, Courier New, monospaced; line-height: 1.5em;");

	// load the object file
	SObj::CSObj_File objFile;
	if (!objFile.Load_From_File(mObject_File)) {
		QMessageBox::critical(nullptr, "Error", "Could not load memory object file");
		return;
	}

	// load sections and sort them from the earliest to latest
	auto& sec = objFile.Get_Sections();
	std::vector<SObj::TSection> sorted;

	for (auto& s : sec) {
		sorted.push_back(s.second);
	}

	std::sort(sorted.begin(), sorted.end(), [](const SObj::TSection& a, const SObj::TSection& b) { return a.startAddr < b.startAddr; });

	// format the outputs to stringstream
	std::ostringstream oss;

	// address formatter
	auto formatAddr = [this](uint32_t addr) -> std::string {
		std::ostringstream p;
		if (mIs_Hexa)
			p << std::hex << std::setw(8) << std::setfill('0');
		else
			p << std::dec << std::setw(10) << std::setfill('0');
		p << addr;
		return p.str();
	};

	// line counter
	size_t lineIndex = 0;

	// line break generator
	auto lineBreak = [&lineIndex]() {
		lineIndex++;
		return "\n";
	};

	// go through all sections and generate disassembly
	for (auto& s : sorted) {

		size_t cur = 0;

		// create section break at current line
		mSection_Breaks.push_back({ s.startAddr, lineIndex });

		// go through the section assembly with a step of 4 bytes
		for (cur = 0; cur < s.size; cur += 4) {

			// format address
			oss << formatAddr(s.startAddr + static_cast<uint32_t>(cur)) << (mIs_Hexa ? "     " : "   ");

			// try to format the instruction
			try {
				// parse binary, build instruction object and generate string
				uint32_t binary = *reinterpret_cast<uint32_t*>(&s.data[cur]);
				auto instr = CInstruction::Build_From_Binary(binary);

				oss << instr->Generate_String(mIs_Hexa);
			}
			catch (...) {
				// cannot parse instruction - output something to indicate this
				oss << "???";
			}

			oss << lineBreak();
		}

		// insert three dots between sections
		oss << "..." << lineBreak();
	}

	// sort section breaks for easier navigation when selecting line pointed to by PC
	std::sort(mSection_Breaks.begin(), mSection_Breaks.end(), [](const auto& a, const auto& b) { return a.startAddr < b.startAddr; });

	// set the text
	mDisassembly->setText(QString::fromStdString(oss.str()));

	// reformat the outputs
	auto fmt = QTextCharFormat();

	// at first, clear formatting from the whole document
	fmt.setFontUnderline(false);
	fmt.setBackground(QBrush(QColor::fromRgb(255, 255, 255)));
	fmt.setFontWeight(0);
	mDisassembly->selectAll();
	mDisassembly->textCursor().mergeCharFormat(fmt);
	mDisassembly->moveCursor(QTextCursor::Start);

	// set bold font with light red background to the "address" field
	fmt.setFontWeight(99);
	fmt.setBackground(QBrush(QColor::fromRgb(255, 240, 240)));

	// find all "first parts" of every line and highlight it
	while (mDisassembly->find(QRegExp(mIs_Hexa ? "^[0-9a-fA-F]{8}" : "^[0-9]{10}"))) {
		auto textcursor = mDisassembly->textCursor();
		// select the marked word (address)
		textcursor.select(QTextCursor::WordUnderCursor);
		// highlight also a few characters extra
		textcursor.movePosition(QTextCursor::MoveOperation::Right, QTextCursor::MoveMode::KeepAnchor, mIs_Hexa ? 3 : 1);
		textcursor.mergeCharFormat(fmt);
		mDisassembly->setTextCursor(textcursor);
	}

	emit Update_View_PC();
}

void CMain_Window::On_Update_View_PC() {

	// retrieve PC and match the section that contains the address
	uint32_t pc = mMachine->Get_CPU_Context().Reg(NRegister::PC);
	size_t lineIdx = 0;
	for (size_t i = 0; i < mSection_Breaks.size(); i++) {
		if (mSection_Breaks[i].startAddr <= pc) {
			// extract the line index as the section break + PC/4 as offset
			lineIdx = mSection_Breaks[i].lineIdx + ((pc - mSection_Breaks[i].startAddr) / 4);
		}
	}

	// create "extra" selection to mark the line of the PC occurrence
	QList<QTextEdit::ExtraSelection> extraSelections;
	QTextEdit::ExtraSelection selection;
	QColor lineBgColor = QColor(Qt::green).lighter(100);
	QColor lineFgColor = QColor(Qt::black);
	selection.format.setBackground(lineBgColor);
	selection.format.setForeground(lineFgColor);
	selection.format.setProperty(QTextFormat::FullWidthSelection, true);

	// TODO: move cursor, so that previous and next N lines are visible
	//mDisassembly->moveCursor(QTextCursor::End);
	QTextCursor cursor(mDisassembly->document()->findBlockByLineNumber(static_cast<int>(lineIdx)));
	mDisassembly->setTextCursor(cursor);

	selection.cursor = cursor;
	cursor.select(QTextCursor::LineUnderCursor);
	selection.cursor.clearSelection();

	extraSelections.append(selection);
	mDisassembly->setExtraSelections(extraSelections);
}

void CMain_Window::On_Step_Requested() {

	// single step
	mMachine->Step();

	// refresh register content and PC
	emit Refresh_Registers();
	emit Update_View_PC();

	// request repaint
	mDisplay_Widget->Trigger_Repaint(mDisplay, mMachine->Get_Memory_Bus());

	statusBar()->showMessage(tr("Step complete"));
}

void CMain_Window::On_Request_Repaint() {
	mDisplay_Widget->Trigger_Repaint(mDisplay, mMachine->Get_Memory_Bus());
}

void CMain_Window::On_Run_Requested() {

	if (mRun_Thread && mRun_Thread->joinable()) {
		mIs_Running = false;
		mRun_Thread->join();
	}

	mRun_Thread = std::make_unique<std::thread>(&CMain_Window::Run_Thread_Fnc, this);

	mIs_Running = true;
	statusBar()->showMessage(tr("Running..."));
}

void CMain_Window::On_Pause_Requested() {
	mIs_Running = false;

	statusBar()->showMessage(tr("Paused"));
}

void CMain_Window::On_Decimal_Fmt_Selected() {

	mIs_Hexa = false;

	emit Refresh_Registers();
	emit Refresh_Disassembly();
}

void CMain_Window::On_Hexadecimal_Fmt_Selected() {

	mIs_Hexa = true;

	emit Refresh_Registers();
	emit Refresh_Disassembly();
}

void CMain_Window::Run_Thread_Fnc() {

	emit Request_Update_Button_State();

	uint32_t pcPrev = mMachine->Get_CPU_Context().Reg(NRegister::PC);

	while (mIs_Running) {

		pcPrev = mMachine->Get_CPU_Context().Reg(NRegister::PC);

		mMachine->Step(1, true);

		if (mDisplay->Is_Video_Memory_Changed()) {
			emit Request_Repaint();
		}

		// detect stalling - an infinite loop
		if (mMachine->Get_CPU_Context().Reg(NRegister::PC) == pcPrev)
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	emit Request_Update_Button_State();
	// refresh register content and PC
	emit Refresh_Registers();
	emit Update_View_PC();
}

void CMain_Window::Update_Button_State() {
	mStep_Button->setEnabled(!mIs_Running);
	mRun_Button->setEnabled(!mIs_Running);
	mPause_Button->setEnabled(mIs_Running);
}

void CMain_Window::On_Update_Button_State() {
	Update_Button_State();
}

void CMain_Window::Setup_GUI() {

	setWindowIcon(QIcon(":img/logo.png"));
	setWindowTitle("SArch32 emulator");
	setWindowState(Qt::WindowState::WindowMaximized);

	/*
	 * Application top menu bar
	 */
	QMenuBar* menuBar = new QMenuBar(this);
	{
		// "File" menu
		QMenu* fileMenu = new QMenu("&File", menuBar);
		{
			auto quitAction = fileMenu->addAction("&Exit");
			connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
		}

		menuBar->addMenu(fileMenu);

		// "Help" menu
		QMenu* helpMenu = new QMenu("&Help", menuBar);
		{
			auto aboutAction = helpMenu->addAction("About");
			connect(aboutAction, SIGNAL(triggered()), this, SLOT(On_About_Clicked()));
		}

		menuBar->addMenu(helpMenu);
	}
	setMenuBar(menuBar);

	// central widget
	QWidget* central = new QWidget(this);
	QGridLayout* clay = new QGridLayout(central);
	{
		/*
		 * Controls
		 */

		central->setLayout(clay);

		QGroupBox* ctlbox = new QGroupBox("Control", central);
		QHBoxLayout* ctllay = new QHBoxLayout();
		{
			ctlbox->setLayout(ctllay);

			mStep_Button = new QPushButton(" Step", ctlbox);
			mStep_Button->setEnabled(!mIs_Running);
			mStep_Button->setIcon(QIcon(":img/step.png"));
			connect(mStep_Button, SIGNAL(clicked()), this, SLOT(On_Step_Requested()));
			ctllay->addWidget(mStep_Button);

			mRun_Button = new QPushButton(" Run", ctlbox);
			mRun_Button->setEnabled(!mIs_Running);
			mRun_Button->setIcon(QIcon(":img/run.png"));
			connect(mRun_Button, SIGNAL(clicked()), this, SLOT(On_Run_Requested()));
			ctllay->addWidget(mRun_Button);

			mPause_Button = new QPushButton(" Pause", ctlbox);
			mPause_Button->setEnabled(mIs_Running);
			mPause_Button->setIcon(QIcon(":img/pause.png"));
			connect(mPause_Button, SIGNAL(clicked()), this, SLOT(On_Pause_Requested()));
			ctllay->addWidget(mPause_Button);

			// TODO: more controls: Run, Pause, Step (5), Step (10), ...

			ctllay->addStretch(1);
		}

		clay->addWidget(ctlbox, 0, 0, 1, 2);
		clay->setRowStretch(0, 1);

		/*
		 * Registers view
		 */

		QGroupBox* gb = new QGroupBox("Registers", central);
		QFormLayout* vblay = new QFormLayout(gb);
		{
			gb->setLayout(vblay);

			for (size_t i = 0; i < Register_Count; i++) {

				mReg_Labels[i] = new QLabel("0", gb);
				mReg_Labels[i]->setStyleSheet("font-size: 14px; font-family: Consolas, Courier New, monospaced;");

				vblay->addRow(QString::fromStdString(Get_Register_Name(i)), mReg_Labels[i]);

				vblay->labelForField(mReg_Labels[i])->setStyleSheet("font-size: 14px; font-weight: bold; font-family: Consolas, Courier New, monospaced;");
			}

			vblay->setFormAlignment(Qt::AlignRight);
			vblay->setAlignment(Qt::AlignRight);
			vblay->setLabelAlignment(Qt::AlignRight);
			vblay->setVerticalSpacing(4);
		}

		clay->addWidget(gb, 1, 0);
		clay->setColumnStretch(0, 1);
		clay->setRowStretch(1, 10);

		/*
		 * View options
		 */

		QGroupBox* vobox = new QGroupBox("View controls", central);
		QVBoxLayout* volay = new QVBoxLayout();
		{
			vobox->setLayout(volay);

			QGroupBox* fmtbox = new QGroupBox("Number format", vobox);
			QHBoxLayout* fmtlay = new QHBoxLayout();
			{
				fmtbox->setLayout(fmtlay);

				QRadioButton* decnum = new QRadioButton("Decimal", fmtbox);
				decnum->setChecked(true);
				fmtlay->addWidget(decnum);

				QRadioButton* hexnum = new QRadioButton("Hexadecimal", fmtbox);
				fmtlay->addWidget(hexnum);

				connect(decnum, SIGNAL(clicked()), this, SLOT(On_Decimal_Fmt_Selected()));
				connect(hexnum, SIGNAL(clicked()), this, SLOT(On_Hexadecimal_Fmt_Selected()));
			}

			volay->addWidget(fmtbox);
		}

		clay->addWidget(vobox, 2, 0);
		clay->setRowStretch(2, 1);

		/*
		 * Disassembly view
		 */

		QGroupBox* da = new QGroupBox("Disassembly", central);
		QVBoxLayout* dala = new QVBoxLayout(da);
		{
			da->setLayout(dala);

			mDisassembly = new QTextEdit(da);
			dala->addWidget(mDisassembly);
		}

		clay->addWidget(da, 1, 1);
		clay->setColumnStretch(1, 5);

		/*
		 * Display view
		 */

		QGroupBox* rpanel = new QGroupBox("Peripherals", central);
		QVBoxLayout* rlay = new QVBoxLayout(rpanel);
		{
			rpanel->setLayout(rlay);

			QGroupBox* mon = new QGroupBox("Display", rpanel);
			QHBoxLayout* monlay = new QHBoxLayout();
			{
				mon->setLayout(monlay);

				mDisplay_Widget = new CDisplay_Widget(mon);
				mDisplay_Widget->setFixedWidth(300);
				mDisplay_Widget->setFixedHeight(200);
				monlay->addWidget(mDisplay_Widget, Qt::AlignHCenter);
			}

			rlay->addWidget(mon);
			rlay->addStretch(1);
		}

		clay->addWidget(rpanel, 1, 2);
		clay->setColumnStretch(2, 2);
	}

	setCentralWidget(central);

	// internal signals
	connect(this, SIGNAL(Refresh_Registers()), this, SLOT(On_Refresh_Registers()));
	connect(this, SIGNAL(Refresh_Disassembly()), this, SLOT(On_Refresh_Disassembly()));
	connect(this, SIGNAL(Update_View_PC()), this, SLOT(On_Update_View_PC()));
	connect(this, SIGNAL(Request_Update_Button_State()), this, SLOT(On_Update_Button_State()));
	connect(this, SIGNAL(Request_Repaint()), this, SLOT(On_Request_Repaint()));

	statusBar()->setStyleSheet("QStatusBar{ border-top: 1px outset grey; }");
	statusBar()->showMessage(tr("Ready"));

	// immediatelly refresh disassembly and register contents
	emit Refresh_Disassembly();
	emit Refresh_Registers();
	emit Request_Update_Button_State();
}

void CMain_Window::On_About_Clicked() {
	QMessageBox::about(this, "About", tr(	"<b>SArch32 emulator</b><br>Created by: Martin Ubl (<a href='mailto:martinubl@seznam.cz'>martinubl@seznam.cz</a>)<br><br>"
											"Experimental ISA, assembler and emulator, created for educational purposes.<br><br>"
											"Licensed under MIT license.<br><br>"
											"<a href='https://github.com/MartinUbl/SArch32'>https://github.com/MartinUbl/SArch32</a>"));
}

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
