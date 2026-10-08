#include "raylib.h"
#include <string>
#include <vector>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>

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
};

// 판정 이펙트 구조체
struct JudgeEffect {
    Vector2 position;     
    const char* text;  
    Color color;        
    float lifetime;      
};

std::vector<JudgeEffect> JudgeEffects;

int main() {
    // 판정 팝업 위한 변수
    const char* hitResult = "";
    Color resultColor = GRAY;

    // 창 생성 및 설정
    InitWindow(1280, 720, "Rhyvolver - Prototype");
    InitAudioDevice();
    SetTargetFPS(60);

    // 마우스 커서 숨김
    DisableCursor();

    // 에임 시야 오프셋과 감도
    Vector2 viewOffset = { 0.0f, 0.0f };
    const float SENSITIVITY = 1.0f;

    // 게임 변수
    int ammo = 6;
    std::vector<Note> Notes;

    // csv 맵 파싱
    std::ifstream file("assets/map/map.csv");
    std::string line;

    if (!file.is_open()) return -1;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::stringstream ss(line);
        std::string token;
        Note n;

        std::getline(ss, token, ','); n.hitTime = std::stof(token);
        std::getline(ss, token, ','); n.x       = std::stof(token);
        std::getline(ss, token, ','); n.y       = std::stof(token);
        std::getline(ss, token, ','); n.type    = static_cast<NoteType>(std::stoi(token));
        std::getline(ss, token, ','); n.radius  = std::stof(token);
        
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

    // 모델 로드 및 배치 설정
    Model revolver = LoadModel("assets/revolver.glb");
    float gunScale = 0.5f;
    float gunRotation = 90.0f;
    Vector3 gunPos = { 0.6f, -0.4f, 1.8f };

    // 메인 게임 루프
    while (!WindowShouldClose()) {
        // 오디오 업데이트
        UpdateMusicStream(music);
        float songTime = GetMusicTimePlayed(music);

        // 마우스 이동으로 시야 좌표 갱신
        Vector2 mouseDelta = GetMouseDelta();
        viewOffset.x -= mouseDelta.x * SENSITIVITY;
        viewOffset.y -= mouseDelta.y * SENSITIVITY;

        // 마우스 우클릭 재장전
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            ammo = 6;
        }

        // 시간 지난 노트 처리
        for (auto& note : Notes) {
            if (!note.isAlive) continue;

            if (songTime - note.hitTime > HIT_WINDOW_GOOD) {
                printf("miss - passed\n");
                hitResult = "MISS";
                resultColor = RED;
                note.isAlive = false;
            }
        }

        // 격발 및 판정
        Vector2 screenCenter = { 1280.0f * 0.5f, 720.0f * 0.5f };

        if (IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_X)) {
            if (ammo > 0) {
                ammo--;

                for (auto& note : Notes) {
                    if (!note.isAlive) continue;

                    Vector2 renderedNotePos = { note.x + viewOffset.x, note.y + viewOffset.y };

                    if (CheckCollisionPointCircle(screenCenter, renderedNotePos, note.radius)) {
                        float diff = fabs(songTime - note.hitTime);
                        if (diff <= HIT_WINDOW_PERFECT) {
                            printf("hit: perfect diff %.1f ms\n", diff * 1000.0f);
                            hitResult = "PERFECT";
                            resultColor = GOLD;
                        } else if (diff <= HIT_WINDOW_GOOD) {
                            printf("hit: good diff %.1f ms\n", diff * 1000.0f);
                            hitResult = "GOOD";
                            resultColor = GREEN;
                        } else {
                            printf("miss\n");
                            hitResult = "MISS";
                            resultColor = RED;
                        }

                        // 판정 팝업
                        JudgeEffect eff;
                        eff.position = { note.x, note.y }; 
                        eff.text = hitResult;            
                        eff.color = resultColor;
                        eff.lifetime = 0.5f;            
                        JudgeEffects.push_back(eff);

                        note.isAlive = false;
                        break;
                    }
                }
            }
        }

        // 화면 그리기
        BeginDrawing();
            ClearBackground(BLACK);

            // 2d 노트 및 수축 링 렌더링
            for (const auto& note : Notes) {
                float timeDiff = note.hitTime - songTime;
                float ringRadius = note.radius + note.radius * 1.5f * timeDiff;

                float drawX = note.x + viewOffset.x;
                float drawY = note.y + viewOffset.y;

                if (note.isAlive && timeDiff <= 1.0f && timeDiff >= -0.2f) {
                    DrawCircle(static_cast<int>(drawX), static_cast<int>(drawY), note.radius, RED);
                    DrawCircleLines(static_cast<int>(drawX), static_cast<int>(drawY), note.radius, WHITE);

                    if (timeDiff > 0.0f) {
                        DrawCircleLines(static_cast<int>(drawX), static_cast<int>(drawY), ringRadius, YELLOW);
                    }
                }
            }

            // 판정 이펙트 렌더링
            for (const auto& eff : JudgeEffects) {
                int textWidth = MeasureText(eff.text, 30);
                int drawTextX = static_cast<int>(eff.position.x + viewOffset.x) - (textWidth / 2);
                int drawTextY = static_cast<int>(eff.position.y + viewOffset.y) - 40; 

                DrawText(eff.text, drawTextX, drawTextY, 30, eff.color);
            }

            // 우측 하단 리볼버 모델 렌더링
            BeginMode3D(gunCam);
                Vector3 scaleVec = { gunScale, gunScale, gunScale };
                DrawModelEx(revolver, gunPos, (Vector3){ 0.0f, 1.0f, 0.0f }, gunRotation, scaleVec, WHITE);
            EndMode3D();

            // 상단 정보 표시
            DrawText("Rhyvolver", 40, 30, 24, WHITE);
            DrawText(TextFormat("Song Time: %.2f s", songTime), 40, 65, 20, GREEN);
            DrawText(TextFormat("AMMO: %d / 6", ammo), 40, 105, 26, (ammo > 0) ? WHITE : RED);

            if (ammo == 0) {
                DrawText("RELOAD REQUIRED!", 40, 170, 22, RED);
            }

            // 화면 중앙 고정 조준선
            int cx = 1280 / 2;
            int cy = 720 / 2;
            DrawCircleLines(cx, cy, 8, GREEN);
            DrawLine(cx - 15, cy, cx - 4, cy, GREEN);
            DrawLine(cx + 4, cy, cx + 15, cy, GREEN);
            DrawLine(cx, cy - 15, cx, cy - 4, GREEN);
            DrawLine(cx, cy + 4, cx, cy + 15, GREEN);

        EndDrawing();
    }

    // 자원 해제
    UnloadModel(revolver);
    UnloadMusicStream(music);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}