#include <cstdlib>
#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <vector>
#include "qapplication.h"
#include "qimage.h"
#include "qnamespace.h"
#include "qpen.h"
#include "third_party/json/single_include/nlohmann/json.hpp"
#include <QApplication>
#include <QImage>
#include <QPainter>
#include <QColor>

using json = nlohmann::json;

struct Floor {
    const int sizeFront,
        sizeBack,
        sizeLeft,
        sizeRight;
    Floor(json);
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

int main(int argc, char* argv[]) {
    std::ifstream f("../OneChance.choreo");
    json data = json::parse(f);
    Floor floor{data["Floor"]};

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

    Scene& scne = scenes[0];

    std::vector<double> x;
    std::vector<double> y;
    for (const auto pos : scne.positions) {
        x.push_back(pos.x);
        y.push_back(pos.y);
    }

    QApplication app(argc, argv);
    int width = 700, height = 700;
    QImage image(width, height, QImage::Format_ARGB32);
    image.fill(Qt::white);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.setPen(QPen(Qt::blue, 5));
    painter.setBrush(QBrush(Qt::red));
    painter.drawRect(100, 100, 400, 300);

    painter.end();

    image.save("rect.jpg", "JPG");

    return EXIT_SUCCESS;
}
