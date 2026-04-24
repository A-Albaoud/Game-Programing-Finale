/**
* Author: [Abdulrahman Albaoud]
* Assignment: [Robotics Antibiotics]
* Date due: 24/04/2026, 2:00pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/
#include "CS3113/ShaderProgram.h"
#include "CS3113/BorderThreat.h"
#include "CS3113/BattleScene.h"
#include "CS3113/TitleScene.h"

// -- Virtual / letterbox resolution (4:3) -------------------------------------
constexpr int VIRTUAL_WIDTH  = 960;
constexpr int VIRTUAL_HEIGHT = 720;

// -- Physical window -----------------------------------------------------------
constexpr int SCREEN_WIDTH  = 1280;
constexpr int SCREEN_HEIGHT = 800;

constexpr int   FPS              = 120;
constexpr int   NUMBER_OF_LEVELS = 1;
constexpr float FIXED_TIMESTEP   = 1.0f / 60.0f;

// Centre of the virtual canvas (used for scene / effects initialisation).
constexpr Vector2 VIRTUAL_ORIGIN = { VIRTUAL_WIDTH / 2.0f, VIRTUAL_HEIGHT / 2.0f };

// -- Global variables ----------------------------------------------------------
AppStatus gAppStatus   = RUNNING;
float gPreviousTicks   = 0.0f;
float gTimeAccumulator = 0.0f;

TitleScene   *gTitleScene   = nullptr;

Scene        *gCurrentScene = nullptr;
std::vector<Scene*> gLevels = {};

Camera2D      gCamera  = { 0 };

Dungeon      *gDungeon = nullptr;
Effects      *gEffects = nullptr;
ShaderProgram gShader;
Vector2       gLightPosition = { 0.0f, 0.0f };
float         gHitFlash      = 0.0f;

// -- Letterbox render target ---------------------------------------------------
RenderTexture2D gRenderTarget;

// -- Border-threat overlay (lives outside all scenes) -------------------------
BorderThreat *gBorderThreat = nullptr;

// -- Music ---------------------------------------------------------------------
Music gMusicMap1;
Music gMusicMap2;
Music gMusicBattle;
bool  gOnMap2 = false;
Sound gWinJingle;
Sound gFootstep;
float gFootstepTimer = 0.0f;
Sound gSndHit;
Sound gSndMagic;
Sound gSndHitZsaber;
Sound gSndMagicZsaber;
Music gMusicBoss;
Music gMusicBossPhase2;
Music gMusicEnding;

// -- Battle --------------------------------------------------------------------
BattleScene  *gBattleScene = nullptr;
bool          gInBattle    = false;

// Player stats that persist across battles.
int gPlayerHP    = 100;
int gPlayerMaxHP = 100;
int gPlayerMP    = 40;
int gPlayerMaxMP = 40;
int gPlayerMoney = 0;

// -- Inventory -----------------------------------------------------------------
bool gShowInventory = false;

// -- Victory screen ------------------------------------------------------------
bool gVictoryPending = false;
int  gVictoryGold    = 150;

// -- Win screen (boss clear) ---------------------------------------------------
bool      gWinScreenActive  = false;
Texture2D gWinScreenTexture = { 0 };

// -- Game over -----------------------------------------------------------------
bool gGameOverActive = false;

// -- Random encounter ----------------------------------------------------------
float gStepAccumulator  = 0.0f;
constexpr float ENCOUNTER_STEP = 360.0f;

// -- Dialogue ------------------------------------------------------------------
struct DialogueBox {
    const char *speaker = "";
    const char *text    = "";
    const char *text2   = "";
    const char *text3   = "";
    bool isActive = false;

    void show(const char *s, const char *t, const char *t2 = "", const char *t3 = "")
    { speaker = s; text = t; text2 = t2; text3 = t3; isActive = true; }
    void dismiss() { isActive = false; }
};

DialogueBox gDialogue;

// -- Key items -----------------------------------------------------------------
bool gHasZsaber           = false;
bool gHasProcessor1       = false;
bool gHasProcessor2       = false;
bool gProcessor1Deposited = false;
bool gProcessor2Deposited = false;
bool gPendingMapSwitch    = false;
bool gPendingBossFight    = false;
bool gIsBossFight         = false;
bool gBossPhase2Active    = false;

// -- Consumable inventory ------------------------------------------------------
int gPotionCount     = 0;
int gBatteryCount    = 0;
int gHighPotionCount = 0;
int gHighEtherCount  = 0;

// -- Shop ----------------------------------------------------------------------
struct ShopState {
    bool        isOpen  = false;
    int         cursor  = 0;   // 0=Potion 1=Battery 2=Leave
    const char *message = "";
};
ShopState gShop;

// -- Function declarations -----------------------------------------------------
void switchToScene(Scene *scene);
void enterBattle();
void enterBossFight();
void exitBattle();
void initialise();
void processInput();
void processInteraction();
void update();
void renderInventory();
void renderDialogue();
void renderShop();
void renderVictory();
void render();
void shutdown();

// -----------------------------------------------------------------------------

void switchToScene(Scene *scene)
{
    gCurrentScene = scene;
    gCurrentScene->initialise();
    gCamera.target = gCurrentScene->getState().protag->getPosition();
}

void enterBattle()
{
    if (gInBattle) return;

    gShowInventory = false;
    gBorderThreat->pause();
    if (gCurrentScene) gCurrentScene->getState().protag->resetMovement();

    // Roll enemy type: 0=weak(green) 1=normal(blue) 2=strong(red)
    static const char *ENEMY_TEXTURES[3] = {
        "assets/game/metalsonic_green.png",
        "assets/game/metalsonic.png",
        "assets/game/metalsonic_red.png",
    };
    static const int ENEMY_HP[3]     = { 28, 40, 55 };
    static const int ENEMY_MINDMG[3] = {  2,  4,  8 };
    static const int ENEMY_MAXDMG[3] = {  6, 10, 16 };

    int roll = GetRandomValue(0, 2);

    gBattleScene = new BattleScene(
        VIRTUAL_ORIGIN,
        "#0d0d1a",
        ENEMY_TEXTURES[roll],
        { 183.0f, 220.0f },
        ENEMY_HP[roll],
        ENEMY_MINDMG[roll],
        ENEMY_MAXDMG[roll],
        gPlayerHP,  gPlayerMaxHP,
        gPlayerMP,  gPlayerMaxMP,
        gPotionCount, gBatteryCount,
        gHighPotionCount, gHighEtherCount,
        gHasZsaber ? 3 : 1,   // fight multiplier
        gHasZsaber ? 4 : 1    // magic multiplier
    );
    gBattleScene->initialise();
    gInBattle = true;

    PauseMusicStream(gOnMap2 ? gMusicMap2 : gMusicMap1);
    PlayMusicStream(gMusicBattle);
}

void enterBossFight()
{
    if (gInBattle) return;

    gShowInventory = false;
    gBorderThreat->pause();
    if (gCurrentScene) gCurrentScene->getState().protag->resetMovement();

    gBattleScene = new BattleScene(
        VIRTUAL_ORIGIN, "#0d0d1a",
        "assets/game/NEO.png",
        { 440.0f, 600.0f },
        1000, 8, 18,
        gPlayerHP, gPlayerMaxHP,
        gPlayerMP, gPlayerMaxMP,
        gPotionCount, gBatteryCount,
        gHighPotionCount, gHighEtherCount,
        gHasZsaber ? 3 : 1,
        gHasZsaber ? 4 : 1,
        false,   // static sprite, not animated sheet
        true,    // is boss
        "assets/game/NEO_OVERLORD.png"
    );
    gBattleScene->initialise();
    gInBattle     = true;
    gIsBossFight  = true;
    gBossPhase2Active = false;

    PauseMusicStream(gOnMap2 ? gMusicMap2 : gMusicMap1);
    PlayMusicStream(gMusicBoss);
}

void exitBattle()
{
    if (!gInBattle || !gBattleScene) return;

    gPlayerHP        = gBattleScene->getPlayerHP();
    gPlayerMP        = gBattleScene->getPlayerMP();
    gPotionCount     = gBattleScene->getPotionCount();
    gBatteryCount    = gBattleScene->getBatteryCount();
    gHighPotionCount = gBattleScene->getHighPotionCount();
    gHighEtherCount  = gBattleScene->getHighEtherCount();

    if (gBattleScene->didPlayerWin())
        gBorderThreat->pushBack(0.35f);

    gBattleScene->shutdown();
    delete gBattleScene;
    gBattleScene = nullptr;

    if (gIsBossFight && gBattleScene && gBattleScene->didPlayerWin())
    {
        Entity *neo = static_cast<Dungeon *>(gCurrentScene)->getNeo();
        if (neo) static_cast<Dungeon *>(gCurrentScene)->consumeNeo();
    }

    gBorderThreat->resume();
    gInBattle         = false;
    gIsBossFight      = false;
    gBossPhase2Active = false;

    StopMusicStream(gMusicBattle);
    StopMusicStream(gMusicBoss);
    StopMusicStream(gMusicBossPhase2);
    StopSound(gWinJingle);
    ResumeMusicStream(gOnMap2 ? gMusicMap2 : gMusicMap1);
}

// -----------------------------------------------------------------------------

void initialise()
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Robotics Antibiotics");
    InitAudioDevice();

    gMusicMap1        = LoadMusicStream("assets/CORE.mp3");
    gMusicMap2        = LoadMusicStream("assets/game/077 Digital Roots.flac");
    gMusicBattle      = LoadMusicStream("assets/game/battletheme.mp3");
    gMusicBoss        = LoadMusicStream("assets/game/PHASE1.mp3");
    gMusicBossPhase2  = LoadMusicStream("assets/game/PHASE2.mp3");
    SetMusicVolume(gMusicMap1,       0.22f);
    SetMusicVolume(gMusicMap2,       1.0f);
    SetMusicVolume(gMusicBattle,     0.22f);
    SetMusicVolume(gMusicBoss,       1.0f);
    SetMusicVolume(gMusicBossPhase2, 1.0f);
    gMusicEnding = LoadMusicStream("assets/game/Goodbye2.mp3");
    SetMusicVolume(gMusicEnding, 0.22f);
    gWinJingle      = LoadSound("assets/game/win_jingle.wav");
    gSndHit         = LoadSound("assets/game/snd_punchstrong.wav");
    gSndMagic       = LoadSound("assets/game/snd_mtt_hit.wav");
    gSndHitZsaber   = LoadSound("assets/game/snd_criticalswing.wav");
    gSndMagicZsaber = LoadSound("assets/game/snd_rudebuster_swing.wav");
    SetSoundVolume(gWinJingle, 1.0f);
    gFootstep  = LoadSound("assets/game/metalpick.wav");
    SetSoundVolume(gFootstep, 0.32f);

    gWinScreenTexture = LoadTexture("assets/game/WinScreen.png");

    gRenderTarget = LoadRenderTexture(VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
    SetTextureFilter(gRenderTarget.texture, TEXTURE_FILTER_BILINEAR);

    gShader.load("shaders/vertex.glsl", "shaders/fragment.glsl");

    gDungeon = new Dungeon(VIRTUAL_ORIGIN, "#011627");
    gLevels.push_back(gDungeon);

    gTitleScene = new TitleScene(SCREEN_WIDTH, SCREEN_HEIGHT);
    gTitleScene->initialise();

    gCamera.offset   = VIRTUAL_ORIGIN;
    gCamera.rotation = 0.0f;
    gCamera.zoom     = 1.0f;

    gEffects = new Effects(VIRTUAL_ORIGIN,
                           (float)VIRTUAL_WIDTH  * 1.25f,
                           (float)VIRTUAL_HEIGHT * 1.25f);
    gEffects->setEffectSpeed(0.33f);

    gBorderThreat = new BorderThreat((float)VIRTUAL_WIDTH, (float)VIRTUAL_HEIGHT);

    SetTargetFPS(FPS);
}

// -----------------------------------------------------------------------------

void processInteraction()
{
    if (gDialogue.isActive)
    {
        gDialogue.dismiss();
        if (gPendingMapSwitch)
        {
            gPendingMapSwitch = false;
            static_cast<Dungeon *>(gCurrentScene)->switchToMap2();
            StopMusicStream(gMusicMap1);
            PlayMusicStream(gMusicMap2);
            gOnMap2 = true;
        }
        else if (gPendingBossFight)
        {
            gPendingBossFight = false;
            enterBossFight();
        }
        return;
    }

    if (!gCurrentScene || !gCurrentScene->getState().map) return;

    Entity *player = gCurrentScene->getState().protag;
    Vector2 pos    = player->getPosition();
    Map    *map    = gCurrentScene->getState().map;

    // -- Z-Saber pickup --------------------------------------------------------
    if (!gHasZsaber)
    {
        Entity *zsaber = static_cast<Dungeon *>(gCurrentScene)->getZsaber();
        if (zsaber && zsaber->isActive())
        {
            Vector2 zp = zsaber->getPosition();
            float dx = zp.x - pos.x, dy = zp.y - pos.y;
            if (dx * dx + dy * dy < 80.0f * 80.0f)
            {
                gHasZsaber = true;
                static_cast<Dungeon *>(gCurrentScene)->consumeZsaber();
                gDialogue.show("System", "Obtained the Z-Saber! ATK x3 / MAG x4.");
                return;
            }
        }
    }

    // -- Prohobot NPC dialogue -------------------------------------------------
    {
        Entity *prohobot = static_cast<Dungeon *>(gCurrentScene)->getProhobot();
        if (prohobot && prohobot->isActive())
        {
            Vector2 np = prohobot->getPosition();
            float dx = np.x - pos.x, dy = np.y - pos.y;
            if (dx * dx + dy * dy < 90.0f * 90.0f)
            {
                gDialogue.show(
                    "John Mcafee",
                    "Hey you should be careful going to the left, all the dead",
                    "circuitry has caused the area to get really dark. If you aren't",
                    "careful you might not realize that you've already lost!"
                );
                return;
            }
        }
    }

    // -- Prohobot 2 NPC dialogue -----------------------------------------------
    {
        Entity *prohobot2 = static_cast<Dungeon *>(gCurrentScene)->getProhobot2();
        if (prohobot2 && prohobot2->isActive())
        {
            Vector2 np = prohobot2->getPosition();
            float dx = np.x - pos.x, dy = np.y - pos.y;
            if (dx * dx + dy * dy < 90.0f * 90.0f)
            {
                gDialogue.show(
                    "██████ █",
                    "I heard in the barren land of broken circuitry there is a",
                    "Legendary Saber capable of cutting down even the most volatile",
                    "of viruses... It belonged to a legendary reploid after all."
                );
                return;
            }
        }
    }

    // -- NEO boss fight --------------------------------------------------------
    {
        Entity *neo = static_cast<Dungeon *>(gCurrentScene)->getNeo();
        if (neo && neo->isActive())
        {
            Vector2 np = neo->getPosition();
            float dx = np.x - pos.x, dy = (np.y + 80.0f) - pos.y;
            if (dx * dx + dy * dy < 90.0f * 90.0f)
            {
                gPendingBossFight = true;
                gDialogue.show(
                    "NEO",
                    "I'm no mere Virus, no I am evolved I am both",
                    "technological and Biological. Yes I am a",
                    "TRUE INFECTION"
                );
                return;
            }
        }
    }

    // -- NPC shop proximity ---------------------------------------------------
    Entity *metaton = static_cast<Dungeon *>(gCurrentScene)->getMetaton();
    if (metaton)
    {
        Vector2 npcPos = metaton->getPosition();
        float dx = npcPos.x - pos.x, dy = npcPos.y - pos.y;
        if (dx * dx + dy * dy < 90.0f * 90.0f)
        {
            gShop.isOpen  = true;
            gShop.cursor  = 0;
            gShop.message = "";
            gPlayerHP     = gPlayerMaxHP;
            gPlayerMP     = gPlayerMaxMP;
            return;
        }
    }

    constexpr float PROBE = 70.0f;
    Vector2 probes[] = {
        pos,
        { pos.x - PROBE, pos.y },
        { pos.x + PROBE, pos.y },
        { pos.x, pos.y - PROBE },
        { pos.x, pos.y + PROBE },
    };

    for (auto &p : probes)
    {
        int tile = map->getTileAt(p);

        if (tile == 5 && !gHasProcessor1)
        {
            gHasProcessor1 = true;
            map->setTileAt(p, 129);
            gDialogue.show("System", "Obtained [Core Processor 1].");
            return;
        }
        if (tile == 6 && !gHasProcessor2)
        {
            gHasProcessor2 = true;
            map->setTileAt(p, 1);
            gDialogue.show("System", "Obtained [Core Processor 2].");
            return;
        }
        if (tile == 85)
        {
            bool depositedSomething = false;
            if (gHasProcessor1 && !gProcessor1Deposited) { gProcessor1Deposited = true; depositedSomething = true; }
            if (gHasProcessor2 && !gProcessor2Deposited) { gProcessor2Deposited = true; depositedSomething = true; }

            if (gProcessor1Deposited && gProcessor2Deposited)
            {
                gPendingMapSwitch = true;
                gDialogue.show("Terminal", "Both Core Processors online. Advancing to next sector...");
            }
            else if (depositedSomething)
            {
                if (gProcessor1Deposited && !gProcessor2Deposited)
                    gDialogue.show("Terminal", "Core Processor 1 deposited. Locate Core Processor 2.");
                else
                    gDialogue.show("Terminal", "Core Processor 2 deposited. Locate Core Processor 1.");
            }
            else
            {
                if (!gHasProcessor1 && !gHasProcessor2)
                    gDialogue.show("Terminal", "Core Processors required to unlock next sector.");
                else
                    gDialogue.show("Terminal", "Awaiting remaining Core Processor...");
            }
            return;
        }
    }
}

// -----------------------------------------------------------------------------

void renderDialogue()
{
    const int DH = 160;
    const int DX = 40;
    const int DY = VIRTUAL_HEIGHT - DH - 20;
    const int DW = VIRTUAL_WIDTH - DX * 2;

    DrawRectangle(DX, DY, DW, DH, { 8, 8, 18, 230 });
    DrawRectangleLines(DX + 4, DY + 4, DW - 8, DH - 8, { 160, 160, 160, 180 });

    DrawText(gDialogue.speaker, DX + 20, DY + 16, 22, YELLOW);
    DrawLine(DX + 16, DY + 44, DX + DW - 16, DY + 44, { 100, 100, 100, 160 });
    DrawText(gDialogue.text,  DX + 20, DY + 50, 20, WHITE);
    if (gDialogue.text2[0])
        DrawText(gDialogue.text2, DX + 20, DY + 76, 20, WHITE);
    if (gDialogue.text3[0])
        DrawText(gDialogue.text3, DX + 20, DY + 102, 20, WHITE);

    const char *hint = "[ Z ]  Continue";
    int hw = MeasureText(hint, 18);
    DrawText(hint, DX + DW - hw - 16, DY + DH - 28, 18, GRAY);
}

// -----------------------------------------------------------------------------

void renderShop()
{
    const int SW = 520, SH = 360;
    const int SX = (VIRTUAL_WIDTH  - SW) / 2;
    const int SY = (VIRTUAL_HEIGHT - SH) / 2;

    DrawRectangle(SX, SY, SW, SH, { 8, 8, 24, 240 });
    DrawRectangleLines(SX + 4, SY + 4, SW - 8, SH - 8, { 180, 160, 80, 200 });

    // Title + gold
    DrawText("METATON'S SHOP", SX + 20, SY + 18, 26, YELLOW);
    DrawText(TextFormat("%d G", gPlayerMoney), SX + SW - 90, SY + 18, 22, { 255, 215, 0, 255 });
    DrawLine(SX + 16, SY + 54, SX + SW - 16, SY + 54, { 120, 110, 60, 180 });

    struct { const char *name; int cost; const char *desc; } items[5] = {
        { "Potion",        10, "Restores 20 HP"    },
        { "Dbl-A Battery", 20, "Restores 15-30 MP" },
        { "Hi-Potion",     20, "Restores 60 HP"    },
        { "Super Battery",  25, "Restores 25-30 MP" },
        { "Leave",          0, ""                  },
    };

    for (int i = 0; i < 5; i++)
    {
        bool sel = (i == gShop.cursor);
        Color col = sel ? YELLOW : WHITE;
        int row = SY + 62 + i * 42;
        DrawText(sel ? ">" : " ", SX + 16, row, 22, col);
        DrawText(items[i].name, SX + 36, row, 22, col);
        if (items[i].cost > 0)
            DrawText(TextFormat("%d G", items[i].cost), SX + SW - 90, row, 20, { 255, 215, 0, 255 });
        if (items[i].desc[0])
            DrawText(items[i].desc, SX + 36, row + 22, 16, { 160, 160, 160, 255 });
    }

    // Inventory counts
    DrawLine(SX + 16, SY + SH - 72, SX + SW - 16, SY + SH - 72, { 80, 80, 80, 160 });
    DrawText(TextFormat("Pot:%d  Bat:%d  HiPot:%d  SupBat:%d",
             gPotionCount, gBatteryCount, gHighPotionCount, gHighEtherCount),
             SX + 20, SY + SH - 56, 16, { 180, 180, 180, 255 });

    // Feedback message / hints
    if (gShop.message[0])
        DrawText(gShop.message, SX + 20, SY + SH - 32, 18, { 100, 220, 100, 255 });
    else
        DrawText("[W/S] Select   [Z/Enter] Buy   [I] Close", SX + 20, SY + SH - 32, 16, GRAY);
}

// -----------------------------------------------------------------------------

void processInput()
{
    if (IsKeyPressed(KEY_Q) || WindowShouldClose())
    {
        gAppStatus = TERMINATED;
        return;
    }

    if (IsKeyPressed(KEY_NINE)) { gPlayerMoney += 90; return; } // DEBUG

    // -- Title screen input ----------------------------------------------------
    if (gTitleScene)
    {
        gTitleScene->processInput(IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_Z));
        if (gTitleScene->isDone())
        {
            gTitleScene->shutdown();
            delete gTitleScene;
            gTitleScene = nullptr;
            switchToScene(gLevels[0]);
            PlayMusicStream(gMusicMap1);
            gPreviousTicks = (float)GetTime(); // prevent huge deltaTime spike after title wait
        }
        return;
    }

    // -- Win screen input ------------------------------------------------------
    if (gWinScreenActive)
    {
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_Z))
        {
            StopSound(gWinJingle);
            StopMusicStream(gMusicEnding);
            gWinScreenActive = false;

            // Reset all player/world state for a fresh run
            gPlayerHP            = gPlayerMaxHP;
            gPlayerMP            = gPlayerMaxMP;
            gPlayerMoney         = 0;
            gPotionCount         = gBatteryCount = gHighPotionCount = gHighEtherCount = 0;
            gHasZsaber           = false;
            gHasProcessor1       = gHasProcessor2       = false;
            gProcessor1Deposited = gProcessor2Deposited = false;
            gPendingBossFight    = false;
            gOnMap2              = false;
            gDialogue.dismiss();
            gShowInventory = false;
            gShop.isOpen   = false;
            gBorderThreat->setEncroachment(0.0f);
            gBorderThreat->pause();

            // Restart at title screen -switchToScene will reinitialise the dungeon
            gTitleScene = new TitleScene(SCREEN_WIDTH, SCREEN_HEIGHT);
            gTitleScene->initialise();
            gPreviousTicks = (float)GetTime();
        }
        return;
    }

    // -- Game over input -------------------------------------------------------
    if (gGameOverActive)
    {
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_Z))
        {
            gGameOverActive = false;

            // Reset all player/world state
            gPlayerHP            = gPlayerMaxHP;
            gPlayerMP            = gPlayerMaxMP;
            gPlayerMoney         = 0;
            gPotionCount         = gBatteryCount = gHighPotionCount = gHighEtherCount = 0;
            gHasZsaber           = false;
            gHasProcessor1       = gHasProcessor2       = false;
            gProcessor1Deposited = gProcessor2Deposited = false;
            gPendingBossFight    = false;
            gOnMap2              = false;
            gDialogue.dismiss();
            gShowInventory = false;
            gShop.isOpen   = false;
            gBorderThreat->setEncroachment(0.0f);
            gBorderThreat->pause();

            gTitleScene = new TitleScene(SCREEN_WIDTH, SCREEN_HEIGHT);
            gTitleScene->initialise();
            gPreviousTicks = (float)GetTime();
        }
        return;
    }

    // -- Battle input ----------------------------------------------------------
    if (gInBattle && gBattleScene)
    {
        if (gVictoryPending)
        {
            if (IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
            {
                gPlayerMoney   += gVictoryGold;
                gVictoryPending = false;
                exitBattle();
            }
            return;
        }
        gBattleScene->processInput(
            IsKeyPressed(KEY_W),
            IsKeyPressed(KEY_S),
            IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)
        );
        return;
    }

    // -- Dialogue interaction --------------------------------------------------
    if (!gShop.isOpen && (IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_ENTER))) { processInteraction(); return; }

    // Block overworld movement while dialogue is open
    if (gDialogue.isActive) return;

    // -- Shop input ------------------------------------------------------------
    if (gShop.isOpen)
    {
        if (IsKeyPressed(KEY_W)) gShop.cursor = (gShop.cursor + 4) % 5;
        if (IsKeyPressed(KEY_S)) gShop.cursor = (gShop.cursor + 1) % 5;
        if (IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_ENTER))
        {
            auto buy = [&](int cost, int &count, const char *msg) {
                if (gPlayerMoney >= cost) { gPlayerMoney -= cost; count++; gShop.message = msg; }
                else                     { gShop.message = "Not enough gold!"; }
            };
            switch (gShop.cursor)
            {
                case 0: buy(10, gPotionCount,     "Purchased Potion!");    break;
                case 1: buy(20, gBatteryCount,    "Purchased Battery!");   break;
                case 2: buy(20, gHighPotionCount, "Purchased Hi-Potion!"); break;
                case 3: buy(25, gHighEtherCount,  "Purchased Super Battery!"); break;
                case 4: gShop.isOpen = false;                              break;
            }
        }
        if (IsKeyPressed(KEY_I)) gShop.isOpen = false;
        return;
    }

    // -- Inventory toggle ------------------------------------------------------
    if (IsKeyPressed(KEY_I))
    {
        gShowInventory = !gShowInventory;
        return;
    }

    if (gShowInventory) return;

    // -- Overworld input (WASD, top-down) -------------------------------------
    gCurrentScene->getState().protag->resetMovement();

    if (IsKeyDown(KEY_A)) gCurrentScene->getState().protag->moveLeft();
    if (IsKeyDown(KEY_D)) gCurrentScene->getState().protag->moveRight();
    if (IsKeyDown(KEY_W)) gCurrentScene->getState().protag->moveUp();
    if (IsKeyDown(KEY_S)) gCurrentScene->getState().protag->moveDown();

    if (GetLength(gCurrentScene->getState().protag->getMovement()) > 1.0f)
        gCurrentScene->getState().protag->normaliseMovement();

    if (IsKeyPressed(KEY_E)) enterBattle();
}

// -----------------------------------------------------------------------------

void update()
{
    UpdateMusicStream(gMusicMap1);
    UpdateMusicStream(gMusicMap2);
    UpdateMusicStream(gMusicBattle);
    UpdateMusicStream(gMusicBoss);
    UpdateMusicStream(gMusicBossPhase2);
    UpdateMusicStream(gMusicEnding);

    if (gTitleScene || gWinScreenActive || gGameOverActive)
    {
        if (gTitleScene) gTitleScene->update(GetFrameTime());
        return;
    }

    float ticks    = (float)GetTime();
    float deltaTime = ticks - gPreviousTicks;
    gPreviousTicks  = ticks;

    deltaTime += gTimeAccumulator;
    if (deltaTime < FIXED_TIMESTEP) { gTimeAccumulator = deltaTime; return; }

    while (deltaTime >= FIXED_TIMESTEP)
    {
        if (gInBattle && gBattleScene && !gVictoryPending)
        {
            gBattleScene->update(FIXED_TIMESTEP);
            if (gBattleScene->consumePlayerHitSignal()) gHitFlash = 1.0f;
            if (gBattleScene->consumePlayerFightSignal())
                PlaySound(gHasZsaber ? gSndHitZsaber : gSndHit);
            if (gBattleScene->consumePlayerMagicSignal())
                PlaySound(gHasZsaber ? gSndMagicZsaber : gSndMagic);

            // Boss phase 2 signals
            if (gBattleScene->consumePhase2Signal())
            {
                gBossPhase2Active = true;
                gBorderThreat->setEncroachment(0.5f);
                gBorderThreat->resume();   // border is live during phase 2
                StopMusicStream(gMusicBoss);
                PlayMusicStream(gMusicBossPhase2);
            }
            if (gBossPhase2Active)
            {
                if (gBattleScene->consumePlayerHitBossSignal())
                    gBorderThreat->pushBack(0.25f);
                if (gBattleScene->consumeBossBorderAttackSignal())
                    gBorderThreat->addEncroachment(0.5f);
            }

            if (gBattleScene->isBattleOver())
            {
                if (gBattleScene->didPlayerWin())
                {
                    StopMusicStream(gMusicBattle);
                    StopMusicStream(gMusicBoss);
                    StopMusicStream(gMusicBossPhase2);

                    if (gIsBossFight)
                    {
                        // Clean up battle, then show the win screen
                        gPlayerHP        = gBattleScene->getPlayerHP();
                        gPlayerMP        = gBattleScene->getPlayerMP();
                        gBattleScene->shutdown();
                        delete gBattleScene;
                        gBattleScene      = nullptr;
                        gInBattle         = false;
                        gIsBossFight      = false;
                        gBossPhase2Active = false;
                        gWinScreenActive  = true;
                        PlayMusicStream(gMusicEnding);
                    }
                    else
                    {
                        gVictoryGold    = GetRandomValue(15, 75);
                        gVictoryPending = true;
                        PlaySound(gWinJingle);
                    }
                }
                else if (gBattleScene->didPlayerRun())
                {
                    // Player fled -return to overworld, no game over
                    gPlayerHP        = gBattleScene->getPlayerHP();
                    gPlayerMP        = gBattleScene->getPlayerMP();
                    gPotionCount     = gBattleScene->getPotionCount();
                    gBatteryCount    = gBattleScene->getBatteryCount();
                    gHighPotionCount = gBattleScene->getHighPotionCount();
                    gHighEtherCount  = gBattleScene->getHighEtherCount();
                    StopMusicStream(gMusicBattle);
                    StopMusicStream(gMusicBoss);
                    StopMusicStream(gMusicBossPhase2);
                    gBattleScene->shutdown();
                    delete gBattleScene;
                    gBattleScene      = nullptr;
                    gInBattle         = false;
                    gIsBossFight      = false;
                    gBossPhase2Active = false;
                    ResumeMusicStream(gOnMap2 ? gMusicMap2 : gMusicMap1);
                }
                else
                {
                    // Player died - clean up battle and show game over
                    StopMusicStream(gMusicBattle);
                    StopMusicStream(gMusicBoss);
                    StopMusicStream(gMusicBossPhase2);
                    gBattleScene->shutdown();
                    delete gBattleScene;
                    gBattleScene      = nullptr;
                    gInBattle         = false;
                    gIsBossFight      = false;
                    gBossPhase2Active = false;
                    gGameOverActive   = true;
                }
            }
        }

        if (gHitFlash > 0.0f)
        {
            gHitFlash -= 2.5f * FIXED_TIMESTEP;   // fades out in ~0.4 s
            if (gHitFlash < 0.0f) gHitFlash = 0.0f;
        }
        else
        {
            // Pause the world while inventory, dialogue, shop, or battle is open
            if (!gInBattle && !gShowInventory && !gDialogue.isActive && !gShop.isOpen)
            {
                gCurrentScene->update(FIXED_TIMESTEP);
                gBorderThreat->update(FIXED_TIMESTEP);

                Vector2 cameraTarget = {
                    gCurrentScene->getState().protag->getPosition().x,
                    gCurrentScene->getState().protag->getPosition().y
                };

                panCamera(&gCamera, &cameraTarget);
                gEffects->update(FIXED_TIMESTEP, &cameraTarget);
                gLightPosition = gCurrentScene->getState().protag->getPosition();

                // -- Random encounter --------------------------------------
                Entity *player = gCurrentScene->getState().protag;
                float moveLen  = GetLength(player->getMovement());

                // -- Footstep sound ----------------------------------------
                if (moveLen > 0.0f)
                {
                    gFootstepTimer -= FIXED_TIMESTEP;
                    if (gFootstepTimer <= 0.0f)
                    {
                        PlaySound(gFootstep);
                        gFootstepTimer = 0.32f;
                    }
                }
                else { gFootstepTimer = 0.0f; }
                if (moveLen > 0.0f)
                {
                    gStepAccumulator += (float)player->getSpeed() * moveLen * FIXED_TIMESTEP;
                    while (gStepAccumulator >= ENCOUNTER_STEP)
                    {
                        gStepAccumulator -= ENCOUNTER_STEP;
                        if (GetRandomValue(0, 15) == 0)
                        {
                            gStepAccumulator = 0.0f;
                            enterBattle();
                            break;
                        }
                    }
                }

                if (gBorderThreat->isGameOver()) gAppStatus = TERMINATED;
            }
        }

        deltaTime -= FIXED_TIMESTEP;
    }
    gTimeAccumulator = deltaTime;
}

// -----------------------------------------------------------------------------

void renderInventory()
{
    // Semi-transparent dark panel centred on the virtual canvas
    const int PAD  = 60;
    const int PX   = PAD;
    const int PY   = PAD;
    const int PW   = VIRTUAL_WIDTH  - PAD * 2;
    const int PH   = VIRTUAL_HEIGHT - PAD * 2;

    DrawRectangle(PX, PY, PW, PH, { 8, 8, 18, 230 });
    DrawRectangleLines(PX + 4, PY + 4, PW - 8, PH - 8, { 160, 160, 160, 180 });

    const char *title = "INVENTORY";
    int tw = MeasureText(title, 48);
    DrawText(title, (VIRTUAL_WIDTH - tw) / 2, PY + 30, 48, YELLOW);

    // Divider
    DrawLine(PX + 20, PY + 90, PX + PW - 20, PY + 90, { 100, 100, 100, 180 });

    // Stats
    int col1 = PX + 60;
    int row  = PY + 120;

    if (gHasZsaber)
    {
        DrawText("Z-Saber", col1, row, 24, { 100, 180, 255, 255 });
        DrawText("ATK x3  MAG x4", col1 + 160, row, 24, WHITE);
        row += 50;
    }

    DrawText("GOLD",   col1, row,       24, { 255, 215, 0, 255 });
    DrawText(TextFormat("%d", gPlayerMoney), col1 + 160, row, 24, WHITE);

    row += 50;
    DrawText("HP",     col1, row,       24, { 80, 220, 80, 255 });
    DrawText(TextFormat("%d / %d", gPlayerHP, gPlayerMaxHP), col1 + 160, row, 24, WHITE);

    row += 50;
    DrawText("MP",      col1, row,      24, { 100, 140, 255, 255 });
    DrawText(TextFormat("%d / %d", gPlayerMP, gPlayerMaxMP), col1 + 160, row, 24, WHITE);

    row += 50;
    DrawText("Potions",   col1, row, 24, { 220, 100, 100, 255 });
    DrawText(TextFormat("x%d", gPotionCount),     col1 + 160, row, 24, WHITE);

    row += 50;
    DrawText("Batteries", col1, row, 24, { 100, 200, 255, 255 });
    DrawText(TextFormat("x%d", gBatteryCount),    col1 + 160, row, 24, WHITE);

    row += 50;
    DrawText("Hi-Potions",col1, row, 24, { 220, 160, 80,  255 });
    DrawText(TextFormat("x%d", gHighPotionCount), col1 + 160, row, 24, WHITE);

    row += 50;
    DrawText("Super Batteries", col1, row, 24, { 160, 100, 255, 255 });
    DrawText(TextFormat("x%d", gHighEtherCount),  col1 + 160, row, 24, WHITE);

    // Close hint
    const char *hint = "[ I ]  Close";
    int hw = MeasureText(hint, 20);
    DrawText(hint, (VIRTUAL_WIDTH - hw) / 2, PY + PH - 40, 20, GRAY);
}

// -----------------------------------------------------------------------------

void renderVictory()
{
    const int BW = 500, BH = 240;
    const int BX = (VIRTUAL_WIDTH  - BW) / 2;
    const int BY = (VIRTUAL_HEIGHT - BH) / 2;

    DrawRectangle(BX, BY, BW, BH, { 8, 22, 8, 245 });
    DrawRectangleLines(BX + 4, BY + 4, BW - 8, BH - 8, { 80, 220, 80, 220 });

    const char *title = "VICTORY!";
    int tw = MeasureText(title, 52);
    DrawText(title, (VIRTUAL_WIDTH - tw) / 2, BY + 22, 52, { 100, 240, 100, 255 });

    DrawLine(BX + 20, BY + 86, BX + BW - 20, BY + 86, { 60, 140, 60, 180 });


    DrawText(TextFormat("Obtained  %d  gold!", gVictoryGold),
             BX + 30, BY + 106, 28, { 255, 215, 0, 255 });

    const char *hint = "[ Z / Enter ]  Continue";
    int hw = MeasureText(hint, 18);
    DrawText(hint, (VIRTUAL_WIDTH - hw) / 2, BY + BH - 34, 18, GRAY);
}

// -----------------------------------------------------------------------------

void render()
{
    // -- Title screen: full-window, no letterbox -------------------------------
    if (gTitleScene)
    {
        BeginDrawing();
            gTitleScene->render();
        EndDrawing();
        return;
    }

    // -- Win screen: full-window, no letterbox ---------------------------------
    if (gWinScreenActive)
    {
        BeginDrawing();
            ClearBackground(BLACK);
            if (gWinScreenTexture.id > 0)
            {
                float scaleX = (float)SCREEN_WIDTH  / (float)gWinScreenTexture.width;
                float scaleY = (float)SCREEN_HEIGHT / (float)gWinScreenTexture.height;
                float scale  = (scaleX < scaleY) ? scaleX : scaleY;
                float w = gWinScreenTexture.width  * scale;
                float h = gWinScreenTexture.height * scale;
                DrawTextureEx(gWinScreenTexture,
                              { (SCREEN_WIDTH - w) / 2.0f, (SCREEN_HEIGHT - h) / 2.0f },
                              0.0f, scale, WHITE);
            }
        EndDrawing();
        return;
    }

    // -- Game over screen: full-window -----------------------------------------
    if (gGameOverActive)
    {
        BeginDrawing();
            ClearBackground({ 10, 0, 0, 255 });

            const char *title = "GAME OVER";
            int ts = 100;
            int tw = MeasureText(title, ts);
            DrawText(title, (SCREEN_WIDTH - tw) / 2, SCREEN_HEIGHT / 3, ts, { 200, 30, 30, 255 });

            if (fmodf((float)GetTime(), 1.4f) < 0.7f)
            {
                const char *hint = "PRESS ENTER TO RETURN TO TITLE";
                int hs = 28;
                int hw = MeasureText(hint, hs);
                DrawText(hint, (SCREEN_WIDTH - hw) / 2, SCREEN_HEIGHT * 2 / 3, hs, GRAY);
            }
        EndDrawing();
        return;
    }

    // -- Draw everything into the virtual-resolution render target -------------
    BeginTextureMode(gRenderTarget);

        if (gInBattle && gBattleScene)
        {
            gShader.begin();
            gBattleScene->render();
            gShader.setFloat("hitFlash", gHitFlash);
            gShader.setVector2("sceneOrigin", { 0.0f, 0.0f }); 
            gShader.end();

            if (gBossPhase2Active) gBorderThreat->render();
            if (gVictoryPending) renderVictory();
        }
        else
        {
            ClearBackground(ColorFromHex(gCurrentScene->getBGColourHexCode()));

            BeginMode2D(gCamera);
                gShader.begin();
                gCurrentScene->render();
                gShader.setVector2("lightPosition", gLightPosition);
                gShader.setVector2("sceneOrigin", VIRTUAL_ORIGIN);
                gShader.setFloat("hitFlash", 0.0f);
                gShader.end();
                gEffects->render();
            EndMode2D();

            gBorderThreat->render();

            if (gShowInventory)      renderInventory();
            if (gDialogue.isActive)  renderDialogue();
            if (gShop.isOpen)        renderShop();
        }

    EndTextureMode();

    // -- Letterbox blit to the physical window ---------------------------------
    float scaleX = (float)SCREEN_WIDTH  / (float)VIRTUAL_WIDTH;
    float scaleY = (float)SCREEN_HEIGHT / (float)VIRTUAL_HEIGHT;
    float scale  = (scaleX < scaleY) ? scaleX : scaleY;

    float offsetX = ((float)SCREEN_WIDTH  - (float)VIRTUAL_WIDTH  * scale) * 0.5f;
    float offsetY = ((float)SCREEN_HEIGHT - (float)VIRTUAL_HEIGHT * scale) * 0.5f;

    Rectangle srcRect = { 0.0f, 0.0f, (float)VIRTUAL_WIDTH, -(float)VIRTUAL_HEIGHT };
    Rectangle dstRect = { offsetX, offsetY,
                          (float)VIRTUAL_WIDTH * scale,
                          (float)VIRTUAL_HEIGHT * scale };

    BeginDrawing();
        ClearBackground(BLACK);
        DrawTexturePro(gRenderTarget.texture, srcRect, dstRect, { 0.0f, 0.0f }, 0.0f, WHITE);
    EndDrawing();
}

// -----------------------------------------------------------------------------

void shutdown()
{
    if (gTitleScene)  { gTitleScene->shutdown();  delete gTitleScene;  }
    if (gBattleScene) { gBattleScene->shutdown(); delete gBattleScene; }

    delete gBorderThreat;
    gBorderThreat = nullptr;

    delete gDungeon;
    for (int i = 0; i < NUMBER_OF_LEVELS; i++) gLevels[i] = nullptr;

    delete gEffects;
    gEffects = nullptr;

    if (gWinScreenTexture.id > 0) UnloadTexture(gWinScreenTexture);
    UnloadRenderTexture(gRenderTarget);
    gShader.unload();

    UnloadMusicStream(gMusicMap1);
    UnloadMusicStream(gMusicMap2);
    UnloadMusicStream(gMusicBattle);
    UnloadMusicStream(gMusicBoss);
    UnloadMusicStream(gMusicBossPhase2);
    UnloadMusicStream(gMusicEnding);
    UnloadSound(gWinJingle);
    UnloadSound(gFootstep);
    UnloadSound(gSndHit);
    UnloadSound(gSndMagic);
    UnloadSound(gSndHitZsaber);
    UnloadSound(gSndMagicZsaber);
    CloseAudioDevice();
    CloseWindow();
}

// -----------------------------------------------------------------------------

int main(void)
{
    initialise();

    while (gAppStatus == RUNNING)
    {
        processInput();
        update();

        if (!gTitleScene && !gInBattle && gCurrentScene && gCurrentScene->getState().nextSceneID > 0)
        {
            int id = gCurrentScene->getState().nextSceneID;
            switchToScene(gLevels[id]);
        }

        render();
    }

    shutdown();
    return 0;
}
