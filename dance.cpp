#include "dance.h"
#include "nlohmann/detail/meta/cpp_future.hpp"
#include "utils.h"
#include "config.h"
#include <iostream>
#include "nlohmann/json.hpp"
#include <QPainter>
#include <fstream>
#include <ostream>
#include <string>


using json = nlohmann::json;

float Floor::px_m = 100.;

QImage Floor::getBlankImage() {
    QImage image(getImWidth(), getImHeight(), QImage::Format_ARGB32);
    image.fill(Qt::white);
    return image;
}

Floor::Floor(json j) {
    j.get_to(*this);
}

void Floor::setXYOffset(int x, int y) {
    this->xOffset = x;
    this->yOffset = y;
}

void Floor::draw(QPainter& painter, bool topUp) const {
    QColor borderColor(GREEN),
           fillColor(GRAY),
           gridColor("#a9a9a9");
    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(fillColor));
    painter.drawRect(xPos_to_px(0) , yPos_to_px(0),
            m_to_px(getWidth()), m_to_px(getHeight()));
    painter.setPen(QPen(gridColor, 2));
    for (int x = xPos_to_px(1); x < xPos_to_px(getWidth()); x += m_to_px(1)) {
        if (x == xPos_to_px(getWidth()/2.))
            continue;
        painter.drawLine(x, yPos_to_px(0), x, yPos_to_px(getHeight()));
    }
    for (int y = yPos_to_px(1); y < yPos_to_px(getHeight()); y += m_to_px(1)) {
        if (y == yPos_to_px(getHeight()/2.))
            continue;
        painter.drawLine(xPos_to_px(0), y, xPos_to_px(getWidth()), y);
    }
    painter.setPen(QPen(borderColor, 5));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(xPos_to_px(0), yPos_to_px(0), m_to_px(getWidth()), m_to_px(getHeight()));
    painter.drawLine(xPos_to_px(0), yPos_to_px(getHeight()/2.), xPos_to_px(getWidth()), yPos_to_px(getHeight()/2.));
    painter.drawLine(xPos_to_px(getWidth()/2.), yPos_to_px(0), xPos_to_px(getWidth()/2.), yPos_to_px(getHeight()));

    QFont voHiFont = painter.font();
    voHiFont.setPixelSize(px_m*.45);
    painter.setPen(QPen(gridColor));
    painter.setFont(voHiFont);
    if (topUp) {
        drawTopLabel(painter, "Vorne");
        drawBottomLabel(painter, "Hinten");
    }
    else {
        drawTopLabel(painter, "Hinten");
        drawBottomLabel(painter, "Vorne");
    }
}

void Floor::drawTopLabel(QPainter& painter, std::string label) const {
    painter.drawText(
            QRect(xPos_to_px(0), yOffset, m_to_px(getWidth()), BORDER),
            Qt::AlignHCenter | Qt::AlignVCenter,
            label.c_str()
            );
}

void Floor::drawBottomLabel(QPainter& painter, std::string label) const {
    painter.drawText(
            QRect(xPos_to_px(0), yPos_to_px(getHeight()), m_to_px(getWidth()), BORDER),
            Qt::AlignHCenter | Qt::AlignVCenter,
            label.c_str()
            );

}

Role::Role(json j) {
    id = std::stoi(j["$id"].get<std::string>());
    zIndex = j["ZIndex"];
    name = j["Name"];
    color = j["Color"];
}

void to_json(json& j, const Role& role) {
    j = {
        {"$id", std::to_string(role.id)},
        {"ZIndex", role.zIndex},
        {"Name", role.name},
        {"Color", role.color}
    };
}

std::ostream& operator<<(std::ostream& os, const Role& role) {
    json j = {};
    return os;
}

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

int Dancer::diameter = 100;

void Dancer::draw(QPainter& painter, int x, int y) {
    QFont dancerFont = painter.font();
    dancerFont.setPixelSize(diameter*.4);
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

void to_json(json& j, const Dancer& dancer) {
    j = {
        {"$id", std::to_string(dancer.id)},
        {"Role", {{"$ref", std::to_string(dancer.role->id)}}},
        {"Name", dancer.name},
        {"Shortcut", dancer.shortcut},
        {"Color", dancer.color}
    };
}


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

void Position::draw(QPainter& painter, Floor floor, bool topUp) const {
    int x, y;
    if (topUp) {
        y = floor.yPos_to_px(floor.SizeBack - this->y);
        x = floor.xPos_to_px(floor.SizeLeft + this->x);
    }
    else {
        y = floor.yPos_to_px(floor.SizeBack + this->y);
        x = floor.xPos_to_px(floor.SizeLeft - this->x);
    }
    this->dancer->draw(painter, x, y);

    QFont annotationFont = painter.font();
    annotationFont.setPixelSize(floor.px_m*.3);
    QFontMetrics fm(annotationFont);
    int annotationOffset = floor.px_m/10;

    painter.setPen(QPen(Qt::black));
    painter.setFont(annotationFont);
    if (this->y != 0) {
        QString text = QString::number(std::abs(this->y));
        int textWidth = fm.horizontalAdvance(text);
        int drawY = y - fm.height()/2 + fm.ascent();
        painter.drawText(floor.xPos_to_px(0) + annotationOffset, drawY, text);
        painter.drawText(floor.xPos_to_px(floor.getWidth()) - annotationOffset - textWidth, drawY, text);
    }
    if (this->x != 0) {
        QString text = QString::number(std::abs(this->x));
        int textWidth = fm.horizontalAdvance(text);
        int drawX = x - textWidth/2;
        painter.drawText(drawX, floor.yPos_to_px(0) + fm.ascent(), text);
        painter.drawText(drawX, floor.yPos_to_px(floor.getHeight()) - fm.descent(), text);
    }
}

void to_json(json& j, const Position& pos) {
    j = {
        {"Dancer", {{"$ref", std::to_string(pos.dancer->id)}}},
        {"X", pos.x},
        {"Y", pos.y}
    };
}

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

void Scene::draw(QPainter& painter, Floor& floor, int roleID, bool topUp) const {
    floor.draw(painter, topUp);
    for (const Position pos : this->positions) {
        if (pos.dancer->role->id != roleID) {
            pos.draw(painter, floor, topUp);
        }
    }
    for (const Position pos : this->positions) {
        if (pos.dancer->role->id == roleID) {
            pos.draw(painter, floor, topUp);
        }
    }
}

void to_json(json& j, const Scene& scene) {
    j = {
        {"Name", scene.name},
        {"Text", scene.text},
        {"FixedPositions", true},
        {"Positions", scene.positions}
    };
}


Settings::Settings(json j) {
    j.get_to(*this);
}


Choreo::Choreo(std::string filePath) {
    std::ifstream file(filePath);
    json data = json::parse(file);
    name = data["Name"];
    subtitle = data["Subtitle"];
    variation = data["Variation"];
    author = data["Author"];
    description = data["Description"];
    lastSaveDate = data["LastSaveDate"];
    floor = Floor{data["Floor"]};
    settings = Settings{data["Settings"]};
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

std::ostream& operator<<(std::ostream& os, const Choreo& choreo) {
        json j = {
        {"_Comment", "This file was created with ChoreoCompiler."},
        {"Name", choreo.name},
        {"Subtitle", choreo.subtitle},
        {"Variation", choreo.variation},
        {"Author", choreo.author},
        {"Description", choreo.description},
        {"LastSaveDate", choreo.lastSaveDate},
        {"Settings", choreo.settings},
        {"Floor", choreo.floor},
        {"Roles", choreo.roles},
        {"Dancers", choreo.dancers},
        {"Scenes", choreo.scenes}
    };
    os << j;
    return os;
}

