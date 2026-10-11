#pragma once

#include "game_types.h"
#include <vector>

void DrawNotes(const std::vector<Note>& notes, float songTime, Vector2 viewOffset);
void DrawJudgeEffects(const std::vector<JudgeEffect>& effects, Font uiFont, Vector2 viewOffset);
void DrawWeapon(
    Camera3D camera,
    Model revolver,
    Model woodGrip,
    Vector3 position,
    float rotation,
    float scale,
    Color tint
);
void DrawHud(
    Font uiFont,
    float songTime,
    int ammo,
    int comboCount,
    Vector2 aimPoint,
    const bool (&chamberLoaded)[CHAMBER_COUNT],
    int nextChamber
);
