#include "Scene.h"

#ifndef BATTLE_SCENE_H
#define BATTLE_SCENE_H

// -- Turn-state machine --------------------------------------------------------
enum TurnState
{
    PLAYER_TURN,   // menu is live, waiting for player input
    ENEMY_TURN,    // enemy acts (resolved instantly, then → ANIMATING)
    ANIMATING,     // dialogue is displayed; countdown before next turn
    BATTLE_END     // combat over - check didPlayerWin() to know the outcome
};

// -- All mutable battle data in one place -------------------------------------
struct BattleState
{
    Entity   *enemy        = nullptr;
    int       playerHP     = 0;
    int       playerMaxHP  = 0;
    int       playerMP     = 0;
    int       playerMaxMP  = 0;
    int       enemyHP      = 0;
    int       enemyMaxHP   = 0;
    TurnState turnState    = PLAYER_TURN;
    int       menuCursor   = 0;     // 0=Fight 1=Magic 2=Item 3=Run
    bool      battleWon    = false;
    bool      playerRanAway = false;

    // Enemy damage range (set per enemy type)
    int enemyMinDmg = 4;
    int enemyMaxDmg = 10;

    // Player damage multipliers (boosted by Z-Saber)
    int fightMult = 1;
    int magicMult = 1;

    // Item inventory (carried in from overworld, synced back on exit)
    int  potionCount     = 0;
    int  batteryCount    = 0;
    int  highPotionCount = 0;
    int  highEtherCount  = 0;

    // Item submenu state
    bool inItemMenu  = false;
    int  itemCursor  = 0;   // 0=Potion 1=Battery 2=HiPotion 3=HiEther 4=Back

    // One-shot signal: player was hit this tick
    bool playerJustHit    = false;
    bool playerFightSignal = false;
    bool playerMagicSignal = false;

    // Boss-specific state
    bool isBoss                = false;
    bool phase2                = false;
    bool preTransformPending   = false;
    bool phase2JustTriggered   = false;
    int  bossAttackInPhase2    = 0;
    bool playerHitBossSignal    = false;
    bool bossBorderAttackSignal = false;
    bool bossDeathDialogueDone  = false;

    std::string dialogueText;
    float       dialogueTimer = 0.0f;
};


// Coordinates are in virtual-screen space (960 × 720).
// Instantiate, call initialise(), then drive via processInput() + update().
class BattleScene : public Scene
{
private:
    // -- Construction-time config ------------------------------------------
    const char *mEnemyTexturePath;
    Vector2     mEnemyDisplaySize;
    int         mInitEnemyHP;
    int         mInitEnemyMinDmg;
    int         mInitEnemyMaxDmg;
    int         mInitPlayerHP;
    int         mInitPlayerMaxHP;
    int         mInitPlayerMP;
    int         mInitPlayerMaxMP;
    int         mInitPotions;
    int         mInitBatteries;
    int         mInitHighPotions;
    int         mInitHighEthers;
    int         mInitFightMult;
    int         mInitMagicMult;
    bool        mEnemyIsAnimated;
    bool        mIsBoss;
    const char *mPhase2TexturePath;

    // -- Runtime state -----------------------------------------------------
    BattleState mBattleState;
    TurnState   mPostAnimState;   // which turn follows the current ANIMATING phase

    // -- Virtual-screen layout constants ----------------------------------
    static constexpr float VW = 960.0f;
    static constexpr float VH = 720.0f;

    static constexpr float BOX_Y        = VH * 0.60f;   // top of dialogue box
    static constexpr float BOX_H        = VH - BOX_Y;   // height of dialogue box
    static constexpr float MENU_X       = VW * 0.62f;   // left edge of menu column
    static constexpr float ENEMY_CX     = VW * 0.50f;   // enemy sprite centre-x
    static constexpr float ENEMY_CY     = VH * 0.30f;   // enemy sprite centre-y

    // -- Helpers ----------------------------------------------------------
    void setDialogue(const std::string &text, float duration = 1.4f);
    void executePlayerFight();
    void executePlayerMagic();
    void executePlayerItem();
    void executeEnemyAttack();   // called when turnState transitions to ENEMY_TURN

    void renderEnemy()        const;
    void renderDialogueBox()  const;
    void renderMenuItems()    const;
    void renderItemSubmenu()  const;
    void renderStatusBars()   const;

public:
    static constexpr int MENU_ITEM_COUNT = 4;

    // enemyTexturePath  – path to a single PNG for the enemy sprite
    // enemyDisplaySize  – rendered dimensions in virtual pixels
    // enemyHP           – enemy starting HP
    // playerHP/MP       – passed in from the overworld; persists across fights
    BattleScene(Vector2     origin,
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
                int         startPotions      = 0,
                int         startBatteries    = 0,
                int         startHighPotions  = 0,
                int         startHighEthers   = 0,
                int         fightMult         = 1,
                int         magicMult         = 1,
                bool        enemyIsAnimated   = true,
                bool        isBoss            = false,
                const char *phase2TexturePath = nullptr);
    ~BattleScene();

    void initialise() override;
    void update(float deltaTime) override;
    void render()     override;
    void shutdown()   override;

    // Feed per-frame input from main's processInput().
    void processInput(bool up, bool down, bool confirm);

    bool isBattleOver()   const { return mBattleState.turnState == BATTLE_END;  }
    bool didPlayerWin()   const { return mBattleState.battleWon;                }
    bool didPlayerRun()   const { return mBattleState.playerRanAway;            }

    // Expose remaining HP/MP so the overworld can carry them forward.
    int  getPlayerHP()    const { return mBattleState.playerHP;    }
    int  getPlayerMP()    const { return mBattleState.playerMP;    }
    int  getPotionCount()     const { return mBattleState.potionCount;     }
    int  getBatteryCount()    const { return mBattleState.batteryCount;    }
    int  getHighPotionCount() const { return mBattleState.highPotionCount; }
    int  getHighEtherCount()  const { return mBattleState.highEtherCount;  }

    bool consumePlayerHitSignal()
    {
        bool hit = mBattleState.playerJustHit;
        mBattleState.playerJustHit = false;
        return hit;
    }
    bool consumePlayerFightSignal()
    {
        bool v = mBattleState.playerFightSignal;
        mBattleState.playerFightSignal = false;
        return v;
    }
    bool consumePlayerMagicSignal()
    {
        bool v = mBattleState.playerMagicSignal;
        mBattleState.playerMagicSignal = false;
        return v;
    }
    bool consumePhase2Signal()
    {
        bool v = mBattleState.phase2JustTriggered;
        mBattleState.phase2JustTriggered = false;
        return v;
    }
    bool consumePlayerHitBossSignal()
    {
        bool v = mBattleState.playerHitBossSignal;
        mBattleState.playerHitBossSignal = false;
        return v;
    }
    bool consumeBossBorderAttackSignal()
    {
        bool v = mBattleState.bossBorderAttackSignal;
        mBattleState.bossBorderAttackSignal = false;
        return v;
    }

    bool isBossFight() const { return mBattleState.isBoss; }

    BattleState getBattleState() const { return mBattleState; }
};

#endif // BATTLE_SCENE_H
