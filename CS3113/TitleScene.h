#include "Scene.h"

#ifndef TITLE_SCENE_H
#define TITLE_SCENE_H

class TitleScene : public Scene
{
private:
    enum Phase { TITLE, TUTORIAL };

    int       mScreenWidth;
    int       mScreenHeight;
    float     mTime    = 0.0f;
    bool      mDone    = false;
    Phase     mPhase   = TITLE;

    Texture2D mTutorial      = { 0 };
    float     mScrollY       = 0.0f;
    float     mMaxScroll     = 0.0f;
    Music     mTutorialMusic = { 0 };

    static constexpr float SCROLL_SPEED = 10.0f; // px/sec

public:
    TitleScene(int screenW, int screenH);

    void initialise() override;
    void update(float deltaTime) override;
    void render()     override;
    void shutdown()   override;

    void processInput(bool confirm);
    bool isDone() const { return mDone; }
};

#endif // TITLE_SCENE_H
