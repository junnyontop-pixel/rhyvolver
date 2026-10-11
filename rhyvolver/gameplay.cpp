#include "gameplay.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {

enum class ColorMode {
    EasyRedBlue,
    HardMultiColor
};

constexpr ColorMode CURRENT_COLOR_MODE = ColorMode::EasyRedBlue;

void AddJudgeEffect(
    std::vector<JudgeEffect>& effects,
    Vector2 position,
    const char* text,
    Color color
) {
    effects.push_back({ position, text, color, 0.5f, true });
}

}

Color GetNoteColor(Color mapColor, std::size_t noteIndex) {
    return mapColor;
}

Color GetChamberColor(int chamberIndex) {
    if (CURRENT_COLOR_MODE == ColorMode::EasyRedBlue) {
        return (chamberIndex % 2 == 0) ? RED : BLUE;
    }

    static const Color hardModeColors[] = { RED, GREEN, PURPLE, WHITE, YELLOW, BLUE };
    return hardModeColors[chamberIndex % CHAMBER_COUNT];
}

bool AreColorsEqual(Color first, Color second) {
    return first.r == second.r &&
        first.g == second.g &&
        first.b == second.b &&
        first.a == second.a;
}

void UpdateJudgeEffects(std::vector<JudgeEffect>& effects, float dt) {
    for (JudgeEffect& effect : effects) {
        effect.lifetime -= dt;
        if (effect.lifetime <= 0.0f) {
            effect.isAlive = false;
        }
    }

    effects.erase(
        std::remove_if(effects.begin(), effects.end(), [](const JudgeEffect& effect) {
            return !effect.isAlive;
        }),
        effects.end()
    );
}

void MarkMissedNotes(
    std::vector<Note>& notes,
    float songTime,
    float hitWindowGood,
    std::vector<JudgeEffect>& effects,
    int& comboCount
) {
    for (Note& note : notes) {
        if (!note.isAlive || songTime - note.hitTime <= hitWindowGood) continue;

        printf("miss - passed\n");
        AddJudgeEffect(effects, { note.x, note.y }, "MISS", RED);
        comboCount = 0;
        note.isAlive = false;
    }
}

Note* FindAutoTarget(std::vector<Note>& notes, float songTime, float hitWindowGood) {
    Note* target = nullptr;
    for (Note& note : notes) {
        if (!note.isAlive) continue;

        const float timeToHit = note.hitTime - songTime;
        if (timeToHit > 1.0f || timeToHit < -hitWindowGood) continue;

        if (target == nullptr || note.hitTime < target->hitTime) {
            target = &note;
        }
    }

    return target;
}

void JudgeShot(
    Note* target,
    Color shotColor,
    float songTime,
    float hitWindowPerfect,
    float hitWindowGood,
    int& comboCount,
    std::vector<JudgeEffect>& effects
) {
    if (target == nullptr) return;

    const float diff = fabs(songTime - target->hitTime);
    const char* resultText = "MISS";
    Color resultColor = RED;

    if (!AreColorsEqual(shotColor, target->color)) {
        printf("miss: color mismatch\n");
        resultText = "WRONG COLOR";
        comboCount = 0;
    } else if (diff <= hitWindowPerfect) {
        printf("hit: perfect diff %.1f ms\n", diff * 1000.0f);
        resultText = "PERFECT";
        resultColor = GOLD;
        comboCount += 1;
    } else if (diff <= hitWindowGood) {
        printf("hit: good diff %.1f ms\n", diff * 1000.0f);
        resultText = "GOOD";
        resultColor = GREEN;
        comboCount += 1;
    } else {
        printf("miss\n");
        comboCount = 0;
    }

    AddJudgeEffect(effects, { target->x, target->y }, resultText, resultColor);
    target->isAlive = false;
}
