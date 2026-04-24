#include "cs3113.h"

#ifndef BORDER_THREAT_H
#define BORDER_THREAT_H


class BorderThreat
{
private:
    float mEncroachment;    // 0.0 = clear  …  1.0 = game over
    float mSpeed;           // encroachment units added per real second
    bool  mPaused;
    float mVirtualWidth;
    float mVirtualHeight;

public:
    // At DEFAULT_SPEED the border fills completely in ~40 s.
    static constexpr float DEFAULT_SPEED = 0.025f;

    BorderThreat(float virtualWidth, float virtualHeight,
                 float speed = DEFAULT_SPEED);

    // Call every fixed-timestep tick
    void update(float deltaTime);

    // Draws four inward-growing bars in virtual-screen space.
    void render();

    // Reduce encroachment (call when player wins a battle).
    void pushBack(float amount);

    void pause()  { mPaused = true;  }
    void resume() { mPaused = false; }

    void setEncroachment(float e) { mEncroachment = e < 0.0f ? 0.0f : e > 1.0f ? 1.0f : e; }
    void addEncroachment(float a) { mEncroachment += a; if (mEncroachment > 1.0f) mEncroachment = 1.0f; }

    bool  isGameOver()      const { return mEncroachment >= 1.0f; }
    float getEncroachment() const { return mEncroachment;         }
};

#endif 
