#include "export.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <memory>
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
#include <vector>
#include "qbrush.h"
#include "qcolor.h"
#include "qfont.h"
#include "qfontmetrics.h"
#include "qimage.h"
#include "qnamespace.h"
#include "qpaintdevice.h"
#include "qpen.h"
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
#include "utils.h"
#include "config.h"
#include <QMainWindow>
#include <QTextEdit>
#include "gui.h"



namespace fs = std::filesystem;

SceneEditor* buildSceneForExport(Scene& scene, Floor& floor, int roleID, bool topUp) {
    SceneEditor* exportScene = new SceneEditor(&floor, nullptr, nullptr);
    exportScene->topUp = topUp;
    for (auto& pos : scene.positions) {
        PositionItem* item = new PositionItem(&pos, &floor, pos.dancer.get(), &exportScene->topUp,
                &exportScene->dragging, &exportScene->gridSize);
        item->updatePos();
        if (roleID >= 0 && pos.dancer->role->id == roleID) {
            item->setZValue(1);
        }
        exportScene->addItem(item);
        exportScene->positions.push_back(&pos);
    }
    return exportScene;
}

QImage renderSceneToImage(Scene& scene, Floor& floor, int roleID = -1, bool topUp = true) {
    std::unique_ptr<SceneEditor> exportScene{buildSceneForExport(scene, floor, roleID, topUp)};
    QImage image(exportScene->sceneRect().width(), exportScene->sceneRect().height(), QImage::Format_ARGB32);
    image.fill(Qt::white);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    exportScene->render(&painter);
    painter.end();
    return image;
}

void renderSceneToPdf(QPainter& painter, Scene& scene, Floor& floor, QPointF targetTopLeft, int roleID = -1, bool topUp = true) {
    SceneEditor* exportScene = buildSceneForExport(scene, floor, roleID, topUp);
    QRectF sceneRect = exportScene->sceneRect();
    QRectF targetRect(targetTopLeft, sceneRect.size());   // native size, positioned at targetTopLeft — same semantics as the old setXYOffset
    exportScene->render(&painter, targetRect, sceneRect);
    delete exportScene;
}

void generateAnki(std::string choreoFileName, std::string dancerName) {
    Choreo choreo{choreoFileName};
    bool found = false;
    int dancerID = 0,
        dancerRoleID = 0;
    for (const auto dancer : choreo.dancers) {
        if (dancer->name == dancerName) {
            found = true;
            dancerID = dancer->id;
            dancerRoleID = dancer->role->id;
            break;
        }
    }
    if (!found) {
        std::cerr << "Name does not exit!" << std::endl;
        exit(EXIT_FAILURE);
    }
    std::string dirName = dancerName;
    std::replace(dirName.begin(), dirName.end(), ' ', '_');
    dirName.erase(std::remove(dirName.begin(), dirName.end(), '/'), dirName.end());
    if (!fs::exists(dirName)) {
        fs::create_directory(dirName);
    }
    std::string txtName = dirName;

    std::ofstream notes{dirName + "/" + txtName + ".txt"};
    notes << "#separator:tab\n";
    notes << "#html:true\n";
    notes << "#notetype Basic\n";

    for (Scene scene : choreo.scenes) {
        QImage image = renderSceneToImage(scene, choreo.floor, dancerRoleID);

        notes << "\"" << find_and_replace(scene.name, "\"", "\"\"") << "\"\t\"";
        for (const Position pos : scene.positions) {
            if (pos.dancer->id == dancerID) {
                notes << std::abs(pos.x) << "|" << std::abs(pos.y) << "<br>";
            }
        }

        std::string imageName = dirName + "_" + scene.name + ".jpg";
        imageName = find_and_replace(imageName, "\"", "");
        imageName = find_and_replace(imageName, "/", "");
        notes << "<img src=\"\"" << imageName << "\"\"><br>";

        std::string text = find_and_replace(scene.text, "\r\n", "<br>");
        text = find_and_replace(text, "\"", "\"\"");
        notes << text << "\"\n";

        image.save((dirName + "/" + imageName).c_str(), "JPG");
    }
    notes.close();
}

void drawTextBox(QPainter& painter, const QString& bodyText) {
    QColor choreoGreen(GREEN),
           textGray(GRAY);
    int pageWidth = painter.device()->width();

    // dimensions
    int boxX      = MARGIN;
    int headerY   = 2285;
    int headerH   = 77;
    int bodyY     = headerY + headerH;
    int bodyH     = 3426 - bodyY;
    int innerMargin = 30;

    // green header bar
    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(choreoGreen));
    painter.drawRect(boxX, headerY, pageWidth - 2*MARGIN, headerH);

    // gray body area
    painter.setBrush(QBrush(textGray));
    painter.drawRect(boxX, bodyY, pageWidth - 2*MARGIN, bodyH);

    // header text
    painter.setFont(QFont("Arial", 15, QFont::Bold));
    painter.setPen(QColor("white"));
    painter.drawText(
        QRect(boxX + innerMargin, headerY, pageWidth - 2*MARGIN - innerMargin, headerH),
        Qt::AlignLeft | Qt::AlignVCenter,
        "Definitionen"
    );

    // body text — wraps automatically within the gray area minus inner margin
    painter.setFont(QFont("Arial", 12));
    painter.setPen(QColor("black"));
    painter.drawText(
        QRect(boxX + innerMargin, bodyY + innerMargin,
              pageWidth - 2*MARGIN - 2*innerMargin,
              bodyH - 2*innerMargin),
        Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
        bodyText
    );
}

void drawFooterHeaderBoxes(QPainter& painter) {
    QColor choreoGreen(GREEN);
    int pageWidth = painter.device()->width();
    int pageHeight = painter.device()->height();

    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(choreoGreen));
    painter.drawRect(0, 0, pageWidth, HEADER_HEIGHT);
    painter.drawRect(0, pageHeight - FOOTER_HEIGHT, pageWidth, FOOTER_HEIGHT);

    painter.setFont(QFont("Arial", 8));
    painter.setPen(QColor("white"));
    painter.drawText(
            QRect(0, 0, pageWidth, HEADER_HEIGHT),
        Qt::AlignHCenter | Qt::AlignVCenter,
        "Erstellt mit ChoreoCompiler - Source Code: https://github.com/MarkusPfeifer2005/ChoreoCompiler"
    );
}

void drawFooterHeader(QPainter& painter, unsigned int pageNum, unsigned int totalPages, std::string choreoTitle) {
    int pageWidth = painter.device()->width();
    int pageHeight = painter.device()->height();
    drawFooterHeaderBoxes(painter);

    painter.setPen(QColor("white"));
    painter.setFont(QFont("Arial", 10));
    painter.drawText(
            QRect(MARGIN, pageHeight - FOOTER_HEIGHT, pageWidth - MARGIN, FOOTER_HEIGHT),
        Qt::AlignLeft | Qt::AlignVCenter,
        "Seite " + QString::number(pageNum) + "/" + QString::number(totalPages) + " (" + QString::fromStdString(choreoTitle) + ")"
    );
}

void drawTitle(QPainter& painter, std::string title, unsigned int pageNum, unsigned int totalPages) {
    int pageWidth = painter.device()->width();
    int headerHeight = 25; // must match drawFooterHeader
    int titleHeight = 100;
    int titleY = 40; // start right below the header
    int fontSize = 12;

    // Left: title text
    painter.setFont(QFont("Arial", fontSize));
    painter.setPen(QColor(GREEN));
    painter.drawText(
        QRect(MARGIN, titleY, pageWidth / 2, titleHeight),
        Qt::AlignLeft | Qt::AlignVCenter,
        QString::number(pageNum) + "/" + QString::number(totalPages)
    );

    // Right: bold centered text
    painter.setFont(QFont("Arial", fontSize, QFont::Bold));
    painter.drawText(
        QRect(0, titleY, pageWidth, titleHeight),
        Qt::AlignHCenter | Qt::AlignVCenter,
        title.c_str()
    );
}

void drawSidePanel(QPainter& painter, Scene& scene, std::vector<std::shared_ptr<Role>>& roles) {
    QColor choreoGreen(GREEN);

    int numRoles = roles.size();
    int numPos = scene.positions.size();
    int H = 1800;
    int headerHeight = 80;
    int pageWidth = painter.device()->width();
    int rowHeight = (H/numRoles - headerHeight) / (numPos/numRoles);
    int rowWidth = 460;
    int Y = 150;
    int X = pageWidth - MARGIN - rowWidth;

    QFont font("Courier New");
    font.setPixelSize(rowHeight / 2.9);
    
    std::sort(roles.begin(), roles.end(), [](const auto& a, const auto& b) {
            return a->id > b->id;
            });
    std::sort(scene.positions.begin(), scene.positions.end(), [](const auto& a, const auto& b) {
            return a.y < b.y;
            });

    for (auto role : roles) {
        // draw header
        Y += headerHeight;  // whitespace
        painter.setPen(Qt::NoPen);
        painter.setBrush(QBrush(choreoGreen));
        painter.drawRect(X, Y, rowWidth, headerHeight);

        // header text
        painter.setPen(QColor("white"));
        painter.setFont(QFont("Arial", 12, QFont::Bold));
        painter.drawText(
            QRect(X, Y, rowWidth, headerHeight),
            Qt::AlignHCenter | Qt::AlignVCenter,
            QString::fromStdString(role->name)
        );

        Y += headerHeight;
        painter.setPen(Qt::NoPen);

        // draw rows
        for (const auto& pos : scene.positions) {
            if (pos.dancer->role->id == role->id) {
                painter.setPen(Qt::NoPen);
                QColor col(pos.dancer->color.c_str());
                painter.setBrush(QBrush(col));
                painter.drawRect(X, Y, rowWidth, rowHeight);

                // row text
                QString text = QString::fromStdString(pos.dancer->shortcut + ": ");
                text.append(QString::number(std::abs(pos.x), 'f', 2));
                if (pos.x > 0) {
                    text.append(" re | ");
                }
                else {
                    text.append(" li | ");
                }
                text.append(QString::number(std::abs(pos.y), 'f', 2));
                if (pos.y > 0) {
                    text.append(" vo");
                }
                else {
                    text.append(" hi");
                }

                painter.setPen(getTextColor(col));
                painter.setFont(font);
                painter.drawText(
                    QRect(X, Y, rowWidth, rowHeight),
                    Qt::AlignHCenter | Qt::AlignVCenter,
                    text
                );

                Y += rowHeight;
            }
        }
    }
}

void drawTeamList(QPainter& painter,
        std::vector<std::shared_ptr<Dancer>>& dancers,
        std::vector<std::shared_ptr<Role>>& roles) {

    int pageWidth = painter.device()->width();
    int Y = 300;
    int roleWidth = (pageWidth - 2*MARGIN) * 2/10;
    int symbolWidth = (pageWidth - 2*MARGIN) * 1/10;
    int nameWidth = (pageWidth - 2*MARGIN) * 7/10;
    int H = 2500;
    int numRoles = roles.size();
    int numDancers = dancers.size();
    int rowHeight = H/numRoles / (numDancers/numRoles + 1) ;

    QColor choreoGreen(GREEN);

    // Title above the list
    painter.setFont(QFont("Arial", 25, QFont::Bold));
    painter.setPen(QColor(GREEN));
    painter.drawText(
        QRect(MARGIN, 150, pageWidth -2*MARGIN, 100),  // 150 = your starting Y before the loop
        Qt::AlignHCenter | Qt::AlignVCenter,
        "Tänzerinnen und Tänzer"
    );

    for (const auto& role : roles) {
        Y+=rowHeight;
        for (const auto& dancer :  dancers) {
            if (dancer->role->id != role->id) {
                continue;
            }
            painter.setFont(QFont("Arial", 18));
            painter.setPen(QColor(GREEN));
            painter.drawText(
                    QRect(MARGIN, Y, roleWidth, rowHeight),
                    Qt::AlignHCenter | Qt::AlignVCenter,
                    role->name.c_str()
                    );
            painter.drawText(
                    QRect(MARGIN + roleWidth + symbolWidth, Y, nameWidth, rowHeight),
                    Qt::AlignHCenter | Qt::AlignVCenter,
                    dancer->name.c_str()
                    );
            PositionItem::drawDancerBody(&painter, &*dancer, MARGIN + roleWidth + symbolWidth/2, Y + rowHeight/2); 
            Y+=rowHeight;
        }
    }
    drawFooterHeaderBoxes(painter);
}

void drawInfo(QPainter& painter, std::string header, std::string text, int Y) {
    int textFieldHeight = 100;
    int pageWidth = painter.device()->width();
    painter.setFont(QFont("Arial", 15, QFont::Bold));
    painter.drawText(
            QRect(MARGIN, Y, pageWidth -2*MARGIN, textFieldHeight),
        Qt::AlignHCenter | Qt::AlignVCenter,
        header.c_str()
    );
    painter.setFont(QFont("Arial", 15));
    painter.drawText(
            QRect(MARGIN, Y + textFieldHeight, pageWidth -2*MARGIN, textFieldHeight),
        Qt::AlignHCenter | Qt::AlignVCenter,
        text.c_str()
    );
}

void drawTitlePage(QPainter& painter, Choreo& choreo) {
    int pageWidth = painter.device()->width();
    int pageHeight = painter.device()->height();
    QColor choreoGreen(GREEN);
    painter.setPen(QColor(GREEN));

    painter.setFont(QFont("Arial", 25, QFont::Bold));
    painter.drawText(
        QRect(MARGIN, 300, pageWidth -2*MARGIN, 100),
        Qt::AlignHCenter | Qt::AlignVCenter,
        choreo.name.c_str()
    );

    drawInfo(painter, "Variante der Choreo", choreo.variation, 1900);
    drawInfo(painter, "Beschreibung:", choreo.description, 2400);

    drawFooterHeaderBoxes(painter);

    painter.setPen(QColor("white"));
    painter.setFont(QFont("Arial", 12));
    painter.drawText(
            QRect(MARGIN, pageHeight-FOOTER_HEIGHT, pageWidth-2*MARGIN, FOOTER_HEIGHT),
        Qt::AlignHCenter | Qt::AlignVCenter,
        ("Choreo zuletzt geändert am " + toGermanDate(choreo.lastSaveDate)).c_str()
    );
}

void generatePDF(Choreo &choreo, std::string pdfName, bool topUp, int dpi) {
    QPdfWriter writer(pdfName.c_str());
    writer.setPageSize(QPageSize::A4);
    writer.setResolution(dpi);
    writer.setPageMargins(QMarginsF(0, 0, 0, 0)); // No margins
    QPainter painter(&writer);

    drawTitlePage(painter, choreo);
    writer.newPage();
    drawTeamList(painter, choreo.dancers, choreo.roles);

    int currPage = 1,
        totalPages = choreo.scenes.size();
    for (Scene scene : choreo.scenes) {
        writer.newPage();
        drawFooterHeader(painter, currPage, totalPages, choreo.name);
        drawTitle(painter, scene.name, currPage, totalPages);
        drawTextBox(painter, QString::fromStdString(scene.text));
        renderSceneToPdf(painter, scene, choreo.floor, QPointF(MARGIN - BORDER, 300), 2, topUp);

        drawSidePanel(painter, scene, choreo.roles);
        currPage++;
    }
    painter.end();
}
