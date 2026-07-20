#ifndef GUI_H
#define GUI_H

#include <QApplication>
#include <QMainWindow>
#include <QTextEdit>
#include <QMenu>
#include <QMenuBar>
#include "config.h"
#include "dance.h"
#include <QStatusBar>
#include <qaction.h>
#include <qcontainerfwd.h>
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
#include <QComboBox>

double m_to_px(double);
double xPos_to_px(double);
double yPos_to_px(double);

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


class FloorScene : public QGraphicsScene {
    Q_OBJECT
public:
    FloorScene(Floor*, QObject* = nullptr);
    void setXYOffset(int, int);
    bool topUp = false;
    bool dragging = false;
    double gridSize = .5;
    std::vector<Position*> positions;
protected:
    void drawBackground(QPainter* painter, const QRectF& rect) override;
    void drawForeground(QPainter* painter, const QRectF& rect) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
private:
    Floor* floor = nullptr;
    unsigned int getImWidth() const {return PX_M * floor->getWidth() + 2*BORDER;}
    unsigned int getImHeight() const {return PX_M * floor->getHeight() + 2*BORDER;}
    void drawTopLabel(QPainter*, QString) const;
    void drawBottomLabel(QPainter*, QString) const;
};


class PositionItem : public QGraphicsItem {
public:
    PositionItem(Position*, Floor*, Dancer*, bool*, bool*, double*);
    QRectF boundingRect() const override;
    void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override;
    void updatePos();
    static int diameter;
    static void drawDancerBody(QPainter*, Dancer*, int=0, int=0);
protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
private:
    Floor *floor = nullptr;
    Position* position = nullptr;
    Dancer* dancer = nullptr;
    bool *topUp = nullptr;
    bool *dragging = nullptr;
    double *gridSize = nullptr;
};


class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QMainWindow* parent=nullptr, Qt::WindowFlags flags=Qt::WindowFlags());
    DefinitionEditor* editor = nullptr;
    QDockWidget *textDock = nullptr,
                *listDock = nullptr;
    OutlineWidget *outline = nullptr;
    FloorScene *floorScene = nullptr;
    CanvasView* canvas = nullptr;
    Choreo choreo;
private:
    std::string filePath = "";
    QComboBox* gridSizeCombo = nullptr;
private slots:
    void newFile();
    void openFile();
    void saveFile();
    void saveAsFile();
    void loadScene(QListWidgetItem*);
    void loadSceneByIndex(int);
    void resetForNewChoreo();
    void pdfExport();
    void onGridSizeChanged(int);
};

#endif
