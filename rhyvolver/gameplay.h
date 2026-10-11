#pragma once

#include "game_types.h"
#include <cstddef>
#include <vector>

Color GetNoteColor(Color mapColor, std::size_t noteIndex);
Color GetChamberColor(int chamberIndex);
bool AreColorsEqual(Color first, Color second);
void UpdateJudgeEffects(std::vector<JudgeEffect>& effects, float dt);
void MarkMissedNotes(
    std::vector<Note>& notes,
    float songTime,
    float hitWindowGood,
    std::vector<JudgeEffect>& effects,
    int& comboCount
);
Note* FindAutoTarget(std::vector<Note>& notes, float songTime, float hitWindowGood);
void JudgeShot(
    Note* target,
    Color shotColor,
    float songTime,
    float hitWindowPerfect,
    float hitWindowGood,
    int& comboCount,
    std::vector<JudgeEffect>& effects
);
