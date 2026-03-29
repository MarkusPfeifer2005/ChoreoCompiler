#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <memory>
#include <ostream>
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
#include "nlohmann/json.hpp"
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
    Floor() = default;
    Floor(json);
    void loadJson(json);
    unsigned int getHeight() const {return sizeFront + sizeBack;}
    unsigned int getWidth() const {return sizeLeft + sizeRight;}
    unsigned int getImWidth() const {return PX_M * getWidth() + 2*BORDER;}
    unsigned int getImHeight() const {return PX_M * getHeight() + 2*BORDER;}
    void draw(QImage&) const;
};

Floor::Floor(json j) {
    loadJson(j);
}

void Floor::loadJson(json j) {
    this->sizeFront = j["SizeFront"];
    this->sizeBack = j["SizeBack"];
    this->sizeLeft = j["SizeLeft"];
    this->sizeRight = j["SizeRight"];
}

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

class Dancer {
   public:
    int id;
    std::shared_ptr<Role> role;
    std::string name,
                shortcut,
                color;
    Dancer(json, std::vector<std::shared_ptr<Role>>&);
    void draw(QImage&, int, int);
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

void Dancer::draw(QImage& img, int x, int y) {
    QPainter painter(&img);
    QFont dancerFont = painter.font();
    dancerFont.setPixelSize(PX_M*.4);
    int diameter = PX_M;
    QColor col(this->color.c_str());
    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(col));
    painter.drawEllipse(x - diameter/2., y - diameter/2,diameter,diameter);
    painter.setPen(QPen(getTextColor(col)));
    painter.setFont(dancerFont);
    painter.drawText(QRect(x - diameter/2., y - diameter/2, diameter, diameter),
            Qt::AlignCenter,
            this->shortcut.c_str());
}

class Position {
   public:
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
    int x = BORDER + (floor.sizeLeft + this->x) * PX_M,
        y = BORDER + (floor.sizeBack - this->y) * PX_M;
    this->dancer->draw(img, x, y);

    QPainter painter(&img);
    QFont annotationFont = painter.font();
    annotationFont.setPixelSize(PX_M*.3);
    QFontMetrics fm(annotationFont);
    int annotationOffset = PX_M/10;

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
    QImage render(Floor, int=0) const;
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

QImage Scene::render(Floor floor, int roleID) const {
    QImage image(floor.getImWidth(), floor.getImHeight(), QImage::Format_ARGB32);
    image.fill(Qt::white);
    floor.draw(image);

    for (const Position pos : this->positions) {
        if (pos.dancer->role->id != roleID) {
            pos.draw(image, floor);
        }
    }
    for (const Position pos : this->positions) {
        if (pos.dancer->role->id == roleID) {
            pos.draw(image, floor);
        }
    }
    return image;
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
    Settings() = default;
    void loadJson(json);
};

void Settings::loadJson(json j) {
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

Settings::Settings(json j) {
    loadJson(j);
}

std::string find_and_replace(std::string text, std::string from, std::string to) {
    size_t start_pos = 0;
    while ((start_pos = text.find(from, start_pos)) != std::string::npos) {
        text.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'y'
    }
    return text;
}


class Choreo {
public:
    Floor floor;
    Settings settings;
    std::vector<std::shared_ptr<Role>> roles;
    std::vector<std::shared_ptr<Dancer>> dancers;
    std::vector<Scene> scenes;
    Choreo(std::string);
};

Choreo::Choreo(std::string filePath) {
    std::ifstream file(filePath);
    json data = json::parse(file);
    floor.loadJson(data["Floor"]);
    settings.loadJson(data["Settings"]);
    for (const auto r : data["Roles"]) {
        roles.push_back(std::make_shared<Role>(r));
    }
    for (const auto d : data["Dancers"]) {
        dancers.push_back(std::make_shared<Dancer>(d, roles));
    }
    for (const auto s : data["Scenes"]) {
        scenes.push_back(Scene{s, dancers});
    }
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

    for (const Scene scene : choreo.scenes) {
        QImage image = scene.render(choreo.floor, dancerRoleID);

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

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Please provide a file path!" << std::endl;
        return EXIT_FAILURE;
    }
    QApplication app(argc, argv);
    if (strcmp(argv[1], "--list") == 0 || strcmp(argv[1], "-l") == 0) {
        Choreo choreo{argv[2]};
        for (auto dancer : choreo.dancers) {
            std::cout << dancer->name << "\n";
        }
    }
    else if (strcmp(argv[1], "--name") == 0 || strcmp(argv[1], "-n") == 0) {
        generateAnki(argv[3], argv[2]);
    }

    return EXIT_SUCCESS;
}


