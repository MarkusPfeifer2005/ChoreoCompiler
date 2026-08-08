#ifndef GUI_H
#define GUI_H

#include <qaction.h>
#include <qcontainerfwd.h>
#include <qdialog.h>
#include <qgraphicsitem.h>
#include <qgraphicsview.h>
#include <qlistwidget.h>
#include <qmainwindow.h>
#include <qpaintdevice.h>
#include <qpainter.h>
#include <qstyleoption.h>
#include <qtextedit.h>
#include <qtmetamacros.h>
#include <qundostack.h>
#include <qwidget.h>

#include <QApplication>
#include <QColorDialog>
#include <QComboBox>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGraphicsItem>
#include <QGraphicsView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QMap>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QStatusBar>
#include <QTextEdit>
#include <QUndoCommand>
#include <QUndoStack>
#include <memory>
#include <string>
#include <vector>

#include "config.h"
#include "dance.h"

double m_to_px(double);
double xPos_to_px(double);
double yPos_to_px(double);

class RoleListWidget : public QListWidget {
    Q_OBJECT
   public:
    RoleListWidget(std::vector<std::shared_ptr<Role>>*, QWidget* = nullptr);
    void load();

   private:
    std::vector<std::shared_ptr<Role>>* roles;
};

class RoleDialog : public QDialog {
    Q_OBJECT
   public:
    RoleDialog(std::vector<std::shared_ptr<Role>>*, std::vector<std::shared_ptr<Dancer>>*,
               QWidget* = nullptr);

   private:
    std::vector<std::shared_ptr<Role>>* roles;
    std::vector<std::shared_ptr<Dancer>>* dancers;
    RoleListWidget* roleList;
    QLineEdit* nameEdit;
    QPushButton* colorButton;
    QSpinBox* zIndexSpin;
    QColor currentColor;
    void showRole(int index);
    void updateColorButton();
   private slots:
    void onSelectionChanged();
    void onNameEdited(const QString&);
    void onZIndexChanged(int);
    void onColorClicked();
    void onAddClicked();
    void onRemoveClicked();
};

class DancerListWidget : public QListWidget {
    Q_OBJECT
   public:
    DancerListWidget(std::vector<std::shared_ptr<Dancer>>*, QWidget* = nullptr);
    void load();

   private:
    std::vector<std::shared_ptr<Dancer>>* dancers;
};

class OutlineWidget : public QListWidget {
    Q_OBJECT
   public:
    std::vector<Scene>* scenes;
    OutlineWidget(std::vector<Scene>&, QWidget* = nullptr);
    void load();
    void load(std::vector<Scene>&);

   protected:
    void contextMenuEvent(QContextMenuEvent*) override;
   private slots:
    void onItemRenamed(QListWidgetItem*);
    void onItemMoved(const QModelIndex&, int, int, const QModelIndex&, int);
    void addScene(QListWidgetItem*);
    void removeScene(QListWidgetItem*);
};

class DancerDialog : public QDialog {
    Q_OBJECT
   public:
    DancerDialog(std::vector<std::shared_ptr<Dancer>>*, std::vector<std::shared_ptr<Role>>*,
                 QWidget* = nullptr);
    DancerListWidget* dancerList;

   private:
    std::vector<std::shared_ptr<Dancer>>* dancers;
    std::vector<std::shared_ptr<Role>>* roles;
    QLineEdit* nameEdit;
    QLineEdit* shortcutEdit;
    QComboBox* roleCombo;
    QPushButton* colorButton;
    QColor currentColor;
    void showDancer(int index);
    void updateColorButton();
    void reloadRoleCombo();
   private slots:
    void onSelectionChanged();
    void onNameEdited(const QString&);
    void onShortcutEdited(const QString&);
    void onRoleChanged(int);
    void onColorClicked();
    void onAddClicked();
    void onRemoveClicked();
};

class DefinitionEditor : public QTextEdit {
    Q_OBJECT
   public:
    DefinitionEditor(QWidget* = nullptr);
    std::string *definitionText = nullptr;
    void load(std::string*);
   private slots:
    void save();
};

class ResizableQGraphicsView : public QGraphicsView {
    Q_OBJECT
   public:
    ResizableQGraphicsView(QGraphicsScene* scene, QWidget* parent = nullptr);

   protected:
    void resizeEvent(QResizeEvent* event) override;
};

class PositionItem : public QGraphicsItem {
   public:
    PositionItem(Position*, Floor*, Dancer*, bool*, bool*, double*);
    QRectF boundingRect() const override;
    void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override;
    void updatePos();
    void setExactX(double meters);
    void setExactY(double meters);
    Dancer* getDancer() const { return dancer; }
    Position* getPosition() const { return position; }
    static int diameter;
    static void drawDancerBody(QPainter*, Dancer*, int = 0, int = 0);

   protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent*) override;

   private:
    Floor* floor = nullptr;
    Position* position = nullptr;
    Dancer* dancer = nullptr;
    bool* topUp = nullptr;
    bool* dragging = nullptr;
    double* gridSize = nullptr;
};

class MovePositionCommand : public QUndoCommand {
   public:
    MovePositionCommand(PositionItem* item, QPointF oldPos, QPointF newPos);
    void undo() override;
    void redo() override;

   private:
    PositionItem* item;
    QPointF oldPos, newPos;
};


class SceneEditor : public QGraphicsScene {
    Q_OBJECT
   public:
    SceneEditor(Floor*, std::vector<std::shared_ptr<Dancer>>*, QUndoStack*, QObject* = nullptr);
    void setXYOffset(int, int);
    bool topUp = false;
    bool dragging = false;
    double gridSize = .5;
    void clear();
    std::vector<Position*> positions;
    void load(Scene*);
    std::vector<std::shared_ptr<Dancer>> *dancers = nullptr;
   signals:
    void positionMoved(PositionItem*);
    void singlePositionSelected(QString, double, double);
    void selectionCleared();
    void updateCoordinateStatus(double, double);
    void removePosition(PositionItem*);
    void addPositionRequested(Dancer*, double, double);

   protected:
    void drawBackground(QPainter* painter, const QRectF& rect) override;
    void drawForeground(QPainter* painter, const QRectF& rect) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent*) override;

   private:
    Floor* floor = nullptr;
    QUndoStack* undoStack = nullptr;
    PositionItem* selectedPositionItem = nullptr;
    QMap<PositionItem*, QPointF> dragStartPositions;
    unsigned int getImWidth() const { return PX_M * floor->getWidth() + 2 * BORDER; }
    unsigned int getImHeight() const { return PX_M * floor->getHeight() + 2 * BORDER; }
    void drawTopLabel(QPainter*, QString) const;
    void drawBottomLabel(QPainter*, QString) const;
   public slots:
    void onManualXChanged(double);
    void onManualYChanged(double);
    void onSelectedPositionsChanged();
    void onPositionMoved(PositionItem*);
};

class MainWindow : public QMainWindow {
    Q_OBJECT
   public:
    MainWindow(QMainWindow* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());

   private:
    std::string filePath = "";
    QComboBox* gridSizeCombo = nullptr;
    QUndoStack* undoStack = nullptr;
    QWidget* positionStatusWidget = nullptr;
    QLabel* positionNameLabel = nullptr;
    QDoubleSpinBox* xSpin = nullptr;
    QDoubleSpinBox* ySpin = nullptr;
    Choreo choreo;
    DefinitionEditor* editor = nullptr;
    QDockWidget *textDock = nullptr, *listDock = nullptr;
    OutlineWidget* outline = nullptr;
    SceneEditor* sceneEditor = nullptr;
    ResizableQGraphicsView* rGraphicsView = nullptr;
    Scene *openScene = nullptr;
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
    void openRoleDialog();
    void openDancerDialog();
    void onSinglePositionSelected(QString, double, double);
    void onSelectionCleared();
    void onUpdateCoordinateStatus(double, double);
    void onRemovePosition(PositionItem*);
    void addPosition(Dancer*, double, double);
};

#endif
