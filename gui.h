#ifndef GUI_H
#define GUI_H

#include <QApplication>
#include <QMainWindow>
#include <QTextEdit>
#include <QMenu>
#include <QMenuBar>
#include "dance.h"
#include <QStatusBar>
#include <qmainwindow.h>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QMainWindow* parent=nullptr, Qt::WindowFlags flags=Qt::WindowFlags());
    QTextEdit* editor;
    QDockWidget* textDock;
    Choreo choreo;

private slots:
    void newFile();
    void openFile();
    void saveFile();
};

#endif
