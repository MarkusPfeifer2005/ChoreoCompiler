#ifndef DANCE_H
#define DANCE_H

#include "build/_deps/json-src/include/nlohmann/detail/macro_scope.hpp"
#include "nlohmann/json.hpp"


using json = nlohmann::json;


namespace nlohmann {
    // allowing to convert vectors of shared pointers to json
    template <typename T>
    struct adl_serializer<std::shared_ptr<T>> {
        static void to_json(json& j, const std::shared_ptr<T>& ptr) {
            if (ptr) {
                j = *ptr;      // dereference, then use T's own to_json
            } else {
                j = nullptr;
            }
        }

        static void from_json(const json& j, std::shared_ptr<T>& ptr) {
            if (j.is_null()) {
                ptr = nullptr;
            } else {
                ptr = std::make_shared<T>(j.get<T>());
            }
        }
    };
}

class Floor {
public:
    unsigned int SizeFront = 8,
                 SizeBack = 8,
                 SizeLeft = 8,
                 SizeRight = 8;
    Floor() = default;
    Floor(json);
    unsigned int getHeight() const {return SizeFront + SizeBack;}
    unsigned int getWidth() const {return SizeLeft + SizeRight;}
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Floor,
        SizeFront,
        SizeBack,
        SizeLeft,
        SizeRight)
};


struct Role {
    int id,
        zIndex;
    std::string name,
                color;
   Role(json);
};

void to_json(json&, const Role&);


class Dancer {
public:
    int id;
    std::shared_ptr<Role> role;
    std::string name,
                shortcut,
                color;
    Dancer(json, std::vector<std::shared_ptr<Role>>&);
};

void to_json(json&, const Dancer&);


class Position {
public:
    std::shared_ptr<Dancer> dancer;
    double x,
           y;
    Position(json, std::vector<std::shared_ptr<Dancer>>&);
};

void to_json(json&, const Position&);


struct Scene {
    std::vector<Position> positions;
    std::string name,
                text;
    Scene(json, std::vector<std::shared_ptr<Dancer>>&);
    void print();
};

void to_json(json&, const Scene&);


struct Settings {
    long AnimationMilliseconds = 500;
    int FrontPosition = 0,
        DancerPosition = 0,
        Resolution = 4;
    double Transparency = 0.,
           DancerSize = .8;
    bool PositionsAtSide = true,
         GridLines = true,
         ShowTimestamps = false;
    std::string FloorColor = "#FFF9F4D4";
    Settings(json);
    Settings() = default;
NLOHMANN_DEFINE_TYPE_INTRUSIVE(Settings,
    AnimationMilliseconds,
    FrontPosition,
    Resolution,
    Transparency,
    DancerSize,
    PositionsAtSide,
    GridLines,
    ShowTimestamps,
    FloorColor);
};


class Choreo {
public:
    Floor floor;
    Settings settings;
    std::string name = "Untitled",
                subtitle = "",
                variation = "",
                author = "",
                description = "",
                lastSaveDate = "";
    std::vector<std::shared_ptr<Role>> roles;
    std::vector<std::shared_ptr<Dancer>> dancers;
    std::vector<Scene> scenes;
    Choreo(std::string);
    Choreo() = default;
    friend std::ostream& operator<<(std::ostream&, const Choreo&);
};

#endif
