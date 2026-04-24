#include "BattleScene.h"

// -- Menu labels ---------------------------------------------------------------
static const char *MENU_LABELS[4] = { "Fight", "Magic", "Item", "Run" };

// -----------------------------------------------------------------------------
// Construction / destruction
// -----------------------------------------------------------------------------

BattleScene::BattleScene(Vector2     origin,
                         const char *bgHexCode,
                         const char *enemyTexturePath,
                         Vector2     enemyDisplaySize,
                         int         enemyHP,
                         int         enemyMinDmg,
                         int         enemyMaxDmg,
                         int         playerHP,
                         int         playerMaxHP,
                         int         playerMP,
                         int         playerMaxMP,
                         int         startPotions,
                         int         startBatteries,
                         int         startHighPotions,
                         int         startHighEthers,
                         int         fightMult,
                         int         magicMult,
                         bool        enemyIsAnimated,
                         bool        isBoss,
                         const char *phase2TexturePath)
    : Scene(origin, bgHexCode),
      mEnemyTexturePath(enemyTexturePath),
      mEnemyDisplaySize(enemyDisplaySize),
      mInitEnemyHP(enemyHP),
      mInitEnemyMinDmg(enemyMinDmg),
      mInitEnemyMaxDmg(enemyMaxDmg),
      mInitPlayerHP(playerHP),
      mInitPlayerMaxHP(playerMaxHP),
      mInitPlayerMP(playerMP),
      mInitPlayerMaxMP(playerMaxMP),
      mInitPotions(startPotions),
      mInitBatteries(startBatteries),
      mInitHighPotions(startHighPotions),
      mInitHighEthers(startHighEthers),
      mInitFightMult(fightMult),
      mInitMagicMult(magicMult),
      mEnemyIsAnimated(enemyIsAnimated),
      mIsBoss(isBoss),
      mPhase2TexturePath(phase2TexturePath),
      mPostAnimState(PLAYER_TURN)
{}

BattleScene::~BattleScene() { shutdown(); }

// -----------------------------------------------------------------------------
// Scene interface
// -----------------------------------------------------------------------------

void BattleScene::initialise()
{
    mGameState.nextSceneID = 0;

    if (mEnemyIsAnimated)
    {
        std::vector<int> frames;
        for (int i = 0; i < 29; i++) frames.push_back(i);
        std::map<Animation, std::vector<int>> atlas;
        for (Animation dir : { ANIM_DOWN, ANIM_DOWN_LEFT, ANIM_LEFT, ANIM_UP_LEFT,
                                ANIM_UP,   ANIM_UP_RIGHT,  ANIM_RIGHT, ANIM_DOWN_RIGHT })
            atlas[dir] = frames;
        mBattleState.enemy = new Entity(
            { ENEMY_CX, ENEMY_CY }, mEnemyDisplaySize,
            mEnemyTexturePath, { 1.0f, 29.0f }, atlas, NPC
        );
        mBattleState.enemy->setMovement({ 0.0f, 1.0f });
    }
    else
    {
        mBattleState.enemy = new Entity(
            { ENEMY_CX, ENEMY_CY }, mEnemyDisplaySize,
            mEnemyTexturePath, NPC
        );
    }
    mBattleState.enemy->setStatic(true);

    mBattleState.playerHP        = mInitPlayerHP;
    mBattleState.playerMaxHP     = mInitPlayerMaxHP;
    mBattleState.playerMP        = mInitPlayerMP;
    mBattleState.playerMaxMP     = mInitPlayerMaxMP;
    mBattleState.enemyHP         = mInitEnemyHP;
    mBattleState.enemyMaxHP      = mInitEnemyHP;
    mBattleState.enemyMinDmg     = mInitEnemyMinDmg;
    mBattleState.enemyMaxDmg     = mInitEnemyMaxDmg;
    mBattleState.turnState       = PLAYER_TURN;
    mBattleState.menuCursor      = 0;
    mBattleState.battleWon       = false;
    mBattleState.potionCount     = mInitPotions;
    mBattleState.batteryCount    = mInitBatteries;
    mBattleState.highPotionCount = mInitHighPotions;
    mBattleState.highEtherCount  = mInitHighEthers;
    mBattleState.fightMult       = mInitFightMult;
    mBattleState.magicMult       = mInitMagicMult;
    mBattleState.inItemMenu          = false;
    mBattleState.itemCursor          = 0;
    mBattleState.isBoss                 = mIsBoss;
    mBattleState.phase2                 = false;
    mBattleState.preTransformPending    = false;
    mBattleState.phase2JustTriggered    = false;
    mBattleState.bossAttackInPhase2     = 0;
    mBattleState.playerHitBossSignal    = false;
    mBattleState.bossBorderAttackSignal = false;

    setDialogue(mIsBoss ? "NEO stands before you!" : "An enemy appeared!", 1.4f);
    mPostAnimState = PLAYER_TURN;
    mBattleState.turnState = ANIMATING;  // opening message before player acts
}

void BattleScene::shutdown()
{
    delete mBattleState.enemy;
    mBattleState.enemy = nullptr;
}

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

void BattleScene::setDialogue(const std::string &text, float duration)
{
    mBattleState.dialogueText  = text;
    mBattleState.dialogueTimer = duration;
}

static void trackBossHit(BattleState &bs)
{
    if (!bs.isBoss || !bs.phase2) return;
    bs.playerHitBossSignal = true;
}

void BattleScene::executePlayerFight()
{
    int base = (mBattleState.fightMult > 1) ? GetRandomValue(12, 19) : GetRandomValue(8, 16);
    int dmg  = base * mBattleState.fightMult;
    mBattleState.enemyHP -= dmg;
    if (mBattleState.enemyHP < 0) mBattleState.enemyHP = 0;
    setDialogue("You attacked for " + std::to_string(dmg) + " damage!");
    mBattleState.playerFightSignal = true;
    trackBossHit(mBattleState);
}

void BattleScene::executePlayerMagic()
{
    if (mBattleState.playerMP < 10)
    {
        setDialogue("Not enough MP!");
        mPostAnimState = PLAYER_TURN;   // back to menu, no enemy turn
        return;
    }
    int dmg = GetRandomValue(23, 38) * mBattleState.magicMult;
    mBattleState.playerMP  -= 10;
    mBattleState.enemyHP   -= dmg;
    if (mBattleState.enemyHP < 0) mBattleState.enemyHP = 0;
    setDialogue("Magic dealt " + std::to_string(dmg) + " damage! (-10 MP)");
    mBattleState.playerMagicSignal = true;
    trackBossHit(mBattleState);
}

void BattleScene::executePlayerItem()
{
    // Opens the item submenu; actual use happens in processInput.
    mBattleState.inItemMenu = true;
    mBattleState.itemCursor = 0;
}

void BattleScene::executeEnemyAttack()
{
    if (mBattleState.isBoss && mBattleState.phase2)
    {
        mBattleState.bossAttackInPhase2++;
        if (mBattleState.bossAttackInPhase2 % 5 == 0)
        {
            mBattleState.bossBorderAttackSignal = true;
            setDialogue("NEO floods your system with corrupted data!");
            return;
        }
    }
    int dmg = GetRandomValue(mBattleState.enemyMinDmg, mBattleState.enemyMaxDmg);
    mBattleState.playerHP -= dmg;
    if (mBattleState.playerHP < 0) mBattleState.playerHP = 0;
    mBattleState.playerJustHit = true;
    setDialogue("Enemy attacked for " + std::to_string(dmg) + " damage!");
}

// -----------------------------------------------------------------------------
// Input -called by main's processInput when gInBattle
// -----------------------------------------------------------------------------

void BattleScene::processInput(bool up, bool down, bool confirm)
{
    if (mBattleState.turnState != PLAYER_TURN) return;

    // -- Item submenu ----------------------------------------------------------
    if (mBattleState.inItemMenu)
    {
        constexpr int ITEM_MENU_COUNT = 5; // Potion, Battery, Hi-Potion, Super Battery, Back
        if (up)
        {
            mBattleState.itemCursor--;
            if (mBattleState.itemCursor < 0) mBattleState.itemCursor = ITEM_MENU_COUNT - 1;
        }
        if (down)
        {
            mBattleState.itemCursor++;
            if (mBattleState.itemCursor >= ITEM_MENU_COUNT) mBattleState.itemCursor = 0;
        }
        if (confirm)
        {
            auto useHeal = [&](int &count, int heal, const char *name) {
                if (count > 0)
                {
                    count--;
                    mBattleState.playerHP += heal;
                    if (mBattleState.playerHP > mBattleState.playerMaxHP)
                        mBattleState.playerHP = mBattleState.playerMaxHP;
                    setDialogue(std::string("Used ") + name + ". +" + std::to_string(heal) + " HP!");
                    mBattleState.inItemMenu = false;
                    mPostAnimState = ENEMY_TURN;
                    mBattleState.turnState = ANIMATING;
                }
                else { setDialogue(std::string("No ") + name + "s left!"); }
            };
            auto useMP = [&](int &count, int lo, int hi, const char *name) {
                if (count > 0)
                {
                    count--;
                    int restore = GetRandomValue(lo, hi);
                    mBattleState.playerMP += restore;
                    if (mBattleState.playerMP > mBattleState.playerMaxMP)
                        mBattleState.playerMP = mBattleState.playerMaxMP;
                    setDialogue(std::string("Used ") + name + ". +" + std::to_string(restore) + " MP!");
                    mBattleState.inItemMenu = false;
                    mPostAnimState = ENEMY_TURN;
                    mBattleState.turnState = ANIMATING;
                }
                else { setDialogue(std::string("No ") + name + "s left!"); }
            };

            switch (mBattleState.itemCursor)
            {
                case 0: useHeal(mBattleState.potionCount,     20,    "Potion");     break;
                case 1: useMP  (mBattleState.batteryCount,    15, 30,"Battery");    break;
                case 2: useHeal(mBattleState.highPotionCount, 60,    "Hi-Potion");  break;
                case 3: useMP  (mBattleState.highEtherCount,  25, 30,"Super Battery"); break;
                case 4: mBattleState.inItemMenu = false;                            break;
            }
        }
        return;
    }

    // -- Main menu -------------------------------------------------------------
    if (up)
    {
        mBattleState.menuCursor--;
        if (mBattleState.menuCursor < 0) mBattleState.menuCursor = MENU_ITEM_COUNT - 1;
    }
    if (down)
    {
        mBattleState.menuCursor++;
        if (mBattleState.menuCursor >= MENU_ITEM_COUNT) mBattleState.menuCursor = 0;
    }

    if (!confirm) return;

    switch (mBattleState.menuCursor)
    {
        case 0:
            mPostAnimState = ENEMY_TURN;
            mBattleState.turnState = ANIMATING;
            executePlayerFight();
            break;
        case 1:
            mPostAnimState = ENEMY_TURN;
            mBattleState.turnState = ANIMATING;
            executePlayerMagic();
            break;
        case 2:
            executePlayerItem();   // opens submenu
            break;
        case 3:
            setDialogue("You fled!", 1.2f);
            mBattleState.battleWon = false;
            mPostAnimState = BATTLE_END;
            mBattleState.turnState = ANIMATING;
            break;
    }
}

// -----------------------------------------------------------------------------
// Update
// -----------------------------------------------------------------------------

void BattleScene::update(float deltaTime)
{
    if (mBattleState.enemy) mBattleState.enemy->update(deltaTime, nullptr, nullptr, nullptr, 0);

    // Step 1 -taunt when HP first drops to/below 500
    if (mBattleState.isBoss && !mBattleState.phase2 && !mBattleState.preTransformPending &&
        mBattleState.enemyHP > 0 && mBattleState.enemyHP <= 500 &&
        mBattleState.turnState == ANIMATING)
    {
        mBattleState.preTransformPending = true;
        setDialogue("I refuse to lose to some Lousy Anti-Virus.\nI am the ULTIMATE INFECTION,\nYOUR PROCESS ENDS HERE!", 4.0f);
        mPostAnimState = PLAYER_TURN;
        return;
    }

    if (mBattleState.turnState != ANIMATING) return;

    mBattleState.dialogueTimer -= deltaTime;
    if (mBattleState.dialogueTimer > 0.0f) return;

    // Step 2 -transform after the taunt dialogue expires
    if (mBattleState.isBoss && mBattleState.preTransformPending && !mBattleState.phase2)
    {
        mBattleState.preTransformPending = false;
        mBattleState.phase2              = true;
        mBattleState.phase2JustTriggered = true;
        mBattleState.enemyHP             = mBattleState.enemyMaxHP;
        mBattleState.enemyMinDmg         = mInitEnemyMinDmg * 3 / 2;
        mBattleState.enemyMaxDmg         = mInitEnemyMaxDmg * 3 / 2;
        if (mPhase2TexturePath && mBattleState.enemy)
            mBattleState.enemy->swapTexture(mPhase2TexturePath);
        setDialogue("NEO OVERLORD has awakened! HP fully restored!", 2.5f);
        mPostAnimState = PLAYER_TURN;
        return;
    }

    // Timer expired - decide what comes next.
    if (mBattleState.enemyHP <= 0)
    {
        if (mBattleState.isBoss && !mBattleState.bossDeathDialogueDone)
        {
            mBattleState.bossDeathDialogueDone = true;
            setDialogue("HOW COULD THIS BE, A SUPERIOR LIFEFORM\nLOSING TO A HUMBLE MACHINE!!!\?\?!?!?\nNOOOOOOOOO!!!!", 4.5f);
            return;
        }
        mBattleState.battleWon = true;
        mBattleState.turnState = BATTLE_END;
        return;
    }
    if (mBattleState.playerHP <= 0)
    {
        setDialogue("You were defeated...", 0.0f);
        mBattleState.battleWon = false;
        mBattleState.turnState = BATTLE_END;
        return;
    }

    // Transition to the queued state.
    if (mPostAnimState == ENEMY_TURN)
    {
        // Execute enemy action immediately and jump straight to ANIMATING again.
        executeEnemyAttack();
        mPostAnimState = PLAYER_TURN;
        // mBattleState.turnState stays ANIMATING; dialogueTimer was reset above.
    }
    else
    {
        mBattleState.turnState = mPostAnimState;
    }
}

// -----------------------------------------------------------------------------
// Render helpers
// -----------------------------------------------------------------------------

void BattleScene::renderEnemy() const
{
    if (mBattleState.enemy) mBattleState.enemy->render();
}

void BattleScene::renderDialogueBox() const
{
    const int PAD   = 14;
    const int BOX_X = 0;
    int bY = (int)BOX_Y;
    int bH = (int)BOX_H;
    int bW = (int)VW;

    // Background panel
    DrawRectangle(BOX_X, bY, bW, bH, { 20, 20, 30, 230 });
    // Border outline
    DrawRectangleLines(BOX_X + 4, bY + 4, bW - 8, bH - 8, { 200, 200, 200, 180 });

    // Multiline dialogue -split on '\n'
    const std::string &text = mBattleState.dialogueText;
    int lineY = bY + PAD * 2;
    size_t pos = 0;
    while (pos <= text.size())
    {
        size_t nl   = text.find('\n', pos);
        size_t len  = (nl == std::string::npos) ? std::string::npos : nl - pos;
        std::string line = text.substr(pos, len);
        DrawText(line.c_str(), BOX_X + PAD * 2, lineY, 24, WHITE);
        lineY += 30;
        if (nl == std::string::npos) break;
        pos = nl + 1;
    }
}

void BattleScene::renderItemSubmenu() const
{
    const int ITEM_H = 30;
    const int PAD    = 10;
    int baseY = (int)BOX_Y + 14;

    struct { const char *label; int count; } entries[5] = {
        { "Potion",        mBattleState.potionCount     },
        { "Dbl-A Battery", mBattleState.batteryCount    },
        { "Hi-Potion",     mBattleState.highPotionCount },
        { "Super Battery",  mBattleState.highEtherCount  },
        { "Back",          -1                            },
    };

    for (int i = 0; i < 5; i++)
    {
        bool selected = (i == mBattleState.itemCursor);
        Color col = selected ? YELLOW : WHITE;
        std::string line = std::string(selected ? "> " : "  ") + entries[i].label;
        if (entries[i].count >= 0)
            line += "  (x" + std::to_string(entries[i].count) + ")";
        DrawText(line.c_str(), (int)MENU_X + PAD + 100, baseY + i * ITEM_H, 22, col);
    }
}

void BattleScene::renderMenuItems() const
{
    if (mBattleState.turnState != PLAYER_TURN) return;

    if (mBattleState.inItemMenu) { renderItemSubmenu(); return; }

    const int ITEM_H = 36;
    const int PAD    = 10;
    int baseY = (int)BOX_Y + 20;

    for (int i = 0; i < MENU_ITEM_COUNT; i++)
    {
        Color col  = (i == mBattleState.menuCursor) ? YELLOW : WHITE;
        const char *prefix = (i == mBattleState.menuCursor) ? "> " : "  ";
        std::string label  = std::string(prefix) + MENU_LABELS[i];
        DrawText(label.c_str(), (int)MENU_X + PAD + 100, baseY + i * ITEM_H, 24, col);
    }
}

void BattleScene::renderStatusBars() const
{
    const int BAR_W = 200, BAR_H = 16;
    const int X = 20;
    int bY = (int)BOX_Y + (int)BOX_H - 70;

    // HP label + bar
    DrawText("HP", X, bY, 20, { 100, 220, 100, 255 });
    DrawRectangle(X + 30, bY + 2, BAR_W, BAR_H, { 60, 60, 60, 200 });
    int hpFill = (mBattleState.playerMaxHP > 0)
               ? (BAR_W * mBattleState.playerHP / mBattleState.playerMaxHP) : 0;
    DrawRectangle(X + 30, bY + 2, hpFill, BAR_H, { 80, 220, 80, 255 });
    DrawText((std::to_string(mBattleState.playerHP) + "/" +
              std::to_string(mBattleState.playerMaxHP)).c_str(),
             X + 240, bY, 18, WHITE);

    bY += 28;
    // MP label + bar
    DrawText("MP", X, bY, 20, { 100, 140, 255, 255 });
    DrawRectangle(X + 30, bY + 2, BAR_W, BAR_H, { 60, 60, 60, 200 });
    int mpFill = (mBattleState.playerMaxMP > 0)
               ? (BAR_W * mBattleState.playerMP / mBattleState.playerMaxMP) : 0;
    DrawRectangle(X + 30, bY + 2, mpFill, BAR_H, { 80, 120, 255, 255 });
    DrawText((std::to_string(mBattleState.playerMP) + "/" +
              std::to_string(mBattleState.playerMaxMP)).c_str(),
             X + 240, bY, 18, WHITE);

    // Enemy HP bar (top-right area)
    int eX = (int)VW - 250, eY = 20;
    DrawText("Enemy HP", eX, eY, 20, { 255, 120, 120, 255 });
    DrawRectangle(eX, eY + 26, 220, BAR_H, { 60, 60, 60, 200 });
    int eFill = (mBattleState.enemyMaxHP > 0)
              ? (220 * mBattleState.enemyHP / mBattleState.enemyMaxHP) : 0;
    DrawRectangle(eX, eY + 26, eFill, BAR_H, { 220, 60, 60, 255 });
    DrawText((std::to_string(mBattleState.enemyHP) + "/" +
              std::to_string(mBattleState.enemyMaxHP)).c_str(),
             eX, eY + 46, 18, WHITE);
}

// -----------------------------------------------------------------------------
// render
// -----------------------------------------------------------------------------

void BattleScene::render()
{
    ClearBackground(ColorFromHex(mBGColourHexCode));

    renderEnemy();
    renderDialogueBox();
    renderMenuItems();
    renderStatusBars();
}
