#include "gui.h"
#include "config.h"
#include "dance.h"
#include "utils.h"
#include <fstream>
#include <qaction.h>
#include <qapplication.h>
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
#include <QToolBar>
#include "export.h"
#include <QContextMenuEvent>

double m_to_px(double meter) {return meter*PX_M;}

double xPos_to_px(double meter) {return BORDER + meter*PX_M;}

double yPos_to_px(double meter) {return BORDER + meter*PX_M;}

// Document Outline
OutlineWidget::OutlineWidget(std::vector<Scene>& scenes, QWidget* parent):
QListWidget(parent), scenes(&scenes) {
    this->setEditTriggers(QAbstractItemView::DoubleClicked);
    this->setDragDropMode(QAbstractItemView::InternalMove);
    connect(this, &QListWidget::itemChanged, this, &OutlineWidget::onItemRenamed);
    connect(model(), &QAbstractItemModel::rowsMoved, this, &OutlineWidget::onItemMoved);
}

void OutlineWidget::load() {
    clear();
    for (int i = 0; i < this->scenes->size(); i++) {
        QListWidgetItem* item = new QListWidgetItem(QString::fromStdString((*scenes)[i].name));
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        item->setData(Qt::UserRole, i);
        this->addItem(item);
    }
}

void OutlineWidget::load(std::vector<Scene> &scenes) {
    this->scenes = &scenes;
    load();
}

void OutlineWidget::onItemRenamed(QListWidgetItem* item) {
    int index = row(item);
    if (index >= 0 && index < scenes->size()) {
        scenes->at(index).name = item->text().toStdString();
    }
}

void OutlineWidget::onItemMoved(const QModelIndex&, int, int, const QModelIndex&, int) {
    std::vector<Scene> newScenes;
    for (int i = 0; i < count(); i++) {
        int oldIndex = item(i)->data(Qt::UserRole).toInt();
        newScenes.push_back(scenes->at(oldIndex));
    }
    *scenes = std::move(newScenes);
    for (int i = 0; i < count(); i++) {
        item(i)->setData(Qt::UserRole, i);
    }
}

void OutlineWidget::contextMenuEvent(QContextMenuEvent *event) {
    QListWidgetItem *item = itemAt(event->pos());

    if (!item)
        return; // right-clicked empty space

    QMenu menu(this);

    QAction *addAction = menu.addAction("Add Scene");
    QAction *removeAction = menu.addAction("Remove Scene");

    QAction *chosen = menu.exec(event->globalPos());

    if (chosen == addAction)
        addScene(item);
    else if (chosen == removeAction)
        removeScene(item);
}

void OutlineWidget::addScene(QListWidgetItem* item) {
    int idx = row(item);
    if (idx < 0 || idx >= static_cast<int>(scenes->size())) {
        throw std::out_of_range("addScene: index out of range");
    }
    scenes->insert(scenes->begin()+idx, Scene(scenes->at(idx)));
    load();
}

void OutlineWidget::removeScene(QListWidgetItem* item) {
    int idx = row(item);
    if (idx < 0 || idx >= static_cast<int>(scenes->size())) {
        throw std::out_of_range("addScene: index out of range");
    }
    scenes->erase(scenes->begin()+idx);
    load();
}

// Main Window
MainWindow::MainWindow(QMainWindow* parent, Qt::WindowFlags flags):
QMainWindow(parent, flags) {
    setWindowTitle("CoCo");

    floorScene = new FloorScene(&this->choreo.floor, this);
    canvas = new CanvasView(floorScene, this);
    setCentralWidget(canvas);
    canvas->setRenderHint(QPainter::Antialiasing);
    canvas->setDragMode(QGraphicsView::RubberBandDrag);

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

    QAction* actSaveAsFile = fileMenu->addAction(tr("&Save As"));
    actSaveAsFile->setShortcut(QKeySequence(tr("Ctrl+Shift+S", "File|Save As")));
    actSaveAsFile->setStatusTip(tr("Saves the currently opened file."));
    connect(actSaveAsFile, &QAction::triggered, this, &MainWindow::saveAsFile);

    QAction* actPdfExport = fileMenu->addAction(tr("&Export"));
    actPdfExport->setShortcut(QKeySequence(tr("Ctrl+E", "File|Export")));
    actPdfExport->setStatusTip(tr("Exports the choreo to PDF."));
    connect(actPdfExport, &QAction::triggered, this, &MainWindow::pdfExport);

    gridSizeCombo = new QComboBox(this);
    gridSizeCombo->addItem(tr("No grid"), 0.0);
    gridSizeCombo->addItem(tr("Coarse (1 m)"), 1.0);
    gridSizeCombo->addItem(tr("Medium (0.5 m)"), 0.5);
    gridSizeCombo->addItem(tr("Fine (0.25 m)"), 0.25);
    gridSizeCombo->setCurrentIndex(3);   // matches FloorScene's default of 0.25
    connect(gridSizeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onGridSizeChanged);

    QToolBar* toolFile = addToolBar(tr("File"));
    toolFile->addAction(actSaveFile);
    toolFile->addWidget(gridSizeCombo);
}

void MainWindow::newFile() {
    choreo = Choreo();
    filePath = "";

    resetForNewChoreo();
    statusBar()->showMessage(tr("New file created."));
}

void MainWindow::onGridSizeChanged(int index) {
    floorScene->gridSize = gridSizeCombo->itemData(index).toDouble();
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
    filePath = fileName.toStdString();
    this->choreo = Choreo{filePath};

    resetForNewChoreo();
    statusBar()->showMessage(tr("File '%1' loaded.").arg(fileName));
}

void MainWindow::saveAsFile() {
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
    filePath = fileName.toStdString();
    saveFile();
}

void MainWindow::saveFile() {
    if (filePath.empty()) {
        saveAsFile();
        return;
    }
    std::ofstream file{filePath};
    file << std::setw(4) << choreo << "\n";
    file.close();
    statusBar()->showMessage(tr("File saved to '%1'.").arg(QString::fromStdString(filePath)));
}

void MainWindow::pdfExport() {
    QString fileName = QFileDialog::getSaveFileName(
        this,
        tr("Export File"),
        "",
        tr("PDF files (*.pdf)")
    );

    if (fileName.isEmpty()) {
        statusBar()->showMessage(tr("No file exported!"));
        return;
    }
    generatePDF(this->choreo, fileName.toStdString(), false);
}

void MainWindow::loadSceneByIndex(int index) {
    this->editor->scene = &this->choreo.scenes[index];
    this->editor->setText(QString::fromStdString(this->editor->scene->text));

    this->floorScene->clear();
    this->floorScene->positions.clear();
    for (auto& pos : this->editor->scene->positions) {
        PositionItem* posItem = new PositionItem(&pos, &choreo.floor, pos.dancer.get(),
                &floorScene->topUp, &floorScene->dragging, &floorScene->gridSize);
        posItem->updatePos();
        this->floorScene->addItem(posItem);
        this->floorScene->positions.push_back(&pos);
    }
    floorScene->update();
}

void MainWindow::loadScene(QListWidgetItem *item) {
    int index = item->data(Qt::UserRole).toInt();
    loadSceneByIndex(index);
}

void MainWindow::resetForNewChoreo() {

    floorScene->clear();
    floorScene->positions.clear();

    outline->load(choreo.scenes);

    if (!choreo.scenes.empty()) {
        loadSceneByIndex(0);
    }
}

// Definition Editor
DefinitionEditor::DefinitionEditor(QWidget* parent): QTextEdit(parent) {
    connect(this, &DefinitionEditor::textChanged, this, &DefinitionEditor::save);
}

void DefinitionEditor::save() {
    this->scene->text = this->toPlainText().toStdString();
}


FloorScene::FloorScene(Floor* floor, QObject* parent) : QGraphicsScene(parent), floor(floor) {
    setSceneRect(0, 0, getImWidth(), getImHeight());
}

void FloorScene::drawBackground(QPainter* painter, const QRectF& rect) {
    QColor borderColor(GREEN),
           fillColor(GRAY),
           gridColor("#a9a9a9");
    painter->setPen(Qt::NoPen);
    painter->setBrush(QBrush(fillColor));
    painter->drawRect(xPos_to_px(0) , yPos_to_px(0),
            m_to_px(floor->getWidth()), m_to_px(floor->getHeight()));
    painter->setPen(QPen(gridColor, 2));
    for (int x = xPos_to_px(1); x < xPos_to_px(floor->getWidth()); x += m_to_px(1)) {
        if (x == xPos_to_px(floor->getWidth()/2.))
            continue;
        painter->drawLine(x, yPos_to_px(0), x, yPos_to_px(floor->getHeight()));
    }
    for (int y = yPos_to_px(1); y < yPos_to_px(floor->getHeight()); y += m_to_px(1)) {
        if (y == yPos_to_px(floor->getHeight()/2.))
            continue;
        painter->drawLine(xPos_to_px(0), y, xPos_to_px(floor->getWidth()), y);
    }
    painter->setPen(QPen(borderColor, 5));
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(xPos_to_px(0), yPos_to_px(0), m_to_px(floor->getWidth()), m_to_px(floor->getHeight()));
    painter->drawLine(xPos_to_px(0), yPos_to_px(floor->getHeight()/2.), xPos_to_px(floor->getWidth()), yPos_to_px(floor->getHeight()/2.));
    painter->drawLine(xPos_to_px(floor->getWidth()/2.), yPos_to_px(0), xPos_to_px(floor->getWidth()/2.), yPos_to_px(floor->getHeight()));

    QFont voHiFont = painter->font();
    voHiFont.setPixelSize(PX_M*.45);
    painter->setPen(QPen(gridColor));
    painter->setFont(voHiFont);
    if (topUp) {
        drawTopLabel(painter, "Vorne");
        drawBottomLabel(painter, "Hinten");
    }
    else {
        drawTopLabel(painter, "Hinten");
        drawBottomLabel(painter, "Vorne");
    }
}

void FloorScene::drawForeground(QPainter* painter, const QRectF&) {
    QFont annotationFont = painter->font();
    annotationFont.setPixelSize(PX_M * .3);
    QFontMetrics fm(annotationFont);
    int annotationOffset = PX_M / 10;

    painter->setPen(QPen(Qt::black));
    painter->setFont(annotationFont);

    for (Position* position : positions) {
        double x, y;
        if (topUp) {
            y = yPos_to_px(floor->SizeBack - position->y);
            x = xPos_to_px(floor->SizeLeft + position->x);
        } else {
            y = yPos_to_px(floor->SizeBack + position->y);
            x = xPos_to_px(floor->SizeLeft - position->x);
        }

        if (position->y != 0) {
            QString text = QString::number(std::abs(position->y));
            int textWidth = fm.horizontalAdvance(text);
            int drawY = y - fm.height()/2. + fm.ascent();
            painter->drawText(xPos_to_px(0) + annotationOffset, drawY, text);
            painter->drawText(xPos_to_px(floor->getWidth()) - annotationOffset - textWidth, drawY, text);
        }
        if (position->x != 0) {
            QString text = QString::number(std::abs(position->x));
            int textWidth = fm.horizontalAdvance(text);
            int drawX = x - textWidth/2.;
            painter->drawText(drawX, yPos_to_px(0) + fm.ascent(), text);
            painter->drawText(drawX, yPos_to_px(floor->getHeight()) - fm.descent(), text);
        }
    }
}

void FloorScene::drawTopLabel(QPainter* painter, QString label) const {
    painter->drawText(
            QRect(xPos_to_px(0), 0, m_to_px(floor->getWidth()), BORDER),
            Qt::AlignHCenter | Qt::AlignVCenter,
            label
            );
}

void FloorScene::drawBottomLabel(QPainter* painter, QString label) const {
    painter->drawText(
            QRect(xPos_to_px(0), yPos_to_px(floor->getHeight()), m_to_px(floor->getWidth()), BORDER),
            Qt::AlignHCenter | Qt::AlignVCenter,
            label
            );
}

PositionItem::PositionItem(Position* position, Floor* floor, Dancer *dancer,
    bool *topUp, bool *dragging, double *gridSize) :
    position(position), floor(floor), dancer(dancer), topUp(topUp),
    dragging(dragging), gridSize(gridSize) {
    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);
}

int PositionItem::diameter = PX_M;

QRectF PositionItem::boundingRect() const {
    double radius = static_cast<double>(diameter) / 2.;
    return QRectF(-radius, -radius, diameter, diameter);
}

void PositionItem::drawDancerBody(QPainter* painter, Dancer* dancer, int x, int y) {
    int diameter = PositionItem::diameter;
    QFont dancerFont = painter->font();
    dancerFont.setPixelSize(diameter * .4);
    QColor col(dancer->color.c_str());
    painter->setPen(Qt::NoPen);
    painter->setBrush(QBrush(col));
    double centerX = x - diameter/2.,
           centerY = y - diameter/2.;
    painter->drawEllipse(centerX, centerY, diameter, diameter);
    painter->setPen(QPen(getTextColor(col)));
    painter->setFont(dancerFont);
    painter->drawText(QRect(centerX, centerY, diameter, diameter),
                       Qt::AlignCenter, dancer->shortcut.c_str());

}

void PositionItem::paint(QPainter* painter, const QStyleOptionGraphicsItem *style, QWidget*) {
    drawDancerBody(painter, dancer);

    if (style->state & QStyle::State_Selected) {
        painter->setPen(QPen(QColor("orange"), 4));
        painter->setBrush(Qt::NoBrush);
        double radius = static_cast<double>(diameter) / 2;
        painter->drawEllipse(QRectF(-radius, -radius, diameter, diameter));
    }
}

void FloorScene::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    dragging = true;
    QGraphicsScene::mousePressEvent(event);
}

void FloorScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    dragging = false;
    QGraphicsScene::mouseReleaseEvent(event);
}

void PositionItem::updatePos() {
    double px, py;
    if (*topUp) {
        py = yPos_to_px(floor->SizeBack - position->y);
        px = xPos_to_px(floor->SizeLeft + position->x);
    } else {
        py = yPos_to_px(floor->SizeBack + position->y);
        px = xPos_to_px(floor->SizeLeft - position->x);
    }
    setPos(px, py);
}

QVariant PositionItem::itemChange(GraphicsItemChange change, const QVariant& value) {
    if (change == ItemPositionChange && *dragging) {
        // runs BEFORE the move is applied — value is the proposed new pos.
        // Returning a modified QPointF here overrides where the item actually ends up.
        QPointF p = value.toPointF();
        double xMeter = (p.x() - BORDER) / PX_M;
        double yMeter = (p.y() - BORDER) / PX_M;

        // snap to a 0.25 m grid
        if (*gridSize > 0) {
            xMeter = std::round(xMeter / *gridSize) * *gridSize;
            yMeter = std::round(yMeter / *gridSize) * *gridSize;
        }

        // clamp to floor boundaries (0 .. width/height in meters)
        xMeter = std::clamp(xMeter, 0.0, static_cast<double>(floor->getWidth()));
        yMeter = std::clamp(yMeter, 0.0, static_cast<double>(floor->getHeight()));

        double snappedX = BORDER + xMeter * PX_M;
        double snappedY = BORDER + yMeter * PX_M;
        return QPointF(snappedX, snappedY);
    }

    if (change == ItemPositionHasChanged) {
        // runs AFTER the move is applied
        QPointF p = value.toPointF();
        double xMeter = (p.x() - BORDER) / PX_M;
        double yMeter = (p.y() - BORDER) / PX_M;
        if (*topUp) {
            position->x = xMeter - floor->SizeLeft;
            position->y = floor->SizeBack - yMeter;
        } else {
            position->x = floor->SizeLeft - xMeter;
            position->y = yMeter - floor->SizeBack;
        }
        if (scene()) scene()->update();
    }

    return QGraphicsItem::itemChange(change, value);
}

CanvasView::CanvasView(QGraphicsScene* scene, QWidget* parent)
    : QGraphicsView(scene, parent) {}

void CanvasView::resizeEvent(QResizeEvent* event) {
    QGraphicsView::resizeEvent(event);
    if (scene()) {
        fitInView(scene()->sceneRect(), Qt::KeepAspectRatio);
    }
}

