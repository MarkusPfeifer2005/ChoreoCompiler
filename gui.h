#ifndef GUI_H
#define GUI_H

#include <QApplication>
#include <QMainWindow>
#include <QTextEdit>
#include <QMenu>
#include <QMenuBar>
#include "dance.h"
#include <QStatusBar>
#include <memory>
#include <qlistwidget.h>
#include <qmainwindow.h>
#include <QListWidget>
#include <qtextedit.h>
#include <qtmetamacros.h>
#include <vector>


class OutlineWidget : public QListWidget {
    Q_OBJECT
public:
    std::vector<Scene> &scenes;
    OutlineWidget(std::vector<Scene>&, QWidget* = nullptr);
    void load();
    void load(std::vector<Scene>&);
private slots:
    void onItemRenamed(QListWidgetItem* item);
    void onItemMoved(const QModelIndex&, int, int, const QModelIndex&, int);
};

class DefinitionEditor : public QTextEdit {
    Q_OBJECT
public:
    DefinitionEditor(QWidget* = nullptr);
    Scene *scene = nullptr;
private slots:
    void save();
};

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QMainWindow* parent=nullptr, Qt::WindowFlags flags=Qt::WindowFlags());
    DefinitionEditor* editor;
    QDockWidget *textDock,
                *listDock;
    OutlineWidget *outline;
    Choreo choreo;

private slots:
    void newFile();
    void openFile();
    void saveFile();
    void loadDefinition(QListWidgetItem*);
};

#endif
