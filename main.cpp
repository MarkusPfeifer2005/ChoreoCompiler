#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <memory>
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
#include "third_party/json/single_include/nlohmann/json.hpp"
#include <QApplication>
#include <QImage>
#include <QPainter>
#include <QColor>
#include <QString>
#include <QColor>

#define BORDER 60
#define PX_M 100.

using json = nlohmann::json;
namespace fs = std::filesystem;

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

struct Floor {
    unsigned int sizeFront,
                 sizeBack,
                 sizeLeft,
                 sizeRight;
    Floor(json);
    unsigned int getHeight() const {return sizeFront + sizeBack;}
    unsigned int getWidth() const {return sizeLeft + sizeRight;}
    unsigned int getImWidth() const {return PX_M * getWidth() + 2*BORDER;}
    unsigned int getImHeight() const {return PX_M * getHeight() + 2*BORDER;}
    void draw(QImage&) const;
};

Floor::Floor(json j) : sizeFront(j["SizeFront"]),
                       sizeBack(j["SizeBack"]),
                       sizeLeft(j["SizeLeft"]),
                       sizeRight(j["SizeRight"]) {}

void Floor::draw(QImage& img) const {
    QPainter painter{&img}; 
    QColor borderColor("#3d7e2d"),
           fillColor("#f0f0f0"),
           gridColor("#a9a9a9");
    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(fillColor));
    painter.drawRect(BORDER, BORDER, getImWidth()-2*BORDER, getImHeight()-2*BORDER);
    painter.setPen(QPen(gridColor, 2));
    for (int x = BORDER + PX_M; x < getWidth()*PX_M; x += PX_M) {
        if (x == getImWidth()/2) {continue;}
        painter.drawLine(x, BORDER, x, getImHeight()-BORDER);
    }
    for (int y = BORDER + PX_M; y < getHeight()*PX_M; y += PX_M) {
        if (y == getImHeight()/2) {continue;}
        painter.drawLine(BORDER, y, getImWidth() - BORDER, y);
    }
    painter.setPen(QPen(borderColor, 5));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(BORDER, BORDER, getImWidth()-2*BORDER, getImHeight()-2*BORDER);
    painter.drawLine(BORDER, getImHeight()/2, getImWidth() - BORDER, getImHeight()/2);  // horizontal
    painter.drawLine(getImWidth()/2, BORDER, getImWidth()/2, getImHeight()-BORDER);  // vertical

    QFont voHiFont = painter.font();
    voHiFont.setPixelSize(PX_M*.45);
    painter.setPen(QPen(gridColor));
    painter.setFont(voHiFont);
    painter.drawText(
            QRect(0, 0, getImWidth(), BORDER),
            Qt::AlignHCenter | Qt::AlignVCenter,
            "Vorne"
            );
    painter.drawText(
            QRect(0, getImHeight() - BORDER, getImWidth(), BORDER),
            Qt::AlignHCenter | Qt::AlignVCenter,
            "Hinten"
            );
}

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
    void draw(QImage&, Floor) const ;
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

void Position::draw(QImage& img, Floor floor) const {
    QPainter painter(&img);
    QFont dancerFont = painter.font();
    dancerFont.setPixelSize(PX_M*.4);
    QFont annotationFont = painter.font();
    annotationFont.setPixelSize(PX_M*.3);
    QFontMetrics fm(annotationFont);
    int diameter = PX_M,
        annotationOffset = PX_M/10;

    QColor col(this->dancer->color.c_str());
    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(col));
    int x = BORDER + (floor.sizeLeft + this->x) * PX_M,
        y = BORDER + (floor.sizeBack - this->y) * PX_M;
    painter.drawEllipse(x - diameter/2., y - diameter/2,diameter,diameter);
    painter.setPen(QPen(getTextColor(col)));
    painter.setFont(dancerFont);
    painter.drawText(QRect(x - diameter/2., y - diameter/2, diameter, diameter),
            Qt::AlignCenter,
            this->dancer->shortcut.c_str());

    painter.setPen(QPen(Qt::black));
    painter.setFont(annotationFont);
    if (this->y != 0) {
        QString text = QString::number(std::abs(this->y));
        int textWidth = fm.horizontalAdvance(text);
        int drawY = y - fm.height()/2 + fm.ascent();
        painter.drawText(BORDER + annotationOffset, drawY, text);
        painter.drawText(floor.getImWidth() - BORDER - annotationOffset - textWidth, drawY, text);
    }
    if (this->x != 0) {
        QString text = QString::number(std::abs(this->x));
        int textWidth = fm.horizontalAdvance(text);
        int drawX = x - textWidth/2;
        painter.drawText(drawX, BORDER + fm.ascent(), text);
        painter.drawText(drawX, floor.getImHeight() - BORDER - fm.descent(), text);
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

std::string find_and_replace(std::string text, std::string from, std::string to) {
    size_t start_pos = 0;
    while ((start_pos = text.find(from, start_pos)) != std::string::npos) {
        text.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'y'
    }
    return text;
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
    for (const auto dancer : dancers) {
        std::string fileName = dancer->name;
        std::replace(fileName.begin(), fileName.end(), ' ', '_');
        fileName.erase(std::remove(fileName.begin(), fileName.end(), '/'), fileName.end());
        if (!fs::exists(fileName)) {
            fs::create_directory(fileName);
        }

        std::ofstream notes{fileName + "/" + fileName + ".txt"};
        notes << "#separator:tab\n";
        notes << "#html:true\n";
        notes << "#notetype Basic\n";

        for (const Scene scene : scenes) {
            QImage image(floor.getImWidth(), floor.getImHeight(), QImage::Format_ARGB32);
            image.fill(Qt::white);
            floor.draw(image);

            notes << scene.name << "\t\"";
            std::vector<std::unique_ptr<Position>> mlePositions, femPositions;
            for (const Position pos : scene.positions) {
                //pos.draw(image, floor);
                if (pos.dancer->role->name == "Herr") {
                    mlePositions.push_back(std::make_unique<Position>(pos));
                }
                else {
                    femPositions.push_back(std::make_unique<Position>(pos));
                }
                if (pos.dancer->id == dancer->id) {
                    notes << std::abs(pos.x) << "|" << std::abs(pos.y) << "<br>";
                }
            }
            if (dancer->role->name == "Herr") {
                for (const auto& pos : femPositions) {
                    pos->draw(image, floor);
                }
                for (const auto& pos : mlePositions) {
                    pos->draw(image, floor);
                }
            }
            else {
                for (const auto& pos : mlePositions) {
                    pos->draw(image, floor);
                }
                for (const auto& pos : femPositions) {
                    pos->draw(image, floor);
                }
            }

            std::string imageName = dancer->shortcut + scene.name + ".jpg";
            std::string escapedImageName = find_and_replace(imageName, "\"", "\"\"");
            notes << "<img src=\"\"" << imageName << "\"\"><br>";

            std::string text = find_and_replace(scene.text, "\r\n", "<br>");
            text = find_and_replace(text, "\"", "\"\"");
            notes << text << "\"\n";

            image.save((fileName + "/" + imageName).c_str(), "JPG");
        }
        notes.close();
    }

    return EXIT_SUCCESS;
}


