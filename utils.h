#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <QColor>

std::string toGermanDate(const std::string& isoDate);

double calculateLuminance(const QColor &color);

Qt::GlobalColor getTextColor(const QColor &bgColor);

std::string find_and_replace(std::string text, std::string from, std::string to);

#endif
