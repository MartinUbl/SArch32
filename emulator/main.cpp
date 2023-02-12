#include <QtWidgets/QApplication>
#include "MainWindow.h"

int main(int argc, char** argv) {

	QApplication app(argc, argv);

	// TODO: parse config, verify number of parameters, ...

	// create main window
	CMain_Window* mwin = new CMain_Window(argv[1]);

	// set up the GUI and run
	mwin->Setup_GUI();
	mwin->show();

	return app.exec();
}
