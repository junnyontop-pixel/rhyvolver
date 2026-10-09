#include "raylib.h"
#include "raymath.h"
#include <string>
#include <vector>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <stdexcept>

enum NoteType {
    normal_note,
    reload_note,
    long_note
};

enum class ColorMode {
    EasyRedBlue,
    HardMultiColor
};

static constexpr ColorMode CURRENT_COLOR_MODE = ColorMode::EasyRedBlue;

static Color GetNoteColor(Color mapColor, size_t noteIndex) {
    if (CURRENT_COLOR_MODE == ColorMode::EasyRedBlue) {
        return (noteIndex % 2 == 0) ? RED : BLUE;
    }

    return mapColor;
}

static Color GetChamberColor(int chamberIndex) {
    if (CURRENT_COLOR_MODE == ColorMode::EasyRedBlue) {
        return (chamberIndex % 2 == 0) ? RED : BLUE;
    }

    static const Color hardModeColors[] = { RED, GREEN, PURPLE, WHITE, YELLOW, BLUE };
    return hardModeColors[chamberIndex % 6];
}

static bool AreColorsEqual(Color first, Color second) {
    return first.r == second.r &&
        first.g == second.g &&
        first.b == second.b &&
        first.a == second.a;
}

struct Note {
    float x;
    float y;
    float radius;
    float hitTime;
    bool isAlive;
    NoteType type;
    Color color;
};

static std::string Trim(const std::string& value) {
    const size_t first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";

    const size_t last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

static bool TryParseColor(std::string name, Color& color) {
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

static bool TryParseFloat(const std::string& value, float& result) {
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

static bool TryParseInt(const std::string& value, int& result) {
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

// 판정 이펙트 구조체
struct JudgeEffect {
    Vector2 position;     
    const char* text;  
    Color color;        
    float lifetime;
    bool isAlive;    
};

std::vector<JudgeEffect> JudgeEffects;

int main() {
    // 판정 팝업 위한 변수
    const char* hitResult = "";
    Color resultColor = GRAY;

    // 총기 반동용 변수
    float recoilOffsetZ = 0.0f;
    float recoilRotation = 0.0f;

    // 화면 셰이크 제어 변수
    Vector2 screenShake = { 0.0f, 0.0f };

    // 창 생성 및 설정
    InitWindow(1280, 720, "Rhyvolver - Prototype");
    InitAudioDevice();
    SetTargetFPS(60);

    // 마우스 커서 숨김
    DisableCursor();

    // 에임 시야 오프셋과 감도
    Vector2 viewOffset = { 0.0f, 0.0f };

    // 게임 변수
    int ammo = 6;
    std::vector<Note> Notes;

    // csv 맵 파싱
    std::ifstream file("assets/map/map.csv");
    std::string line;
    int lineNumber = 0;

    if (!file.is_open()) {
        printf("Failed to open assets/map/map.csv\n");
        CloseAudioDevice();
        CloseWindow();
        return -1;
    }

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
            file.close();
            CloseAudioDevice();
            CloseWindow();
            return -1;
        }

        Note n;
        int type = 0;
        if (!TryParseFloat(fields[0], n.hitTime) ||
            !TryParseFloat(fields[1], n.x) ||
            !TryParseFloat(fields[2], n.y) ||
            !TryParseInt(fields[3], type) ||
            !TryParseFloat(fields[4], n.radius) ||
            !TryParseColor(fields[5], n.color) ||
            type < normal_note || type > long_note ||
            n.hitTime < 0.0f || n.radius <= 0.0f) {
            printf("Invalid map data at line %d\n", lineNumber);
            file.close();
            CloseAudioDevice();
            CloseWindow();
            return -1;
        }
        n.type = static_cast<NoteType>(type);
        n.color = GetNoteColor(n.color, Notes.size());
        
        n.isAlive = true;
        Notes.push_back(n);
    }

    file.close();

    // 판정 기준
    const float HIT_WINDOW_PERFECT = 0.08f;
    const float HIT_WINDOW_GOOD    = 0.18f;

    // 음악 재생
    Music music = LoadMusicStream("assets/virtual_love_2001.mp3");
    PlayMusicStream(music);

    // 총기 렌더링용 고정 3d 카메라
    Camera3D gunCam = { 0 };
    gunCam.position = (Vector3){ 0.0f, 0.0f, 3.0f };
    gunCam.target   = (Vector3){ 0.0f, 0.0f, 0.0f };
    gunCam.up       = (Vector3){ 0.0f, 1.0f, 0.0f };
    gunCam.fovy     = 55.0f;
    gunCam.projection = CAMERA_PERSPECTIVE;

    // 총기 모델 로드 및 배치 설정
    Model revolver = LoadModel("assets/revolver.glb");
    Model cylinder = LoadModel("assets/cylinder.glb");
    float gunScale = 0.5f;
    float baseGunRotation = 90.0f;
    float cylinderRotation = 0.0f;
    const float CYLINDER_STEP = 60.0f;
    const int CHAMBER_COUNT = 6;
    const float AUTO_AIM_SPEED = 12.0f;
    bool chamberLoaded[CHAMBER_COUNT] = { true, true, true, true, true, true };
    int nextChamber = 0;
    const Vector3 cylinderPivot = { 0.4931627f, 0.3096911f, 0.0008756f };
    Vector3 baseGunPos = { 0.6f, -0.4f, 1.8f };
    Vector2 aimPoint = {
        static_cast<float>(GetScreenWidth()) * 0.5f,
        static_cast<float>(GetScreenHeight()) * 0.5f
    };

    // 효과음 불러오기
    Sound shot = LoadSound("assets/sound/shot.wav");
    Sound reload = LoadSound("assets/sound/reload.wav");

    // 메인 게임 루프
    while (!WindowShouldClose()) {
        // 오디오 업데이트
        UpdateMusicStream(music);
        float songTime = GetMusicTimePlayed(music);

        // R 키로 재장전
        if (IsKeyPressed(KEY_R)) {
            ammo = 6;
            for (int i = 0; i < CHAMBER_COUNT; i++) {
                chamberLoaded[i] = true;
            }
            PlaySound(reload);
        }

        // 마우스 클릭으로 실린더를 한 칸 회전
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            cylinderRotation += CYLINDER_STEP;
            nextChamber = (nextChamber + 1) % CHAMBER_COUNT;
        }
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            cylinderRotation -= CYLINDER_STEP;
            nextChamber = (nextChamber + CHAMBER_COUNT - 1) % CHAMBER_COUNT;
        }

        float dt = GetFrameTime();

        // 반동 감쇄
        recoilOffsetZ = Lerp(recoilOffsetZ, 0.0f, dt * 20.0f);
        recoilRotation = Lerp(recoilRotation, 0.0f, dt * 20.0f);

        screenShake.x = Lerp(screenShake.x, 0.0f, dt * 15.0f);
        screenShake.y = Lerp(screenShake.y, 0.0f, dt * 15.0f);

        for (auto& eff : JudgeEffects) {
            eff.lifetime -= dt;
            if (eff.lifetime <= 0.0f) {
                eff.isAlive = false;
            }
        }

        JudgeEffects.erase(
            std::remove_if(JudgeEffects.begin(), JudgeEffects.end(), [](const JudgeEffect& eff) {
                return !eff.isAlive;
            }),
            JudgeEffects.end()
        );

        // 시간 지난 노트 처리
        for (auto& note : Notes) {
            if (!note.isAlive) continue;

            if (songTime - note.hitTime > HIT_WINDOW_GOOD) {
                printf("miss - passed\n");
                
                // 판정 팝업
                JudgeEffect eff;
                eff.position = { note.x, note.y }; 
                eff.text = "MISS";            
                eff.color = RED;
                eff.lifetime = 0.5f;            
                eff.isAlive = true;
                JudgeEffects.push_back(eff);

                note.isAlive = false;
            }
        }

        const Vector2 screenCenter = {
            static_cast<float>(GetScreenWidth()) * 0.5f,
            static_cast<float>(GetScreenHeight()) * 0.5f
        };
        Note* autoTarget = nullptr;
        for (auto& note : Notes) {
            if (!note.isAlive) continue;

            const float timeToHit = note.hitTime - songTime;
            if (timeToHit > 1.0f || timeToHit < -HIT_WINDOW_GOOD) continue;

            if (autoTarget == nullptr || note.hitTime < autoTarget->hitTime) {
                autoTarget = &note;
            }
        }

        const Vector2 aimTarget = autoTarget
            ? (Vector2){ autoTarget->x + viewOffset.x, autoTarget->y + viewOffset.y }
            : screenCenter;
        const float aimPull = fminf(dt * AUTO_AIM_SPEED, 1.0f);
        aimPoint.x += (aimTarget.x - aimPoint.x) * aimPull;
        aimPoint.y += (aimTarget.y - aimPoint.y) * aimPull;

        // 격발 및 판정
        if (IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_X)) {
            const int firedChamber = nextChamber;
            const Color shotColor = GetChamberColor(firedChamber);

            // 선택된 약실에 탄환이 있을 때만 격발한다.
            if (chamberLoaded[firedChamber]) {
                ammo--;
                chamberLoaded[firedChamber] = false;
                nextChamber = (nextChamber + 1) % CHAMBER_COUNT;
                cylinderRotation += CYLINDER_STEP;

                // 격발 시 반동 값 부여
                recoilOffsetZ = 0.15f;
                recoilRotation = 18.0f;

                // 격발 시 화면 반동
                screenShake.y = 12.0f;
                screenShake.x = (GetRandomValue(-5, 5));

                PlaySound(shot);

                if (autoTarget != nullptr) {
                    const float diff = fabs(songTime - autoTarget->hitTime);
                    const char* resultText = "MISS";
                    Color resultColor = RED;

                    if (!AreColorsEqual(shotColor, autoTarget->color)) {
                        printf("miss: color mismatch\n");
                        resultText = "WRONG COLOR";
                    } else if (diff <= HIT_WINDOW_PERFECT) {
                        printf("hit: perfect diff %.1f ms\n", diff * 1000.0f);
                        resultText = "PERFECT";
                        resultColor = GOLD;
                    } else if (diff <= HIT_WINDOW_GOOD) {
                        printf("hit: good diff %.1f ms\n", diff * 1000.0f);
                        resultText = "GOOD";
                        resultColor = GREEN;
                    } else {
                        printf("miss\n");
                    }

                    JudgeEffect eff;
                    eff.position = { autoTarget->x, autoTarget->y };
                    eff.text = resultText;
                    eff.color = resultColor;
                    eff.lifetime = 0.5f;
                    eff.isAlive = true;
                    JudgeEffects.push_back(eff);
                    autoTarget->isAlive = false;
                }
            }
        }

        Vector2 finalViewOffset = {
            viewOffset.x + screenShake.x,
            viewOffset.y + screenShake.y
        };

        // 화면 그리기
        BeginDrawing();
            ClearBackground((Color){ 40, 44, 56, 255 });

            // 2d 노트 및 수축 링 렌더링
            for (const auto& note : Notes) {
                float timeDiff = note.hitTime - songTime;
                float ringRadius = note.radius + note.radius * 1.5f * timeDiff;

                float drawX = note.x + viewOffset.x;
                float drawY = note.y + viewOffset.y;

                if (note.isAlive && timeDiff <= 1.0f && timeDiff >= -0.2f) {
                    DrawCircle(static_cast<int>(drawX), static_cast<int>(drawY), note.radius, note.color);
                    DrawCircleLines(static_cast<int>(drawX), static_cast<int>(drawY), note.radius, RAYWHITE);

                    if (timeDiff > 0.0f) {
                        DrawCircleLines(static_cast<int>(drawX), static_cast<int>(drawY), ringRadius, YELLOW);
                    }
                }
            }

            // 판정 이펙트 렌더링
            for (auto& eff : JudgeEffects) {
                int textWidth = MeasureText(eff.text, 30);
                int drawTextX = static_cast<int>(eff.position.x + viewOffset.x) - (textWidth / 2);
                int drawTextY = static_cast<int>(eff.position.y + viewOffset.y) - 40; 

                DrawText(eff.text, drawTextX, drawTextY, 30, eff.color);
            }

            // 반동이 적용된 리볼버 위치 및 회전 계산
            Vector3 currentGunPos = {
                baseGunPos.x,
                baseGunPos.y,
                baseGunPos.z + recoilOffsetZ
            };
            float currentGunRotation = baseGunRotation + recoilRotation;

            BeginMode3D(gunCam);
                Vector3 scaleVec = { gunScale, gunScale, gunScale };
                Quaternion gunOrientation = QuaternionFromAxisAngle(
                    (Vector3){ 0.0f, 1.0f, 0.0f },
                    currentGunRotation * DEG2RAD
                );
                Quaternion cylinderSpin = QuaternionFromAxisAngle(
                    (Vector3){ 1.0f, 0.0f, 0.0f },
                    cylinderRotation * DEG2RAD
                );
                Quaternion cylinderOrientation = QuaternionMultiply(gunOrientation, cylinderSpin);
                Vector3 cylinderAxis;
                float cylinderAngle;
                QuaternionToAxisAngle(cylinderOrientation, &cylinderAxis, &cylinderAngle);

                Vector3 scaledPivot = Vector3Multiply(cylinderPivot, scaleVec);
                Vector3 basePivotOffset = Vector3Transform(scaledPivot, QuaternionToMatrix(gunOrientation));
                Vector3 rotatedPivotOffset = Vector3Transform(scaledPivot, QuaternionToMatrix(cylinderOrientation));
                Vector3 cylinderPos = Vector3Add(
                    currentGunPos,
                    Vector3Subtract(basePivotOffset, rotatedPivotOffset)
                );

                DrawModelEx(revolver, currentGunPos, (Vector3){ 0.0f, 1.0f, 0.0f }, currentGunRotation, scaleVec, WHITE);
                DrawModelEx(cylinder, cylinderPos, cylinderAxis, cylinderAngle * RAD2DEG, scaleVec, WHITE);
            EndMode3D();

            // 상단 정보 표시
            DrawText("Rhyvolver", 40, 30, 24, WHITE);
            DrawText(TextFormat("Song Time: %.2f s", songTime), 40, 65, 20, GREEN);
            DrawText(TextFormat("AMMO: %d / 6", ammo), 40, 105, 26, (ammo > 0) ? WHITE : RED);

            if (ammo == 0) {
                DrawText("RELOAD REQUIRED!", 40, 170, 22, RED);
            }

            // 조준점과 통합된 6색 실린더 HUD
            const Vector2 hudCenter = aimPoint;
            const float hudOrbitRadius = 42.0f;
            const Color selectedChamberColor = chamberLoaded[nextChamber]
                ? GetChamberColor(nextChamber)
                : (Color){ 90, 90, 90, 255 };
            DrawCircleLines(
                static_cast<int>(hudCenter.x),
                static_cast<int>(hudCenter.y),
                5.0f,
                selectedChamberColor
            );
            DrawCircleV(hudCenter, 2.0f, RAYWHITE);

            for (int slot = 0; slot < CHAMBER_COUNT; slot++) {
                float angle = (-90.0f + slot * 60.0f) * DEG2RAD;
                Vector2 chamberPos = {
                    hudCenter.x + cosf(angle) * hudOrbitRadius,
                    hudCenter.y + sinf(angle) * hudOrbitRadius
                };
                int chamberIndex = (nextChamber + slot) % CHAMBER_COUNT;

                DrawCircleV(chamberPos, 12.0f, (Color){ 5, 8, 14, 210 });
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
                    DrawCircleV(chamberPos, 8.0f, (Color){ 45, 50, 60, 255 });
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

        EndDrawing();
    }

    // 자원 해제
    UnloadModel(revolver);
    UnloadModel(cylinder);
    UnloadMusicStream(music);
    UnloadSound(shot);
    UnloadSound(reload);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}