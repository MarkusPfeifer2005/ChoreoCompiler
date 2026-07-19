#ifndef EXPORT_H
#define EXPORT_H

#include <string>
#include "dance.h"

void generateAnki(std::string, std::string);

void generatePDF(Choreo&, std::string, bool, int=300);

#endif
