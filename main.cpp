#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <memory>
#include <ostream>
#include <qcontainerfwd.h>
#include <qpagesize.h>
#include <qpainter.h>
#include <qpdfwriter.h>
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
#include <QPdfWriter>
#include <QPageSize>

#define BORDER 60
#define PX_M 100.
#define MARGIN 120
#define HEADER_HEIGHT 40
#define FOOTER_HEIGHT 60

#define GREEN "#3d7e2d"
#define GRAY "#f0f0f0"

using json = nlohmann::json;
namespace fs = std::filesystem;


std::string toGermanDate(const std::string& isoDate) {
    // Extract YYYY-MM-DD
    std::string year = isoDate.substr(0, 4);
    std::string month = isoDate.substr(5, 2);
    std::string day = isoDate.substr(8, 2);

    // Combine in German format: DD.MM.YYYY
    return day + "." + month + "." + year;
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
    void draw(QImage&, bool=true) const;
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

void Floor::draw(QImage& img, bool topUp) const {
    QPainter painter{&img}; 
    QColor borderColor(GREEN),
           fillColor(GRAY),
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
    if (topUp) {
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
    else {
    painter.drawText(
            QRect(0, 0, getImWidth(), BORDER),
            Qt::AlignHCenter | Qt::AlignVCenter,
            "Hinten"
            );
    painter.drawText(
            QRect(0, getImHeight() - BORDER, getImWidth(), BORDER),
            Qt::AlignHCenter | Qt::AlignVCenter,
            "Vorne"
            );
    }
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
    void draw(QPainter&, int, int);
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

void Dancer::draw(QPainter& painter, int x, int y) {
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
    void draw(QImage&, Floor, bool=true) const ;
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

void Position::draw(QImage& img, Floor floor, bool topUp) const {
    int x, y;
    if (topUp) {
        y = BORDER + (floor.sizeBack - this->y) * PX_M;
        x = BORDER + (floor.sizeLeft + this->x) * PX_M;
    }
    else {
        y = BORDER + (floor.sizeBack + this->y) * PX_M;
        x = BORDER + (floor.sizeLeft - this->x) * PX_M;
    }
        
    QPainter painter(&img);

    this->dancer->draw(painter, x, y);

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
    QImage render(Floor, int=0, bool=true) const;
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

QImage Scene::render(Floor floor, int roleID, bool topUp) const {
    QImage image(floor.getImWidth(), floor.getImHeight(), QImage::Format_ARGB32);
    image.fill(Qt::white);
    floor.draw(image, topUp);

    for (const Position pos : this->positions) {
        if (pos.dancer->role->id != roleID) {
            pos.draw(image, floor, topUp);
        }
    }
    for (const Position pos : this->positions) {
        if (pos.dancer->role->id == roleID) {
            pos.draw(image, floor, topUp);
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

Choreo::Choreo(std::string filePath) {
    std::ifstream file(filePath);
    json data = json::parse(file);
    name = data["Name"];
    subtitle = data["Subtitle"];
    variation = data["Variation"];
    author = data["Author"];
    description = data["Description"];
    lastSaveDate = data["LastSaveDate"];
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
            dancer->draw(painter, MARGIN + roleWidth + symbolWidth/2, Y +rowHeight/2);
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
    else {
        bool topUp = false;
        std::string pdfName = "out.pdf";
        std::string choreoFileName = "";
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];
            if (arg == "--topUp") {
                topUp = true;
            }
            else if (arg.ends_with(".pdf")) {
                pdfName = arg;
            }
            else if (arg.ends_with(".choreo")) {
                choreoFileName = arg;
            }
        }
        Choreo choreo(choreoFileName);

        int dpi = 300;
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
            painter.drawImage(MARGIN-BORDER, 200, scene.render(choreo.floor, 2, topUp));
            drawSidePanel(painter, scene, choreo.roles);

            currPage++;
        }
        painter.end();
    }

    return EXIT_SUCCESS;
}

