#include "dance.h"
#include <iostream>
#include "nlohmann/json.hpp"
#include <QPainter>
#include <fstream>
#include <memory>
#include <ostream>
#include <string>


using json = nlohmann::json;


Floor::Floor(json j) {
    j.get_to(*this);
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

Position::Position(double x, double y, int dancerID, std::vector<std::shared_ptr<Dancer>>& dancers) :
    x(x), y(y) {
    for (const auto d: dancers) {
        if (d->id == dancerID) {
            this->dancer = d;
            break;
        }
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

Scene::Scene(std::vector<std::shared_ptr<Dancer>>& dancers) {
    name = "New Scene";
    text = "";

    double xDame = 3.5, xHerr = 3.5;
    for (auto& dancer : dancers) {
        if (dancer->role->id == 0) {
            positions.push_back(Position{xDame, -1., dancer->id, dancers});
            xDame -= 1.;
        } else {
            positions.push_back(Position{xHerr, 1., dancer->id, dancers});
            xHerr -= 1.;
        }
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

Choreo::Choreo() {
    floor = Floor();
    settings = Settings();
    roles.push_back(std::make_shared<Role>("Dame", 0, "#FFC71585", 0));
    roles.push_back(std::make_shared<Role>("Herr", 1, "#FF4169E1", 1));
    for (int i = 1; i <= 8; i++) {
        char letter = 'A' + (i - 1);
        std::string letterStr(1, letter);
        dancers.push_back(std::make_shared<Dancer>(letterStr, i+8, letterStr, roles[0]));
        dancers.push_back(std::make_shared<Dancer>(std::to_string(i), i, std::to_string(i), roles[1]));
    }
    scenes.push_back(Scene{dancers});
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

