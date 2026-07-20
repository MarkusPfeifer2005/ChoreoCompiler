#include <cstdlib>
#include <cstring>
#include <iostream>
#include <ostream>
#include <qapplication.h>
#include <qboxlayout.h>
#include <qcontainerfwd.h>
#include <qlayout.h>
#include <qmainwindow.h>
#include <qobjectdefs.h>
#include <qpagesize.h>
#include <qpainter.h>
#include <qpdfwriter.h>
#include <qslider.h>
#include <qwidget.h>
#include <string>
#include <QApplication>
#include <QWidget>
#include <QImage>
#include <QPainter>
#include <QColor>
#include <QString>
#include <QColor>
#include <QPdfWriter>
#include <QPageSize>
#include <QPushButton>
#include <QVBoxLayout>
#include <QSpinBox>
#include <QSlider>
#include "dance.h"
#include <QMainWindow>
#include <QTextEdit>
#include "gui.h"
#include "export.h"

void showHelp() {
    std::cout << "Usage: coco [OPTION] [FILE]...\n";
    std::cout << "  -h, --help\tdisplay this help\n";
    std::cout << "  -l, --list\tlist all dancers\n";
    std::cout << "  -a, --anki\tgenerate anki cards for the provided dancer\n";
    std::cout << "  -p, --pdf \tgenerate pdf of the choreography\n";
    std::cout << "      --topUp\torient the top of the dance floor to face upwards\n";
    std::cout << "\nExamples:\n";
    std::cout << "coco  # starts the GUI\n";
    std::cout << "coco --list MyChoreography.choreo\n";
    std::cout << "coco --anki 'FistName LastName' MyChoreography.choreo\n";
    std::cout << "coco --pdf MyChoreography.choreo Output.pdf\n";
    std::cout << "coco --pdf MyChoreography.choreo Output.pdf --topUp\n";
    std::cout << std::endl;
}


int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    if (argc < 2) {
        MainWindow mainWindow;
        mainWindow.show();
        return app.exec();
    }
    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        showHelp();
        return EXIT_SUCCESS;
    }
    if (strcmp(argv[1], "--list") == 0 || strcmp(argv[1], "-l") == 0) {
        Choreo choreo{argv[2]};
        for (auto dancer : choreo.dancers) {
            std::cout << dancer->name << "\n";
        }
        return EXIT_SUCCESS;
    }
    else if (strcmp(argv[1], "--anki") == 0 || strcmp(argv[1], "-a") == 0) {
        generateAnki(argv[3], argv[2]);
        return EXIT_SUCCESS;
    }
    else if (strcmp(argv[1], "--pdf") == 0 || strcmp(argv[1], "-p") == 0) {
        bool topUp = false;
        std::string pdfName = "out.pdf";
        std::string choreoFileName = "";
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];
            if (arg == "--topUp") {
                topUp = true;
            }
            else if (arg.ends_with(".pdf")) {
                pdfName = arg;
            }
            else if (arg.ends_with(".choreo")) {
                choreoFileName = arg;
            }
        }

        Choreo choreo(choreoFileName);
        generatePDF(choreo, pdfName, topUp);
        return EXIT_SUCCESS;
    }
    showHelp();
    return EXIT_SUCCESS;
}

