#ifndef GUI_H
#define GUI_H

#include <QApplication>
#include <QMainWindow>
#include <QTextEdit>
#include <QMenu>
#include <QMenuBar>
#include "dance.h"
#include <QStatusBar>
#include <qaction.h>
#include <qgraphicsitem.h>
#include <qgraphicsview.h>
#include <qlistwidget.h>
#include <qmainwindow.h>
#include <QListWidget>
#include <qpaintdevice.h>
#include <qpainter.h>
#include <qstyleoption.h>
#include <qtextedit.h>
#include <qtmetamacros.h>
#include <qwidget.h>
#include <vector>
#include <QGraphicsView>
#include <QGraphicsItem>


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


class CanvasView : public QGraphicsView {
    Q_OBJECT
public:
    CanvasView(QGraphicsScene* scene, QWidget* parent = nullptr);
protected:
    void resizeEvent(QResizeEvent* event) override;
};

class FloorItem : public QGraphicsItem {
public:
    FloorItem(Floor*);
    QRectF boundingRect() const override;
    void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override;
private:
    Floor* floor = nullptr;
};

class DancerItem : public QGraphicsItem {
public:
    DancerItem(Dancer*);
    QRectF boundingRect() const override;
    void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override;
    int x, y;
private:
    Dancer* dancer = nullptr;
};

class PositionItem : public QGraphicsItem {
public:
    PositionItem(Position*, Floor*);
    QRectF boundingRect() const override;
    void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override;
private:
    Floor *floor = nullptr;
    Position* position = nullptr;
};

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QMainWindow* parent=nullptr, Qt::WindowFlags flags=Qt::WindowFlags());
    DefinitionEditor* editor;
    QDockWidget *textDock,
                *listDock;
    OutlineWidget *outline;
    QGraphicsScene* graphicScene;
    CanvasView* canvas;
    FloorItem* floorItem = nullptr;
    Choreo choreo;

private slots:
    void newFile();
    void openFile();
    void saveFile();
    void loadScene(QListWidgetItem*);
};

#endif
