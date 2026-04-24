#include "BorderThreat.h"
#include <math.h>

BorderThreat::BorderThreat(float virtualWidth, float virtualHeight, float speed)
    : mEncroachment(0.0f), mSpeed(speed), mPaused(false),
      mVirtualWidth(virtualWidth), mVirtualHeight(virtualHeight) {}

void BorderThreat::update(float deltaTime)
{
    if (mPaused) return;

    mEncroachment += mSpeed * deltaTime;
    if (mEncroachment > 1.0f) mEncroachment = 1.0f;
}

void BorderThreat::pushBack(float amount)
{
    mEncroachment -= amount;
    if (mEncroachment < 0.0f) mEncroachment = 0.0f;
}

void BorderThreat::render()
{
    if (mEncroachment <= 0.0f) return;

    // Each bar grows from 0 to half the shorter axis, guaranteeing full
    // coverage (opposite bars meet) at encroachment == 1.0.
    float halfH = mVirtualHeight * 0.5f;
    float halfW = mVirtualWidth  * 0.5f;
    float t     = mEncroachment; // shorthand

    Color bar = BLACK;

    int vW = (int)mVirtualWidth;
    int vH = (int)mVirtualHeight;

    int thickH = (int)(t * halfH);   // top / bottom bar height
    int thickW = (int)(t * halfW);   // left / right bar width

    // Top
    DrawRectangle(0,           0,           vW,     thickH, bar);
    // Bottom
    DrawRectangle(0,           vH - thickH, vW,     thickH, bar);
    // Left  (only the strip between the top/bottom bars to avoid corner overlap)
    DrawRectangle(0,           thickH,      thickW, vH - 2 * thickH, bar);
    // Right
    DrawRectangle(vW - thickW, thickH,      thickW, vH - 2 * thickH, bar);
}
