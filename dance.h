#ifndef DANCE_H
#define DANCE_H

#include "nlohmann/json.hpp"
#include <QImage>
#include "config.h"


using json = nlohmann::json;

class Floor {
  public:
    static float px_m;
    unsigned int sizeFront,
                 sizeBack,
                 sizeLeft,
                 sizeRight;
    Floor() = default;
    Floor(json);
    void loadJson(json);
    unsigned int getHeight() const {return sizeFront + sizeBack;}
    unsigned int getWidth() const {return sizeLeft + sizeRight;}
    unsigned int getImWidth() const {return px_m * getWidth() + 2*BORDER;}
    unsigned int getImHeight() const {return px_m * getHeight() + 2*BORDER;}
    void draw(QPainter&, bool=true) const;
    void setXYOffset(int, int);
    QImage getBlankImage();
    int xOffset=0,
        yOffset=0;
    float xPos_to_px(float meter) const {return xOffset + BORDER + meter*px_m;}
    float yPos_to_px(float meter) const {return yOffset + BORDER + meter*px_m;}
    float m_to_px(float meter) const {return meter*px_m;}
    void drawTopLabel(QPainter&, std::string) const;
    void drawBottomLabel(QPainter&, std::string) const;
};


struct Role {
    int id,
        zIndex;
    std::string name,
                color;
   Role(json);
};


class Dancer {
  public:
    int id;
    static int diameter;
    std::shared_ptr<Role> role;
    std::string name,
                shortcut,
                color;
    Dancer(json, std::vector<std::shared_ptr<Role>>&);
    void draw(QPainter&, int, int);
};


class Position {
   public:
    std::shared_ptr<Dancer> dancer;
    double x,
           y;
    Position(json, std::vector<std::shared_ptr<Dancer>>&);
    void draw(QPainter&, Floor, bool=true) const ;
};


struct Scene {
    std::vector<Position> positions;
    std::string name,
                text;
    Scene(json, std::vector<std::shared_ptr<Dancer>>&);
    void print();
    void draw(QPainter&, Floor&, int=0, bool=true) const;
};


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


class Choreo {
public:
    Floor floor;
    Settings settings;
    std::string name;
    std::string subtitle,
                variation,
                author,
                description,
                lastSaveDate;
    std::vector<std::shared_ptr<Role>> roles;
    std::vector<std::shared_ptr<Dancer>> dancers;
    std::vector<Scene> scenes;
    Choreo(std::string);
};

#endif
