#include <cstdlib>
#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <vector>
#include "qapplication.h"
#include "qbrush.h"
#include "qcolor.h"
#include "qcoreapplication.h"
#include "qfont.h"
#include "qfontmetrics.h"
#include "qimage.h"
#include "qnamespace.h"
#include "qpaintdevice.h"
#include "qpen.h"
#include "third_party/json/single_include/nlohmann/json.hpp"
#include <QApplication>
#include <QImage>
#include <QPainter>
#include <QColor>
#include <QString>
#include <QColor>

using json = nlohmann::json;

struct Floor {
    unsigned int sizeFront,
                 sizeBack,
                 sizeLeft,
                 sizeRight;
    Floor(json);
    unsigned int getHeight() {return sizeFront + sizeBack;}
    unsigned int getWidth() {return sizeLeft + sizeRight;}
};

Floor::Floor(json j) : sizeFront(j["SizeFront"]),
                       sizeBack(j["SizeBack"]),
                       sizeLeft(j["SizeLeft"]),
                       sizeRight(j["SizeRight"]) {}

struct Role {
    int id,
        zIndex;
    std::string name,
                color;
   Role(json);
};

Role::Role(json j) {
    id = std::stoi(j["$id"].get<std::string>());
    zIndex = j["ZIndex"];
    name = j["Name"];
    color = j["Color"];
}

struct Dancer {
    int id;
    std::shared_ptr<Role> role;
    std::string name,
                shortcut,
                color;
    Dancer(json, std::vector<std::shared_ptr<Role>>&);
};

Dancer::Dancer(json j, std::vector<std::shared_ptr<Role>>& role_ptrs) {
    id = std::stoi(j["$id"].get<std::string>());
    name = j["Name"];
    shortcut = j["Shortcut"];
    color = j["Color"];
    int ref = std::stoi(j["Role"]["$ref"].get<std::string>());
    for (const auto& rptr : role_ptrs) {
        if (rptr->id == ref) {
            this->role = rptr;
            break;
        }
    }
}

struct Position {
    std::shared_ptr<Dancer> dancer;
    double x,
           y;
    Position(json, std::vector<std::shared_ptr<Dancer>>&);
};

Position::Position(json j, std::vector<std::shared_ptr<Dancer>>& dancers) {
    x = j["X"];
    y = j["Y"];
    int ref = std::stoi(j["Dancer"]["$ref"].get<std::string>());
    for (const auto d: dancers) {
        if (d->id == ref) {
            this->dancer = d;
            break;
        }
    }

}

struct Scene {
    std::vector<Position> positions;
    std::string name,
                text;
    Scene(json, std::vector<std::shared_ptr<Dancer>>&);
    void print();
};

Scene::Scene(json j, std::vector<std::shared_ptr<Dancer>>& dancers) {
    name = j["Name"];
    text = j["Text"];

    for (const auto position : j["Positions"]) {
        positions.push_back(Position{position, dancers});
    }
}

void Scene::print() {
    std::cout << this->name << '\n';
    std::cout << this->text << '\n';
    for (auto position : this->positions) {
        std::cout << position.dancer->name << ": " << position.x << "|" << position.y << '\n';
    }
    std::cout << std::endl;
}

struct Settings {
    long animationMilliseconds = 500;
    int frontPosition,
        dancerPosition,
        resolution;
    float transparency,
          dancerSize = .8f;
    bool positionsAtSide = true,
         gridLines = true,
         showTimestamps = false;
    std::string floorColor = "#FFF9F4D4";
    Settings(json);
};

Settings::Settings(json j) {
    animationMilliseconds = j["AnimationMilliseconds"];
    frontPosition = j["FrontPosition"];
    dancerPosition = j["DancerPosition"];
    resolution = j["Resolution"];
    transparency = j["Transparency"];
    positionsAtSide = j["PositionsAtSide"];
    gridLines = j["GridLines"];
    floorColor = j["FloorColor"];
    dancerSize = j["DancerSize"];
    showTimestamps = j["ShowTimestamps"];
}

// Function to calculate the relative luminance of a color
double calculateLuminance(const QColor &color) {
    double r = color.redF();
    double g = color.greenF();
    double b = color.blueF();

    // Apply gamma correction
    r = (r <= 0.03928) ? r / 12.92 : std::pow((r + 0.055) / 1.055, 2.4);
    g = (g <= 0.03928) ? g / 12.92 : std::pow((g + 0.055) / 1.055, 2.4);
    b = (b <= 0.03928) ? b / 12.92 : std::pow((b + 0.055) / 1.055, 2.4);

    // Calculate relative luminance
    return 0.2126 * r + 0.7152 * g + 0.0722 * b;
}

// Function to determine text color based on background color
Qt::GlobalColor getTextColor(const QColor &bgColor) {
    double luminance = calculateLuminance(bgColor);
    return (luminance > 0.5) ? Qt::black : Qt::white;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Please provide file path!" << std::endl;
        return EXIT_FAILURE;
    }

    std::ifstream f(argv[1]);
    json data = json::parse(f);
    Floor floor{data["Floor"]};
    Settings setting{data["Settings"]};

    std::vector<std::shared_ptr<Role>> roles;
    for (const auto r : data["Roles"]) {
        roles.push_back(std::make_shared<Role>(r));
    }

    std::vector<std::shared_ptr<Dancer>> dancers;
    for (const auto d : data["Dancers"]) {
        dancers.push_back(std::make_shared<Dancer>(d, roles));
    }

    std::vector<Scene> scenes;
    for (const auto s : data["Scenes"]) {
        scenes.push_back(Scene{s, dancers});
    }

    QApplication app(argc, argv);
    unsigned int border = 60;
    double px_m = 100;
    unsigned int imWidth = px_m * floor.getWidth() + 2*border,
                 imHeight = px_m * floor.getHeight() + 2*border;

    for (const Scene scene : scenes) {
        QImage image(imWidth, imHeight, QImage::Format_ARGB32);
        image.fill(Qt::white);

        QPainter painter(&image);
        // painter.setRenderHint(QPainter::Antialiasing);

        QColor borderColor("#3d7e2d"),
               fillColor("#f0f0f0"),
               lineColor("#a9a9a9");

        painter.setPen(Qt::NoPen);
        painter.setBrush(QBrush(fillColor));
        painter.drawRect(border, border, imWidth-2*border, imHeight-2*border);
        painter.setPen(QPen(lineColor, 2));
        for (int x = border + px_m; x < floor.getWidth()*px_m; x += px_m) {
            if (x == imWidth/2) {continue;}
            painter.drawLine(x, border, x, imHeight-border);
        }
        for (int y = border + px_m; y < floor.getHeight()*px_m; y += px_m) {
            if (y == imHeight/2) {continue;}
            painter.drawLine(border, y, imWidth - border, y);
        }
        painter.setPen(QPen(borderColor, 5));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(border, border, imWidth-2*border, imHeight-2*border);
        painter.drawLine(border, imHeight/2, imWidth - border, imHeight/2);  // horizontal
        painter.drawLine(imWidth/2, border, imWidth/2, imHeight-border);  // vertical

        QFont dancerFont = painter.font();
        dancerFont.setPixelSize(px_m*.4);
        QFont annotationFont = painter.font();
        annotationFont.setPixelSize(px_m*.3);
        QFontMetrics fm(annotationFont);
        int diameter = px_m,
            annotationOffset = px_m/10;
        for (const auto pos : scene.positions) {
            QColor col(pos.dancer->color.c_str());
            painter.setPen(Qt::NoPen);
            painter.setBrush(QBrush(col));
            int x = border + (floor.sizeLeft + pos.x) * px_m,
                y = border + (floor.sizeBack - pos.y) * px_m;
            painter.drawEllipse(x - diameter/2., y - diameter/2,diameter,diameter);
            painter.setPen(QPen(getTextColor(col)));
            painter.setFont(dancerFont);
            painter.drawText(QRect(x - diameter/2., y - diameter/2, diameter, diameter),
                    Qt::AlignCenter,
                    pos.dancer->shortcut.c_str());

            painter.setPen(QPen(Qt::black));
            painter.setFont(annotationFont);
            if (pos.y != 0) {
                QString text = QString::number(std::abs(pos.y));
                int textWidth = fm.horizontalAdvance(text);
                int drawY = y - fm.height()/2 + fm.ascent();
                painter.drawText(border + annotationOffset, drawY, text);
                painter.drawText(imWidth - border - annotationOffset - textWidth, drawY, text);
            }
            if (pos.x != 0) {
                QString text = QString::number(std::abs(pos.x));
                int textWidth = fm.horizontalAdvance(text);
                int drawX = x - textWidth/2;
                painter.drawText(drawX, border + fm.ascent(), text);
                painter.drawText(drawX, imHeight - border - fm.descent(), text);
            }
        }

        QFont voHiFont = painter.font();
        voHiFont.setPixelSize(px_m*.45);
        painter.setPen(QPen(lineColor));
        painter.setFont(voHiFont);
        painter.drawText(
                QRect(0, 0, imWidth, border),
                Qt::AlignHCenter | Qt::AlignVCenter,
                "Vorne"
                );
        painter.drawText(
                QRect(0, imHeight - border, imWidth, border),
                Qt::AlignHCenter | Qt::AlignVCenter,
                "Hinten"
                );

        painter.end();
        image.save((scene.name + ".jpg").c_str(), "JPG");
    }
    return EXIT_SUCCESS;
}
