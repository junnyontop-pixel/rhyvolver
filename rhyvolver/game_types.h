#pragma once

#include "raylib.h"

enum NoteType {
    normal_note,
    reload_note,
    long_note
};

struct Note {
    float x;
    float y;
    float radius;
    float hitTime;
    bool isAlive;
    NoteType type;
    Color color;
};

struct JudgeEffect {
    Vector2 position;
    const char* text;
    Color color;
    float lifetime;
    bool isAlive;
};

constexpr int CHAMBER_COUNT = 6;
