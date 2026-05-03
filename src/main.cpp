#include "raylib.h"
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>

typedef enum GameScreen { MENU, SKIN, GAMEPLAY, END } GameScreen;

enum WaveState { WAVE_ANNOUNCE, WAVE_ENTERING, WAVE_FIGHTING };

const int W = 1000;
const int H = 900;

struct Enemy {
    Vector2 pos;
    int tex_id;
    float shoot_timer;
    float shoot_cd;
    float move_timer;
    float move_cd;
    enum state {STAND, MOVE, MOVE_BACK} states; 
    bool active = true;
};

struct Bullet {
    Vector2 pos;
    bool active = true;
};

struct Particle {
    Vector2 pos;
    Vector2 vel;
    float life = 1.0f;      
    bool active = true;
};

void spawnExplosion(std::vector<Particle>& particles, Vector2 pos) {
    for (int i = 0; i < 24; i++) {
        Particle p;
        p.pos = pos;
        float angle = GetRandomValue(0, 360) * DEG2RAD;
        float speed = GetRandomValue(50, 250);
        p.vel = { cosf(angle) * speed, sinf(angle) * speed };
        p.life = 1.0f;
        particles.push_back(p);
    }
}

void Extend(Texture2D tex, float x, float y, float mul) {
    float mid_x = x + tex.width / 2.0f;
    float mid_y = y + tex.height / 2.0f;
    if (CheckCollisionPointRec(GetMousePosition(), {x, y, (float)tex.width, (float)tex.height})) {
        float new_width = tex.width * mul;
        float new_height = tex.height * mul;
        DrawTextureEx(tex, {mid_x - new_width * 0.5f, mid_y - new_height * 0.5f}, 0, mul, WHITE);
    }
    else DrawTextureEx(tex, {mid_x - tex.width * 0.5f, mid_y - tex.height * 0.5f}, 0, 1.0f, WHITE);
}

int main() {
    InitWindow(W, H, "My Game");
    InitAudioDevice();
    SetTargetFPS(60);

    std::vector<Texture2D> texPlayer;
    for (int i = 1; i <= 8; i++) {
        texPlayer.push_back(LoadTexture(("assets/Ships/spaceShips_00" + std::to_string(i) + ".png").c_str()));
    }

    std::vector<Texture2D> texEnemy;
    for (int i = 1; i <= 5; i++) {
        texEnemy.push_back(LoadTexture(("assets/Enemies/enemyBlack" + std::to_string(i) + ".png").c_str()));
    }
    for (int i = 1; i <= 5; i++) {
        texEnemy.push_back(LoadTexture(("assets/Enemies/enemyBlue" + std::to_string(i) + ".png").c_str()));
    }
    for (int i = 1; i <= 5; i++) {
        texEnemy.push_back(LoadTexture(("assets/Enemies/enemyGreen" + std::to_string(i) + ".png").c_str()));
    }
    for (int i = 1; i <= 5; i++) {
        texEnemy.push_back(LoadTexture(("assets/Enemies/enemyRed" + std::to_string(i) + ".png").c_str()));
    } 

    std::vector<Bullet> bullets;
    std::vector<Bullet> enemy_bullets;
    std::vector<Enemy> enemies;
    std::vector<Particle> particles;

    Texture2D texBackground_back = LoadTexture("assets/Backgrounds/bg_back.png");
    Texture2D texBackground_front = LoadTexture("assets/Backgrounds/bg_front.png");
    Texture2D texBackground_star = LoadTexture("assets/Backgrounds/Galaxy-Transparent-Background.png");
    Texture2D texLogo_space_shooter = LoadTexture("assets/Logo/s_spaceshooter_0.png");
    Texture2D texLogo_skin = LoadTexture("assets/Logo/Cool Text - CHOOSING SKIN 507067326165305.png");
    Texture2D texLogo_victory = LoadTexture("assets/Logo/s_victory_0.png");
    Texture2D texLogo_gameover = LoadTexture("assets/Logo/Cool Text - GAMEOVER 507198318820195.png");
    Texture2D texBullet = LoadTexture("assets/Lasers/tile003.png");
    Texture2D texBullet_e = LoadTexture("assets/Lasers/2.png");

    Image imButton_play = LoadImage("assets/UI_button/PlayButton.png");
    ImageResize(&imButton_play, imButton_play.width * 0.5f, imButton_play.height * 0.5f);
    Texture2D texButton_play = LoadTextureFromImage(imButton_play);
    UnloadImage(imButton_play);

    Image imButton_exit = LoadImage("assets/UI_button/ExitButton.png");
    ImageResize(&imButton_exit, imButton_exit.width * 0.5f, imButton_exit.height * 0.5f);
    Texture2D texButton_exit = LoadTextureFromImage(imButton_exit);
    UnloadImage(imButton_exit);

    Image imButton_menu = LoadImage("assets/UI_button/MenuButton.png");
    ImageResize(&imButton_menu, imButton_menu.width * 0.5f, imButton_menu.height * 0.5f);
    Texture2D texButton_menu = LoadTextureFromImage(imButton_menu);
    UnloadImage(imButton_menu);

    Music music_background = LoadMusicStream("assets/sound/sou_background.mp3");
    Music music_win = LoadMusicStream("assets/sound/win_sound.mp3");
    Music music_lose = LoadMusicStream("assets/sound/lose_sound.mp3");
    PlayMusicStream(music_background);
    Sound sou_explose = LoadSound("assets/sound/sou_explose.mp3");
    Sound sou_explose_1 = LoadSound("assets/sound/EnemyShoot.wav");
    Sound sou_laser = LoadSound("assets/sound/sou_laser.mp3");
    Sound sou_laser_enemy = LoadSound("assets/sound/EnemyShoot.wav");
    

    GameScreen CurrentScreen = MENU;

    WaveState waveState = WAVE_ANNOUNCE;

    float bgScroll_back = 0;
    float bgScroll_front = 0;
    float bgScroll_star = 0;
    float bgScroll_galaxy = 0;

    float player_speed = 400.0f;
    int skin_id = 0;
    float shoot_timer = 0;
    float shoot_cd = 0.5f;
    
    float enemy_speed = 400.0f;

    int wave = 0;
    bool next_wave = false;
    float waveTimer = 0;
    float announceTime = 2.0f;  
    float enterTime = 2.0f; 
    bool GameOver = false;
    float wait = 0;
    bool shouldExit = false;
    bool playingWin = false;
    bool playingLose = false;

    Vector2 player_pos;

    while (!WindowShouldClose() && !shouldExit) {
        float dt = GetFrameTime();

        UpdateMusicStream(music_background);
        UpdateMusicStream(music_win);
        UpdateMusicStream(music_lose);

        bgScroll_back += 100 * dt;
        if (bgScroll_back >= 512) bgScroll_back -= 512;
        bgScroll_front += 150 * dt;
        if (bgScroll_front >= 512) bgScroll_front -= 512;
        bgScroll_star += 200 * dt;
        if (bgScroll_star >= 512) bgScroll_star -= 512;

        switch (CurrentScreen)
        {
            case MENU:
            {
                
            }break;
            case SKIN:
            {

            }break;
            case GAMEPLAY:
            {
                waveTimer += dt;
                switch (waveState)
                {
                    case WAVE_ANNOUNCE:
                    {
                        if (waveTimer >= announceTime) {
                            waveTimer = 0;
                            float e_x = 100;
                            float e_y = -100;

                            if (wave <= 4) {
                                for (int i = 1; i <= 5; i++) {
                                    if (wave == 1 || wave == 3) enemies.push_back({{e_x, e_y}, i + (wave - 1) * 5, 0, GetRandomValue(200, 500) / 100.0f, 0, GetRandomValue(200, 500) / 100.0f, Enemy::STAND, true});
                                    else enemies.push_back({{e_x, e_y}, i + (wave - 1) * 5, 0, GetRandomValue(150, 300) / 100.0f, 0, GetRandomValue(150, 300) / 100.0f, Enemy::STAND, true});
                                    e_x += 180.0f;
                                }
                            }
                            else {
                                e_y = -250;
                                for (int i = 1; i <= 5; i++) {
                                    enemies.push_back({{e_x, e_y}, i + 5, 0, GetRandomValue(150, 300) / 100.0f, 0, GetRandomValue(150, 300) / 100.0f, Enemy::STAND, true});
                                    e_x += 180.0f;
                                }
                                e_x = 100;
                                e_y += 150;
                                for (int i = 1; i <= 5; i++) {
                                    enemies.push_back({{e_x, e_y}, i + 15, 0, GetRandomValue(150, 300) / 100.0f, 0, GetRandomValue(150, 300) / 100.0f, Enemy::STAND, true});
                                    e_x += 180.0f;
                                }
                            }
                            waveState = WAVE_ENTERING;
                        }

                    }break;
                    case WAVE_ENTERING:
                    {
                        if (wave <= 4) {
                            for (auto& e : enemies) {
                                e.pos.y += enemy_speed * dt;
                                if (e.pos.y > 150){
                                    e.pos.y = 150;
                                }  
                            }
                        }
                        else {
                            for (auto &e : enemies) {
                                e.pos.y += enemy_speed * dt;
                                if (e.tex_id <= 10) {
                                    if (e.pos.y > 100) {
                                        e.pos.y = 100;
                                    }
                                }
                                else {
                                    if (e.pos.y > 250) {
                                        e.pos.y = 250;
                                    }
                                }
                            }
                        }
                        if (waveTimer >= enterTime) {
                            waveTimer = 0;
                            waveState = WAVE_FIGHTING;
                        }

                    }break;
                    case WAVE_FIGHTING:
                    {
                        shoot_timer += dt;
                        if (GameOver) {
                            wait -= dt;
                            if (wait < 0) {
                                wait = 0;
                                CurrentScreen = END;
                                if (!playingLose) {
                                    StopMusicStream(music_background);
                                    PlayMusicStream(music_lose);
                                    playingLose = true;
                                }
                                continue;
                            }
                        }
                        else if (next_wave) {
                            wait -= dt;
                            if (wait < 0) {
                                wait = 0;
                                wave++;
                                if (wave > 5){
                                    CurrentScreen = END;
                                    if (!playingWin) {
                                        StopMusicStream(music_background);
                                        PlayMusicStream(music_win);
                                        playingWin = true;
                                    }
                                }
                                waveState = WAVE_ANNOUNCE;
                                player_pos = {W/2, 800};
                                next_wave = false;
                                waveTimer = 0;
                                enemy_bullets.clear();  
                                bullets.clear();       
                                continue;
                            }
                        }

                        if (wave == 1) {
                            enemy_speed = 400.0f;
                        }
                        else {
                            enemy_speed = 600.0f;
                        }

                        for (auto &e : enemies) {
                            if (!e.active) continue;
                            if ((wave > 2 && wave < 5) || (wave == 5 && e.tex_id <= 10)) {
                                e.shoot_timer += dt;
                                if (e.shoot_timer >= e.shoot_cd) {
                                    enemy_bullets.push_back({{e.pos.x + texEnemy[e.tex_id - 1].width * 0.5f, e.pos.y + texEnemy[e.tex_id - 1].height}, true});
                                    PlaySound(sou_laser_enemy);
                                    e.shoot_timer = 0;
                                    if (wave == 3) e.shoot_cd = GetRandomValue(300, 500) / 100.0f;
                                    else e.shoot_cd = GetRandomValue(150, 300) / 100.0f;
                                }
                            }
                            if (wave <= 2 || (wave == 5 && e.tex_id > 15)) {
                                switch (e.states)
                                {
                                    case Enemy::STAND:
                                    {
                                        e.move_timer += dt;
                                        if (e.move_timer >= e.move_cd) {
                                            e.move_timer = 0;
                                            e.states = Enemy::MOVE;
                                            if (wave == 3) e.move_cd = GetRandomValue(300, 500) / 100.0f;
                                            else e.move_cd = GetRandomValue(150, 300) / 100.0f; 
                                        }
                                    }break;
                                    case Enemy::MOVE:
                                    {
                                        e.pos.y += enemy_speed * dt;
                                        if (e.pos.y > 750) {
                                            e.pos.y = 750;
                                            e.states = Enemy::MOVE_BACK;
                                        }
                                    }break;
                                    case Enemy::MOVE_BACK:
                                    {
                                        e.pos.y -= enemy_speed * dt;
                                        if (wave == 5) {
                                            if (e.pos.y < 250){
                                                e.pos.y = 250;
                                                e.states = Enemy::STAND;
                                            }
                                        }
                                        else {
                                            if (e.pos.y < 150){
                                                e.pos.y = 150;
                                                e.states = Enemy::STAND;
                                            }
                                        }
                                    }break;
                                }
                                
                            }
                        }

                        if (!GameOver) {
                            if (IsKeyDown(KEY_UP) && player_pos.y - texPlayer[skin_id - 1].height * 0.5f > 30)
                                player_pos.y -= player_speed * dt;

                            if (IsKeyDown(KEY_DOWN) && player_pos.y + texPlayer[skin_id - 1].height * 0.5f < H - 30)
                                player_pos.y += player_speed * dt;

                            if (IsKeyDown(KEY_LEFT) && player_pos.x - texPlayer[skin_id - 1].width * 0.5f > 30)
                                player_pos.x -= player_speed * dt;

                            if (IsKeyDown(KEY_RIGHT) && player_pos.x + texPlayer[skin_id - 1].width * 0.5f < W - 30)
                                player_pos.x += player_speed * dt;

                            if (IsKeyPressed(KEY_SPACE) && shoot_timer >= shoot_cd) {
                                shoot_timer = 0;
                                bullets.push_back({player_pos, true});
                                PlaySound(sou_laser);
                            }
                        }

                        for (auto &it : bullets) { 
                            if (!it.active) continue;
                            it.pos.y -= 600*dt;
                            for (auto &e : enemies) {
                                if (!e.active) continue;
                                if (CheckCollisionRecs({it.pos.x - texBullet.width * 0.5f, it.pos.y - texBullet.height * 0.5f, (float)texBullet.width, (float)texBullet.height}, {e.pos.x, e.pos.y, (float)texEnemy[e.tex_id - 1].width, (float)texEnemy[e.tex_id - 1].height})) {
                                    it.active = false;
                                    e.active = false;
                                    spawnExplosion(particles, {e.pos.x + texEnemy[e.tex_id - 1].width * 0.5f, e.pos.y + texEnemy[e.tex_id - 1].height * 0.5f});
                                    PlaySound(sou_explose);
                                }
                            }
                            if (it.pos.y < -5) it.active = false;
                        }

                        for (auto &it : enemy_bullets) {
                            if (!it.active) continue;
                            if (wave == 3) it.pos.y += 400*dt;
                            else it.pos.y += 600*dt;
                            if (CheckCollisionRecs({it.pos.x - texBullet_e.width * 0.5f, it.pos.y - texBullet_e.height * 0.5f, (float)texBullet_e.width, (float)texBullet_e.height}, {player_pos.x - texPlayer[skin_id - 1].width * 0.5f, player_pos.y - texPlayer[skin_id - 1].height * 0.5f, (float)texPlayer[skin_id - 1].width, (float)texPlayer[skin_id - 1].height})) {
                                GameOver = true;
                                spawnExplosion(particles, player_pos);
                                PlaySound(sou_explose);
                                wait = 2.0f;
                            }
                            if (it.pos.y > H + 15) it.active = false;
                        }

                        for (auto &e : enemies) {
                            if (!e.active) continue;
                            if (CheckCollisionRecs({e.pos.x, e.pos.y, (float)texEnemy[e.tex_id - 1].width, (float)texEnemy[e.tex_id - 1].height}, {player_pos.x - texPlayer[skin_id - 1].width * 0.5f, player_pos.y - texPlayer[skin_id - 1].height * 0.5f, (float)texPlayer[skin_id - 1].width, (float)texPlayer[skin_id - 1].height})) {
                                GameOver = true;
                                spawnExplosion(particles, player_pos);
                                PlaySound(sou_explose_1);
                                spawnExplosion(particles, {e.pos.x + texEnemy[e.tex_id - 1].width * 0.5f, e.pos.y + texEnemy[e.tex_id - 1].height * 0.5f});
                                PlaySound(sou_explose);
                                wait = 2.0f;
                                e.active = false;
                            }
                        }

                        for (auto& p : particles) {
                            p.pos.x += p.vel.x * dt;
                            p.pos.y += p.vel.y * dt;
                            p.life  -= dt * 2.0f;
                            if (p.life <= 0) p.active = false;
                        }

                        auto inactive = [](const auto& x) { return !x.active; };
                        bullets.erase(std::remove_if(bullets.begin(), bullets.end(), inactive), bullets.end());
                        enemy_bullets.erase(std::remove_if(enemy_bullets.begin(), enemy_bullets.end(), inactive), enemy_bullets.end());
                        enemies.erase(std::remove_if(enemies.begin(), enemies.end(), inactive), enemies.end());
                        particles.erase(std::remove_if(particles.begin(), particles.end(), inactive), particles.end());

                        if (enemies.empty() && !next_wave && !GameOver) {
                            next_wave = true;
                            wait = 2.0f;
                        }

                    }break;
                }
                
            }break;
            case END:
            {

            }break;
            default: break;
        }

        BeginDrawing();
            ClearBackground(RAYWHITE);

            for (int y = (int)bgScroll_back - 512; y < H; y += 512) {
                for (int x = 0; x < W; x += 512) {
                    DrawTexture(texBackground_back, x, y, WHITE);
                }
            }

            for (int y = (int)bgScroll_front - 512; y < H; y += 512) {
                for (int x = 0; x < W; x += 512) {
                    DrawTexture(texBackground_front, x, y, WHITE);
                }
            }

            for (int y = (int)bgScroll_star - 512; y < H; y += 512) {
                for (int x = 0; x < W; x += 512) {
                    DrawTexture(texBackground_star, x, y, WHITE);
                }
            }

            switch (CurrentScreen)
            {
                case MENU:
                {
                    DrawTextureEx(texLogo_space_shooter, {W/7 - 20, H/5}, 0, 0.8f, WHITE);
                    Extend(texButton_play, W/2 - 142.5, H/2 + 20, 1.2f);
                    if (CheckCollisionPointRec(GetMousePosition(), {W/2 - 142.5, H/2 + 20, (float)texButton_play.width, (float)texButton_play.height})) {
                        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                            player_speed = 300.0f;
                            skin_id = 0;
                            shoot_timer = 0;
                            shoot_cd = 0.5f;
                            
                            enemy_speed = 300.0f;

                            wave = 0;
                            next_wave = false;
                            waveTimer = 0;
                            announceTime = 2.0f;  
                            enterTime = 2.0f; 
                            GameOver = false;
                            wait = 0;

                            bullets.clear();
                            enemy_bullets.clear();
                            enemies.clear();
                            particles.clear();

                            waveState = WAVE_ANNOUNCE;
                            CurrentScreen = SKIN;
                        }
                    }

                    Extend(texButton_exit, W/2 - 142.5, H/2 + 220, 1.2f);
                    if (CheckCollisionPointRec(GetMousePosition(), {W/2 - 142.5, H/2 + 220, (float)texButton_exit.width, (float)texButton_exit.height})) {
                        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                            shouldExit = true;
                        }
                    }

                }break;
                case SKIN:
                {
                    DrawTextureEx(texLogo_skin, {W/2 - texLogo_skin.width * 0.5f, H/6}, 0, 1.0f, WHITE);
                    float st_x = 100;
                    float st_y = H/2 - 50;

                    for (int id = 1; id <= 8; id++) {
                        if (id == 5) {
                            st_x = 100;
                            st_y += 200;
                        }

                        Extend(texPlayer[id - 1], st_x, st_y, 1.3f);

                        if (CheckCollisionPointRec(GetMousePosition(), {st_x, st_y, (float)texPlayer[id - 1].width, (float)texPlayer[id - 1].height})) {
                            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                                wave = 1;
                                skin_id = id;
                                player_pos.x = W/2;
                                player_pos.y = 800;
                                CurrentScreen = GAMEPLAY;
                            }
                        }
                        st_x += 220;
                    }

                }break;
                case GAMEPLAY:
                {
                    //DrawText(TextFormat("wait: %d", (int)enemies.size()), 10, 10, 40, RED);
                    if (!GameOver) {
                        float rotation = 0;

                        if (IsKeyDown(KEY_LEFT) && IsKeyDown(KEY_RIGHT)) rotation = 0;
                        else if (IsKeyDown(KEY_LEFT) && IsKeyDown(KEY_DOWN)) rotation = 30;
                        else if (IsKeyDown(KEY_RIGHT) && IsKeyDown(KEY_DOWN)) rotation = -30;
                        else if (IsKeyDown(KEY_LEFT))  rotation = -30;
                        else if (IsKeyDown(KEY_RIGHT)) rotation =  30;

                        auto& tex = texPlayer[skin_id - 1];
                        float hw = tex.width * 0.5f;
                        float fh = (float)tex.height;
                            
                        DrawTexturePro(
                            tex,
                            { 0, 0, (float)tex.width, fh },
                            { player_pos.x, player_pos.y, (float)tex.width, fh },
                            { hw, fh * 0.5f },   
                            rotation,
                            WHITE
                        );
                    
                    }  

                    switch (waveState)
                    {
                        case (WAVE_ANNOUNCE):
                        {
                            unsigned char a = (unsigned char)(1.0f * 255);
                            const char* waveText = TextFormat("WAVE %d", wave);
                            int fontSize = 50;
                            int textW = MeasureText(waveText, fontSize);
                            DrawText(waveText, W/2 - textW/2, H/2 - 25, 50, { 255, 255, 0, a });

                        }break;
                        case (WAVE_ENTERING):
                        {
                            float alpha = 1.0f - (waveTimer) / (enterTime);
                            unsigned char a = (unsigned char)(alpha * 255);
                            const char* waveText = TextFormat("WAVE %d", wave);
                            int fontSize = 50;
                            int textW = MeasureText(waveText, fontSize);
                            DrawText(waveText, W/2 - textW/2, H/2 - 25, 50, { 255, 255, 0, a });

                            for (auto &e : enemies) {
                                DrawTexture(texEnemy[e.tex_id - 1], e.pos.x, e.pos.y, WHITE);
                            }

                        }break;
                        case (WAVE_FIGHTING):
                        {
                            for (auto &e : enemies) {
                                if (!e.active) continue;
                                DrawTexture(texEnemy[e.tex_id - 1], e.pos.x, e.pos.y, WHITE);
                            }

                            for (auto &it : bullets) {
                                DrawTexture(texBullet, it.pos.x - texBullet.width * 0.5f, it.pos.y - texPlayer[skin_id - 1].height * 0.5f - texBullet.height, WHITE);
                            }

                            for (auto &it : enemy_bullets) {
                                DrawTexture(texBullet_e, it.pos.x - texBullet_e.width * 0.5f, it.pos.y - texBullet_e.height * 0.5f, WHITE);
                            }

                            for (auto& p : particles) {
                                unsigned char alpha = (unsigned char)(p.life * 255);
                                DrawCircleV(p.pos, 3, { 255, 150, 0, alpha });
                            }

                        }break;
                    }
                }break;
                case END:
                {
                    if (GameOver) DrawTexture(texLogo_gameover, W/2 - texLogo_gameover.width * 0.5f, 200, WHITE);
                    else DrawTexture(texLogo_victory, W/2 - texLogo_victory.width * 0.5f, 200, WHITE);

                    Extend(texButton_menu, W/2 - texButton_menu.width * 0.5f, H/2 + 20, 1.2f);
                    if (CheckCollisionPointRec(GetMousePosition(), {W/2 - texButton_menu.width * 0.5f, H/2 + 20, (float)texButton_menu.width, (float)texButton_menu.height})) {
                        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                            StopMusicStream(music_win);
                            StopMusicStream(music_lose);
                            PlayMusicStream(music_background);
                            playingWin = false;
                            playingLose = false;
                            CurrentScreen = MENU;
                        }
                    }

                    Extend(texButton_exit, W/2 - 142.5, H/2 + 220, 1.2f);
                    if (CheckCollisionPointRec(GetMousePosition(), {W/2 - 142.5, H/2 + 220, (float)texButton_exit.width, (float)texButton_exit.height})) {
                        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                            shouldExit = true;
                        }
                    } 
                }break;
                default: break;
            }

            
        EndDrawing();
    }

    for (auto &it : texPlayer) {
        UnloadTexture(it);
    }
    for (auto &it : texEnemy) {
        UnloadTexture(it);
    }
    UnloadTexture(texBackground_back);
    UnloadTexture(texBackground_front);
    UnloadTexture(texBackground_star);
    UnloadTexture(texLogo_space_shooter);
    UnloadTexture(texLogo_skin);
    UnloadTexture(texLogo_victory);
    UnloadTexture(texLogo_gameover);
    UnloadTexture(texBullet);
    UnloadTexture(texBullet_e);
    UnloadTexture(texButton_play);
    UnloadTexture(texButton_menu);
    UnloadTexture(texButton_exit);

    UnloadMusicStream(music_background);
    UnloadMusicStream(music_win);
    UnloadSound(sou_explose);
    UnloadSound(sou_laser);
    UnloadSound(sou_laser_enemy);

    CloseAudioDevice();

    CloseWindow();
    return 0;
}