#include "map_loader.h"
#include "gameplay.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::string Trim(const std::string& value) {
    const size_t first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";

    const size_t last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

bool TryParseColor(std::string name, Color& color) {
    name = Trim(name);
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });

    if (name == "RED") color = RED;
    else if (name == "YELLOW") color = YELLOW;
    else if (name == "PINK") color = PINK;
    else if (name == "WHITE") color = WHITE;
    else if (name == "BLUE") color = BLUE;
    else if (name == "BLACK") color = BLACK;
    else if (name == "GREEN") color = GREEN;
    else if (name == "PURPLE") color = PURPLE;
    else if (name == "ORANGE") color = ORANGE;
    else if (name == "GOLD") color = GOLD;
    else if (name == "LIME") color = LIME;
    else if (name == "SKYBLUE") color = SKYBLUE;
    else if (name == "VIOLET") color = VIOLET;
    else if (name == "MAROON") color = MAROON;
    else if (name == "BEIGE") color = BEIGE;
    else if (name == "BROWN") color = BROWN;
    else if (name == "GRAY") color = GRAY;
    else if (name == "LIGHTGRAY") color = LIGHTGRAY;
    else if (name == "DARKGRAY") color = DARKGRAY;
    else return false;

    return true;
}

bool TryParseFloat(const std::string& value, float& result) {
    try {
        size_t parsedLength = 0;
        const float parsed = std::stof(value, &parsedLength);
        if (parsedLength != value.size() || !std::isfinite(parsed)) return false;
        result = parsed;
        return true;
    } catch (const std::invalid_argument&) {
        return false;
    } catch (const std::out_of_range&) {
        return false;
    }
}

bool TryParseInt(const std::string& value, int& result) {
    try {
        size_t parsedLength = 0;
        const int parsed = std::stoi(value, &parsedLength);
        if (parsedLength != value.size()) return false;
        result = parsed;
        return true;
    } catch (const std::invalid_argument&) {
        return false;
    } catch (const std::out_of_range&) {
        return false;
    }
}

}

bool LoadMapFromCsv(const std::string& path, std::vector<Note>& notes) {
    std::ifstream file(path);
    if (!file.is_open()) {
        printf("Failed to open %s\n", path.c_str());
        return false;
    }

    std::string line;
    int lineNumber = 0;
    while (std::getline(file, line)) {
        lineNumber++;
        line = Trim(line);
        if (line.empty() || line[0] == '#') continue;

        std::stringstream ss(line);
        std::string token;
        std::vector<std::string> fields;
        while (std::getline(ss, token, ',')) {
            fields.push_back(Trim(token));
        }

        if (fields.size() != 6) {
            printf("Invalid map row at line %d: expected 6 columns, got %zu\n",
                lineNumber, fields.size());
            return false;
        }

        Note note;
        int type = 0;
        if (!TryParseFloat(fields[0], note.hitTime) ||
            !TryParseFloat(fields[1], note.x) ||
            !TryParseFloat(fields[2], note.y) ||
            !TryParseInt(fields[3], type) ||
            !TryParseFloat(fields[4], note.radius) ||
            !TryParseColor(fields[5], note.color) ||
            type < normal_note || type > long_note ||
            note.hitTime < 0.0f || note.radius <= 0.0f) {
            printf("Invalid map data at line %d\n", lineNumber);
            return false;
        }

        note.type = static_cast<NoteType>(type);
        note.color = GetNoteColor(note.color, notes.size());
        note.isAlive = true;
        notes.push_back(note);
    }

    return true;
}
