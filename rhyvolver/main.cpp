#include "raylib.h"
#include "raymath.h"
#include "gameplay.h"
#include "map_loader.h"
#include "rendering.h"

#include <cmath>
#include <cstdio>
#include <vector>

int main() {
    int comboCount = 0;
    float recoilOffsetZ = 0.0f;
    float recoilRotation = 0.0f;
    Vector2 screenShake = { 0.0f, 0.0f };

    InitWindow(1280, 720, "Rhyvolver - Prototype");
    Font uiFont = LoadFont("assets/font/Orbitron-Medium.ttf");
    if (!IsFontValid(uiFont)) {
        printf("Failed to load assets/font/Orbitron-Medium.ttf\n");
        CloseWindow();
        return -1;
    }
    InitAudioDevice();
    SetTargetFPS(60);
    DisableCursor();

    Vector2 viewOffset = { 0.0f, 0.0f };
    int ammo = CHAMBER_COUNT;
    std::vector<Note> notes;
    std::vector<JudgeEffect> judgeEffects;
    if (!LoadMapFromCsv("assets/map/map.csv", notes)) {
        CloseAudioDevice();
        CloseWindow();
        return -1;
    }

    const float HIT_WINDOW_PERFECT = 0.08f;
    const float HIT_WINDOW_GOOD = 0.18f;
    const float AUTO_AIM_SPEED = 12.0f;

    Music music = LoadMusicStream("assets/virtual_love_2001.mp3");
    PlayMusicStream(music);

    Camera3D gunCam = { 0 };
    gunCam.position = { 0.0f, 0.0f, 3.0f };
    gunCam.target = { 0.0f, 0.0f, 0.0f };
    gunCam.up = { 0.0f, 1.0f, 0.0f };
    gunCam.fovy = 55.0f;
    gunCam.projection = CAMERA_PERSPECTIVE;

    Model revolver = LoadModel("assets/model/main.glb");
    Model woodGrip = LoadModel("assets/model/wood.glb");
    const Color revolverTint = { 255, 220, 160, 255 };
    SetMaterialTexture(
        &revolver.materials[0],
        MATERIAL_MAP_DIFFUSE,
        revolver.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture
    );

    const float gunScale = 0.5f;
    const float baseGunRotation = 90.0f;
    bool chamberLoaded[CHAMBER_COUNT] = { true, true, true, true, true, true };
    int nextChamber = 0;
    const Vector3 baseGunPos = { 0.6f, -0.4f, 1.8f };
    Vector2 aimPoint = {
        static_cast<float>(GetScreenWidth()) * 0.5f,
        static_cast<float>(GetScreenHeight()) * 0.5f
    };

    Sound shot = LoadSound("assets/sound/shot.wav");
    Sound reload = LoadSound("assets/sound/reload.wav");

    while (!WindowShouldClose()) {
        UpdateMusicStream(music);
        const float songTime = GetMusicTimePlayed(music);

        if (IsKeyPressed(KEY_R)) {
            ammo = CHAMBER_COUNT;
            for (int i = 0; i < CHAMBER_COUNT; i++) {
                chamberLoaded[i] = true;
            }
            PlaySound(reload);
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            nextChamber = (nextChamber + 1) % CHAMBER_COUNT;
        }
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            nextChamber = (nextChamber + CHAMBER_COUNT - 1) % CHAMBER_COUNT;
        }

        const float dt = GetFrameTime();
        recoilOffsetZ = Lerp(recoilOffsetZ, 0.0f, dt * 20.0f);
        recoilRotation = Lerp(recoilRotation, 0.0f, dt * 20.0f);
        screenShake.x = Lerp(screenShake.x, 0.0f, dt * 15.0f);
        screenShake.y = Lerp(screenShake.y, 0.0f, dt * 15.0f);

        UpdateJudgeEffects(judgeEffects, dt);
        MarkMissedNotes(notes, songTime, HIT_WINDOW_GOOD, judgeEffects, comboCount);

        const Vector2 screenCenter = {
            static_cast<float>(GetScreenWidth()) * 0.5f,
            static_cast<float>(GetScreenHeight()) * 0.5f
        };
        Note* autoTarget = FindAutoTarget(notes, songTime, HIT_WINDOW_GOOD);
        const Vector2 aimTarget = autoTarget
            ? (Vector2){ autoTarget->x + viewOffset.x, autoTarget->y + viewOffset.y }
            : screenCenter;
        const float aimPull = fminf(dt * AUTO_AIM_SPEED, 1.0f);
        aimPoint.x += (aimTarget.x - aimPoint.x) * aimPull;
        aimPoint.y += (aimTarget.y - aimPoint.y) * aimPull;

        if (IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_X)) {
            const int firedChamber = nextChamber;
            if (chamberLoaded[firedChamber]) {
                chamberLoaded[firedChamber] = false;
                ammo--;
                nextChamber = (nextChamber + 1) % CHAMBER_COUNT;
                recoilOffsetZ = 0.15f;
                recoilRotation = 18.0f;
                screenShake.y = 12.0f;
                screenShake.x = static_cast<float>(GetRandomValue(-5, 5));
                PlaySound(shot);

                JudgeShot(
                    autoTarget,
                    GetChamberColor(firedChamber),
                    songTime,
                    HIT_WINDOW_PERFECT,
                    HIT_WINDOW_GOOD,
                    comboCount,
                    judgeEffects
                );
            }
        }

        const Vector2 finalViewOffset = {
            viewOffset.x + screenShake.x,
            viewOffset.y + screenShake.y
        };
        const Vector3 currentGunPos = {
            baseGunPos.x,
            baseGunPos.y,
            baseGunPos.z + recoilOffsetZ
        };
        const float currentGunRotation = baseGunRotation + recoilRotation;

        BeginDrawing();
        ClearBackground((Color){ 68, 78, 98, 255 });
        DrawNotes(notes, songTime, viewOffset);
        DrawJudgeEffects(judgeEffects, uiFont, viewOffset);
        DrawWeapon(
            gunCam,
            revolver,
            woodGrip,
            currentGunPos,
            currentGunRotation,
            gunScale,
            revolverTint
        );
        DrawHud(uiFont, songTime, ammo, comboCount, aimPoint, chamberLoaded, nextChamber);
        EndDrawing();
    }

    UnloadModel(revolver);
    UnloadModel(woodGrip);
    UnloadMusicStream(music);
    UnloadSound(shot);
    UnloadSound(reload);
    UnloadFont(uiFont);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
