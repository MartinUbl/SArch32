#include <QtWidgets/QApplication>
#include "MainWindow.h"

#include <iostream>

int main(int argc, char** argv) {

	QApplication app(argc, argv);

	// there must be exactly one parameter
	if (argc < 2) {
		std::cerr << "Invalid number of parameters. Usage:\n\n" << argv[0] << " <config file>" << std::endl;
		return 1;
	}

	// load config
	CConfig cfg;
	std::string err;
	if (!cfg.Load_From_File(argv[1], err)) {
		std::cerr << err << std::endl;
		return 2;
	}

	// create main window
	CMain_Window* mwin = new CMain_Window();

	// set up the GUI and run
	if (!mwin->Setup_Machine(cfg)) {
		return 3;
	}
	mwin->Setup_GUI();
	mwin->show();

	return app.exec();
}
