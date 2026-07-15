#include "gui.h"
#include "dance.h"
#include <fstream>
#include <qaction.h>
#include <qcoreapplication.h>
#include <qkeysequence.h>
#include <QString>
#include <QAction>
#include <QStyle>
#include <QFileDialog>
#include <qmainwindow.h>
#include <QDockWidget>
#include <qnamespace.h>
#include <qtextedit.h>

MainWindow::MainWindow(QMainWindow* parent, Qt::WindowFlags flags):
QMainWindow(parent, flags) {
    editor = new QTextEdit;
    setCentralWidget(editor);
    setWindowTitle("CoCo");

    this->textDock = new QDockWidget(tr("definition text"), this);
    textDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    QTextEdit* defEditor = new QTextEdit;
    textDock->setWidget(defEditor);
    addDockWidget(Qt::RightDockWidgetArea, textDock);

    statusBar();

    QMenu *fileMenu = new QMenu(tr("&File"), this);
    menuBar()->addMenu(fileMenu);

    QAction* actNewFile = fileMenu->addAction(tr("&New"));
    actNewFile->setIcon(QApplication::style()->standardIcon(QStyle::SP_FileIcon));
    actNewFile->setShortcut(QKeySequence(tr("Ctrl+N", "File|New")));
    connect(actNewFile, &QAction::triggered, this, &MainWindow::newFile);
    actNewFile->setStatusTip(tr("Creates a new file."));

    QAction* actOpenFile = fileMenu->addAction(tr("&Open"));
    actOpenFile->setIcon(QApplication::style()->standardIcon(QStyle::SP_DirOpenIcon));
    actOpenFile->setShortcut(QKeySequence(tr("Ctrl+O", "File|Open")));
    actOpenFile->setStatusTip(tr("Opens a .choreo file."));
    connect(actOpenFile, &QAction::triggered, this, &MainWindow::openFile);

    QAction* actSaveFile = fileMenu->addAction(tr("&Save"));
    actSaveFile->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogSaveButton));
    actSaveFile->setShortcut(QKeySequence(tr("Ctrl+S", "File|Save")));
    actSaveFile->setStatusTip(tr("Saves the currently opened file."));
    connect(actSaveFile, &QAction::triggered, this, &MainWindow::saveFile);

}

void MainWindow::newFile() {
    editor->clear();
}

void MainWindow::openFile() {
    QString fileName = QFileDialog::getOpenFileName(
            this,
        tr("Open File"),
        "",
        tr("JSON and Choreo files (*.json *.choreo)")
    );
    if (fileName.isEmpty()) {
        statusBar()->showMessage(tr("No file loaded!"));
        return;
    }
    this->choreo = Choreo{fileName.toStdString()};
    statusBar()->showMessage(tr("File '%1' loaded.").arg(fileName));
}

void MainWindow::saveFile() {
    QString fileName = QFileDialog::getSaveFileName(
        this,
        tr("Save File"),
        "",
        tr("JSON and Choreo files (*.json *.choreo)")
    );
    if (fileName.isEmpty()) {
        statusBar()->showMessage(tr("No file saved!"));
        return;
    }
    std::ofstream file{fileName.toStdString()};
    file << std::setw(4) << choreo << "\n";
    file.close();
}
