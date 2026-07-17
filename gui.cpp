#include "gui.h"
#include "dance.h"
#include <fstream>
#include <qaction.h>
#include <qcoreapplication.h>
#include <qdockwidget.h>
#include <qgraphicsscene.h>
#include <qgraphicsview.h>
#include <qkeysequence.h>
#include <QString>
#include <QAction>
#include <QStyle>
#include <QFileDialog>
#include <qlistwidget.h>
#include <qmainwindow.h>
#include <QDockWidget>
#include <qnamespace.h>
#include <qpaintdevice.h>
#include <qpainter.h>
#include <qstyleoption.h>
#include <qtextedit.h>
#include <QListWidget>
#include <qwidget.h>
#include <vector>

OutlineWidget::OutlineWidget(std::vector<Scene>& scenes, QWidget* parent):
QListWidget(parent), scenes(scenes) {
    this->setEditTriggers(QAbstractItemView::DoubleClicked);
    this->setDragDropMode(QAbstractItemView::InternalMove);
    connect(this, &QListWidget::itemChanged, this, &OutlineWidget::onItemRenamed);
    connect(model(), &QAbstractItemModel::rowsMoved, this, &OutlineWidget::onItemMoved);
}

void OutlineWidget::load() {
    clear();
    for (int i = 0; i < this->scenes.size(); i++) {
        QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(scenes[i].name));
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        item->setData(Qt::UserRole, i);
        this->addItem(item);
    }
}

void OutlineWidget::load(std::vector<Scene> &scenes) {
    this->scenes = scenes;
    load();
}

void OutlineWidget::onItemRenamed(QListWidgetItem* item) {
    int index = row(item);
    if (index >= 0 && index < scenes.size()) {
        scenes[index].name = item->text().toStdString();
    }
}

void OutlineWidget::onItemMoved(const QModelIndex&, int, int, const QModelIndex&, int) {
    std::vector<Scene> newScenes;
    for (int i = 0; i < count(); i++) {
        int oldIndex = item(i)->data(Qt::UserRole).toInt();
        newScenes.push_back(scenes[oldIndex]);
    }
    scenes = newScenes;
    for (int i = 0; i < count(); i++) {
        item(i)->setData(Qt::UserRole, i);
    }
}

MainWindow::MainWindow(QMainWindow* parent, Qt::WindowFlags flags):
QMainWindow(parent, flags) {
    setWindowTitle("CoCo");

    graphicScene = new QGraphicsScene(this);
    canvas = new CanvasView(graphicScene, this);
    setCentralWidget(canvas);
    canvas->setRenderHint(QPainter::Antialiasing);
    canvas->setDragMode(QGraphicsView::RubberBandDrag);

    graphicScene->setSceneRect(0, 0, choreo.floor.getImWidth(), choreo.floor.getImHeight());
    floorItem = new FloorItem(&choreo.floor);
    graphicScene->addItem(floorItem);

    editor = new DefinitionEditor;
    textDock = new QDockWidget(tr("definition text"), this);
    textDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    textDock->setWidget(editor);
    addDockWidget(Qt::RightDockWidgetArea, textDock);

    listDock = new QDockWidget(tr("titles"), this);
    listDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    outline = new OutlineWidget{this->choreo.scenes};
    listDock->setWidget(outline);
    addDockWidget(Qt::LeftDockWidgetArea, listDock);
    connect(outline, &QListWidget::itemClicked, this, &MainWindow::loadScene);

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

    outline->load(choreo.scenes);
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

DefinitionEditor::DefinitionEditor(QWidget* parent): QTextEdit(parent) {
    connect(this, &DefinitionEditor::textChanged, this, &DefinitionEditor::save);
}

void DefinitionEditor::save() {
    this->scene->text = this->toPlainText().toStdString();
}

void MainWindow::loadScene(QListWidgetItem *item) {
    int index = item->data(Qt::UserRole).toInt();
    this->editor->scene = &this->choreo.scenes[index];
    this->editor->setText(QString::fromStdString(this->editor->scene->text));

    this->graphicScene->removeItem(this->floorItem);
    this->graphicScene->clear();
    this->graphicScene->addItem(this->floorItem);
    for (auto& pos : this->editor->scene->positions) {
        PositionItem* posItem = new PositionItem(&pos, &(choreo.floor));
        this->graphicScene->addItem(posItem);
    }
}

FloorItem::FloorItem(Floor* floor) : floor(floor) {
    floor->setXYOffset(0, 0);
}

void FloorItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* style, QWidget* widget) {
    this->floor->draw(*painter);
}

QRectF FloorItem::boundingRect() const {
    return QRectF(0,0,floor->getImWidth(), floor->getImWidth());
}

DancerItem::DancerItem(Dancer* dancer) : dancer(dancer) {}

QRectF DancerItem::boundingRect() const {
    double radius = static_cast<double>(dancer->diameter)/2;
    return QRectF(-radius, -radius, radius, radius);
}

void DancerItem::paint(QPainter* painter, const QStyleOptionGraphicsItem *style, QWidget *widget) {
    this->dancer->draw(*painter, x, y);
}

PositionItem::PositionItem(Position* position, Floor* floor) : position(position),
floor(floor){}

QRectF PositionItem::boundingRect() const {
    double radius = static_cast<double>(position->dancer->diameter)/2;
    return QRectF(-radius, -radius, radius, radius);
}

void PositionItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* style, QWidget *widget) {
    this->position->draw(*painter, *floor);
}

CanvasView::CanvasView(QGraphicsScene* scene, QWidget* parent)
    : QGraphicsView(scene, parent) {}

void CanvasView::resizeEvent(QResizeEvent* event) {
    QGraphicsView::resizeEvent(event);
    if (scene()) {
        fitInView(scene()->sceneRect(), Qt::KeepAspectRatio);
    }
}

