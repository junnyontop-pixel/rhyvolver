#pragma once

#include "game_types.h"
#include <string>
#include <vector>

bool LoadMapFromCsv(const std::string& path, std::vector<Note>& notes);
