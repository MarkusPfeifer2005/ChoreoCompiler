#include "utils.h"


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

std::string find_and_replace(std::string text, std::string from, std::string to) {
    size_t start_pos = 0;
    while ((start_pos = text.find(from, start_pos)) != std::string::npos) {
        text.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'y'
    }
    return text;
}
