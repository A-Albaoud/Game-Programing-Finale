#include "TitleScene.h"
#include <math.h>

TitleScene::TitleScene(int screenW, int screenH)
    : Scene({ 0.0f, 0.0f }, "#000000"),
      mScreenWidth(screenW), mScreenHeight(screenH) {}

void TitleScene::initialise()
{
    mGameState.protag      = nullptr;
    mGameState.map         = nullptr;
    mGameState.nextSceneID = 0;
    mTime    = 0.0f;
    mDone    = false;
    mPhase   = TITLE;
    mScrollY = 0.0f;

    mTutorial      = LoadTexture("assets/game/Tutorial.png");
    mTutorialMusic = LoadMusicStream("assets/game/Intro to game.mp3");
    mTutorialMusic.looping = false;

    // Scale tutorial image to fit window width, 
    if (mTutorial.width > 0)
    {
        float scale      = (float)mScreenWidth / (float)mTutorial.width;
        float scaledH    = mTutorial.height * scale;
        mMaxScroll       = scaledH - mScreenHeight;
        if (mMaxScroll < 0.0f) mMaxScroll = 0.0f;
    }
}

void TitleScene::processInput(bool confirm)
{
    if (!confirm) return;

    if (mPhase == TITLE)
    {
        mPhase   = TUTORIAL;
        mScrollY = 0.0f;
        PlayMusicStream(mTutorialMusic);
    }
    else // TUTORIAL
    {
        StopMusicStream(mTutorialMusic);
        mDone = true;
    }
}

void TitleScene::update(float deltaTime)
{
    mTime += deltaTime;

    if (mPhase == TUTORIAL)
    {
        UpdateMusicStream(mTutorialMusic);
        if (mScrollY < mMaxScroll)
        {
            mScrollY += SCROLL_SPEED * deltaTime;
            if (mScrollY > mMaxScroll) mScrollY = mMaxScroll;
        }
    }
}

void TitleScene::render()
{
    if (mPhase == TITLE)
    {
        ClearBackground({ 10, 8, 20, 255 });

        // -- Title -------------------------------------------------------------
        const char *title     = "ROBOTICS ANTIBIOTICS";
        int         titleSize = 90;
        int         titleW    = MeasureText(title, titleSize);
        DrawText(title,
                 (mScreenWidth - titleW) / 2,
                 mScreenHeight / 3,
                 titleSize,
                 WHITE);

        // -- Blinking prompt ---------------------------------------------------
        if (fmodf(mTime, 1.4f) < 0.7f)
        {
            const char *prompt     = "PRESS ENTER TO START";
            int         promptSize = 32;
            int         promptW    = MeasureText(prompt, promptSize);
            DrawText(prompt,
                     (mScreenWidth - promptW) / 2,
                     mScreenHeight * 2 / 3,
                     promptSize,
                     YELLOW);
        }
    }
    else // TUTORIAL
    {
        ClearBackground(BLACK);

        if (mTutorial.id > 0)
        {
            float scale      = (float)mScreenWidth / (float)mTutorial.width;
            float viewportH  = mScreenHeight / scale;   // texture rows visible at once

            Rectangle src = { 0, mScrollY / scale, (float)mTutorial.width, viewportH };
            Rectangle dst = { 0, 0, (float)mScreenWidth, (float)mScreenHeight };
            DrawTexturePro(mTutorial, src, dst, { 0, 0 }, 0.0f, WHITE);
        }

        // Prompt at bottom
        if (fmodf(mTime, 1.4f) < 0.7f)
        {
            const char *prompt     = "PRESS ENTER OR Z TO CONTINUE";
            int         promptSize = 26;
            int         promptW    = MeasureText(prompt, promptSize);
            DrawText(prompt,
                     (mScreenWidth - promptW) / 2,
                     mScreenHeight - 40,
                     promptSize,
                     YELLOW);
        }
    }
}

void TitleScene::shutdown()
{
    if (mTutorial.id > 0)
    {
        UnloadTexture(mTutorial);
        mTutorial = { 0 };
    }
    StopMusicStream(mTutorialMusic);
    UnloadMusicStream(mTutorialMusic);
}
