#include "rendering.h"
#include "gameplay.h"

#include <cmath>

void DrawNotes(const std::vector<Note>& notes, float songTime, Vector2 viewOffset) {
    for (const Note& note : notes) {
        const float timeDiff = note.hitTime - songTime;
        const float ringRadius = note.radius + note.radius * 1.5f * timeDiff;
        const float drawX = note.x + viewOffset.x;
        const float drawY = note.y + viewOffset.y;

        if (note.isAlive && timeDiff <= 1.0f && timeDiff >= -0.2f) {
            DrawCircle(static_cast<int>(drawX), static_cast<int>(drawY), note.radius, note.color);
            DrawCircleLines(static_cast<int>(drawX), static_cast<int>(drawY), note.radius, RAYWHITE);

            if (timeDiff > 0.0f) {
                DrawCircleLines(static_cast<int>(drawX), static_cast<int>(drawY), ringRadius, YELLOW);
            }
        }
    }
}

void DrawJudgeEffects(const std::vector<JudgeEffect>& effects, Font uiFont, Vector2 viewOffset) {
    for (const JudgeEffect& effect : effects) {
        const float textWidth = MeasureTextEx(uiFont, effect.text, 30.0f, 1.0f).x;
        const float drawTextX = effect.position.x + viewOffset.x - (textWidth / 2.0f);
        const float drawTextY = effect.position.y + viewOffset.y - 40.0f;
        DrawTextEx(uiFont, effect.text, { drawTextX, drawTextY }, 30.0f, 1.0f, effect.color);
    }
}

void DrawWeapon(
    Camera3D camera,
    Model revolver,
    Model woodGrip,
    Vector3 position,
    float rotation,
    float scale,
    Color tint
) {
    const Vector3 scaleVector = { scale, scale, scale };
    BeginMode3D(camera);
    DrawModelEx(revolver, position, { 0.0f, 1.0f, 0.0f }, rotation, scaleVector, tint);
    DrawModelEx(woodGrip, position, { 0.0f, 1.0f, 0.0f }, rotation, scaleVector, WHITE);
    EndMode3D();
}

void DrawHud(
    Font uiFont,
    float songTime,
    int ammo,
    int comboCount,
    Vector2 aimPoint,
    const bool (&chamberLoaded)[CHAMBER_COUNT],
    int nextChamber
) {
    DrawTextEx(uiFont, "Rhyvolver", { 40.0f, 30.0f }, 24.0f, 1.0f, WHITE);
    DrawTextEx(uiFont, TextFormat("Song Time: %.2f s", songTime), { 40.0f, 65.0f }, 20.0f, 1.0f, GREEN);
    DrawTextEx(uiFont, TextFormat("AMMO: %d / 6", ammo), { 40.0f, 105.0f }, 26.0f, 1.0f, (ammo > 0) ? WHITE : RED);

    if (ammo == 0) {
        DrawTextEx(uiFont, "RELOAD REQUIRED!", { 40.0f, 170.0f }, 22.0f, 1.0f, RED);
    }

    DrawTextEx(uiFont, TextFormat("COMBO: %d", comboCount), { 40.0f, 140.0f }, 30.0f, 1.0f, YELLOW);

    const float hudOrbitRadius = 42.0f;
    const Color selectedChamberColor = chamberLoaded[nextChamber]
        ? GetChamberColor(nextChamber)
        : (Color){ 125, 130, 145, 255 };
    DrawCircleLines(
        static_cast<int>(aimPoint.x),
        static_cast<int>(aimPoint.y),
        5.0f,
        selectedChamberColor
    );
    DrawCircleV(aimPoint, 2.0f, RAYWHITE);

    for (int slot = 0; slot < CHAMBER_COUNT; slot++) {
        const float angle = (-90.0f + slot * 60.0f) * DEG2RAD;
        const Vector2 chamberPos = {
            aimPoint.x + cosf(angle) * hudOrbitRadius,
            aimPoint.y + sinf(angle) * hudOrbitRadius
        };
        const int chamberIndex = (nextChamber + slot) % CHAMBER_COUNT;

        DrawCircleV(chamberPos, 12.0f, (Color){ 30, 38, 52, 230 });
        DrawCircleLines(
            static_cast<int>(chamberPos.x),
            static_cast<int>(chamberPos.y),
            12.0f,
            chamberIndex == nextChamber ? GOLD : (Color){ 115, 125, 145, 255 }
        );
        if (chamberLoaded[chamberIndex]) {
            DrawCircleV(chamberPos, 8.0f, GetChamberColor(chamberIndex));
            DrawCircleV(chamberPos, 3.0f, (Color){ 245, 245, 245, 220 });
        } else {
            DrawCircleV(chamberPos, 8.0f, (Color){ 85, 95, 112, 255 });
            DrawLine(
                static_cast<int>(chamberPos.x - 4.0f),
                static_cast<int>(chamberPos.y - 4.0f),
                static_cast<int>(chamberPos.x + 4.0f),
                static_cast<int>(chamberPos.y + 4.0f),
                GRAY
            );
            DrawLine(
                static_cast<int>(chamberPos.x + 4.0f),
                static_cast<int>(chamberPos.y - 4.0f),
                static_cast<int>(chamberPos.x - 4.0f),
                static_cast<int>(chamberPos.y + 4.0f),
                GRAY
            );
        }
    }
}
