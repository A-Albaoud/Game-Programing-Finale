#include "Entity.h"

Entity::Entity()
    : mPosition {0.0f, 0.0f}, mMovement {0.0f, 0.0f},
      mVelocity {0.0f, 0.0f}, mAcceleration {0.0f, 0.0f},
      mScale {DEFAULT_SIZE, DEFAULT_SIZE},
      mColliderDimensions {DEFAULT_SIZE, DEFAULT_SIZE},
      mCurrentTexture {}, mTextureType {SINGLE}, mAngle {0.0f},
      mSpriteSheetDimensions {}, mAnimation {ANIM_DOWN},
      mAnimationIndices {}, mFrameSpeed {0}, mEntityType {EMPTY}
{}

Entity::Entity(Vector2 position, Vector2 scale, const char *textureFilepath,
               EntityType entityType)
    : mPosition {position}, mVelocity {0.0f, 0.0f},
      mAcceleration {0.0f, 0.0f}, mScale {scale}, mMovement {0.0f, 0.0f},
      mColliderDimensions {scale},
      mCurrentTexture {LoadTexture(textureFilepath)},
      mTextureType {SINGLE}, mAnimation {ANIM_DOWN},
      mSpriteSheetDimensions {}, mAnimationIndices {}, mFrameSpeed {0},
      mSpeed {DEFAULT_SPEED}, mAngle {0.0f}, mEntityType {entityType}
{}

Entity::Entity(Vector2 position, Vector2 scale, const char *textureFilepath,
               Vector2 spriteSheetDimensions,
               std::map<Animation, std::vector<int>> animationAtlas,
               EntityType entityType)
    : mPosition {position}, mVelocity {0.0f, 0.0f},
      mAcceleration {0.0f, 0.0f}, mMovement {0.0f, 0.0f}, mScale {scale},
      mColliderDimensions {scale},
      mCurrentTexture {LoadTexture(textureFilepath)},
      mTextureType {ATLAS}, mSpriteSheetDimensions {spriteSheetDimensions},
      mAnimationAtlas {animationAtlas}, mAnimation {ANIM_DOWN},
      mAnimationIndices {animationAtlas.at(ANIM_DOWN)},
      mFrameSpeed {DEFAULT_FRAME_SPEED}, mAngle {0.0f},
      mSpeed {DEFAULT_SPEED}, mEntityType {entityType}
{}

Entity::~Entity()
{
    if (mCurrentTexture.id > 0)
        UnloadTexture(mCurrentTexture);
}

// -- Collision -----------------------------------------------------------------

void Entity::checkCollisionY(Entity *collidableEntities, int collisionCheckCount)
{
    for (int i = 0; i < collisionCheckCount; i++)
    {
        Entity *collidableEntity = &collidableEntities[i];

        if (isColliding(collidableEntity))
        {
            float yDistance = fabs(mPosition.y - collidableEntity->mPosition.y);
            float yOverlap  = fabs(yDistance - (mColliderDimensions.y / 2.0f) -
                              (collidableEntity->mColliderDimensions.y / 2.0f));

            if (mVelocity.y > 0)
            {
                mPosition.y -= yOverlap;
                mVelocity.y  = 0;
                mIsCollidingBottom = true;
            }
            else if (mVelocity.y < 0)
            {
                mPosition.y += yOverlap;
                mVelocity.y  = 0;
                mIsCollidingTop = true;

                if (collidableEntity->mEntityType == BLOCK)
                    collidableEntity->deactivate();
            }
        }
    }
}

void Entity::checkCollisionX(Entity *collidableEntities, int collisionCheckCount)
{
    for (int i = 0; i < collisionCheckCount; i++)
    {
        Entity *collidableEntity = &collidableEntities[i];

        if (isColliding(collidableEntity))
        {
            float yDistance = fabs(mPosition.y - collidableEntity->mPosition.y);
            float yOverlap  = fabs(yDistance - (mColliderDimensions.y / 2.0f) -
                              (collidableEntity->mColliderDimensions.y / 2.0f));

            if (yOverlap < Y_COLLISION_THRESHOLD) continue;

            float xDistance = fabs(mPosition.x - collidableEntity->mPosition.x);
            float xOverlap  = fabs(xDistance - (mColliderDimensions.x / 2.0f) -
                              (collidableEntity->mColliderDimensions.x / 2.0f));

            if (mVelocity.x > 0) {
                mPosition.x      -= xOverlap;
                mVelocity.x       = 0;
                mIsCollidingRight = true;
            } else if (mVelocity.x < 0) {
                mPosition.x     += xOverlap;
                mVelocity.x      = 0;
                mIsCollidingLeft = true;
            }
        }
    }
}

void Entity::checkCollisionY(Map *map)
{
    if (map == nullptr) return;

    Vector2 topCentreProbe    = { mPosition.x, mPosition.y - (mColliderDimensions.y / 2.0f) };
    Vector2 topLeftProbe      = { mPosition.x - (mColliderDimensions.x / 2.0f), mPosition.y - (mColliderDimensions.y / 2.0f) };
    Vector2 topRightProbe     = { mPosition.x + (mColliderDimensions.x / 2.0f), mPosition.y - (mColliderDimensions.y / 2.0f) };

    Vector2 bottomCentreProbe = { mPosition.x, mPosition.y + (mColliderDimensions.y / 2.0f) };
    Vector2 bottomLeftProbe   = { mPosition.x - (mColliderDimensions.x / 2.0f), mPosition.y + (mColliderDimensions.y / 2.0f) };
    Vector2 bottomRightProbe  = { mPosition.x + (mColliderDimensions.x / 2.0f), mPosition.y + (mColliderDimensions.y / 2.0f) };

    float xOverlap = 0.0f;
    float yOverlap = 0.0f;

    if ((map->isSolidTileAt(topCentreProbe, &xOverlap, &yOverlap) ||
         map->isSolidTileAt(topLeftProbe,   &xOverlap, &yOverlap) ||
         map->isSolidTileAt(topRightProbe,  &xOverlap, &yOverlap)) && mVelocity.y < 0.0f)
    {
        mPosition.y    += yOverlap;
        mVelocity.y     = 0.0f;
        mIsCollidingTop = true;
    }

    if ((map->isSolidTileAt(bottomCentreProbe, &xOverlap, &yOverlap) ||
         map->isSolidTileAt(bottomLeftProbe,   &xOverlap, &yOverlap) ||
         map->isSolidTileAt(bottomRightProbe,  &xOverlap, &yOverlap)) && mVelocity.y > 0.0f)
    {
        mPosition.y       -= yOverlap;
        mVelocity.y        = 0.0f;
        mIsCollidingBottom = true;
    }
}

void Entity::checkCollisionX(Map *map)
{
    if (map == nullptr) return;

    Vector2 leftCentreProbe  = { mPosition.x - (mColliderDimensions.x / 2.0f), mPosition.y };
    Vector2 rightCentreProbe = { mPosition.x + (mColliderDimensions.x / 2.0f), mPosition.y };

    float xOverlap = 0.0f;
    float yOverlap = 0.0f;

    if (map->isSolidTileAt(rightCentreProbe, &xOverlap, &yOverlap)
        && mVelocity.x > 0.0f && yOverlap >= 0.5f)
    {
        mPosition.x      -= xOverlap * 1.01f;
        mVelocity.x       = 0.0f;
        mIsCollidingRight = true;
    }

    if (map->isSolidTileAt(leftCentreProbe, &xOverlap, &yOverlap)
        && mVelocity.x < 0.0f && yOverlap >= 0.5f)
    {
        mPosition.x     += xOverlap * 1.01f;
        mVelocity.x      = 0.0f;
        mIsCollidingLeft = true;
    }
}

bool Entity::isColliding(Entity *other) const
{
    if (!other->isActive() || other == this) return false;

    float xDistance = fabs(mPosition.x - other->getPosition().x) -
        ((mColliderDimensions.x + other->getColliderDimensions().x) / 2.0f);
    float yDistance = fabs(mPosition.y - other->getPosition().y) -
        ((mColliderDimensions.y + other->getColliderDimensions().y) / 2.0f);

    return xDistance < 0.0f && yDistance < 0.0f;
}

// -- Animation -----------------------------------------------------------------

void Entity::animate(float deltaTime)
{
    bool moving = GetLength(mMovement) > 0.0f;

    if (moving)
    {
        float mx = mMovement.x, my = mMovement.y;
        bool goLeft  = mx < -0.1f;
        bool goRight = mx >  0.1f;
        bool goUp    = my < -0.1f;
        bool goDown  = my >  0.1f;

        Animation newDir;
        if      (goDown  && goLeft)  newDir = ANIM_DOWN_LEFT;
        else if (goDown  && goRight) newDir = ANIM_DOWN_RIGHT;
        else if (goUp    && goLeft)  newDir = ANIM_UP_LEFT;
        else if (goUp    && goRight) newDir = ANIM_UP_RIGHT;
        else if (goLeft)             newDir = ANIM_LEFT;
        else if (goRight)            newDir = ANIM_RIGHT;
        else if (goDown)             newDir = ANIM_DOWN;
        else                         newDir = ANIM_UP;

        if (newDir != mAnimation)
        {
            mAnimation         = newDir;
            mAnimationIndices  = mAnimationAtlas.at(mAnimation);
            mCurrentFrameIndex = 0;
            mAnimationTime     = 0.0f;
        }

        mAnimationTime += deltaTime;
        float frameInterval = 1.0f / (float)mFrameSpeed;
        if (mAnimationTime >= frameInterval)
        {
            mAnimationTime      = 0.0f;
            mCurrentFrameIndex  = (mCurrentFrameIndex + 1) % (int)mAnimationIndices.size();
        }
    }
    // Not moving: leave mAnimation, mAnimationIndices, mCurrentFrameIndex frozen.
}

// -- AI ------------------------------------------------------------------------

void Entity::AIWander() { moveLeft(); }

void Entity::AIFollow(Entity *target)
{
    switch (mAIState)
    {
    case AI_IDLE:
        if (Vector2Distance(mPosition, target->getPosition()) < 250.0f)
            mAIState = WALKING;
        break;

    case WALKING:
        if (mPosition.x > target->getPosition().x) moveLeft();
        else                                        moveRight();
        break;

    default: break;
    }
}

void Entity::AIActivate(Entity *target)
{
    switch (mAIType)
    {
    case WANDERER: AIWander();       break;
    case FOLLOWER: AIFollow(target); break;
    default: break;
    }
}

void Entity::collideWith(Entity *other)
{
    if (!other || !other->isActive()) return;
    checkCollisionY(other, 1);
    checkCollisionX(other, 1);
}

// -- Update --------------------------------------------------------------------

void Entity::update(float deltaTime, Entity *player, Map *map,
                    Entity *collidableEntities, int collisionCheckCount)
{
    if (mEntityStatus == INACTIVE) return;

    if (mIsStatic)
    {
        if (mTextureType == ATLAS) animate(deltaTime);
        return;
    }

    if (mEntityType == NPC) AIActivate(player);

    resetColliderFlags();

    mVelocity.x = mMovement.x * mSpeed;
    mVelocity.y = mMovement.y * mSpeed;

    mVelocity.x += mAcceleration.x * deltaTime;
    mVelocity.y += mAcceleration.y * deltaTime;

    mPosition.y += mVelocity.y * deltaTime;
    checkCollisionY(collidableEntities, collisionCheckCount);
    checkCollisionY(map);

    mPosition.x += mVelocity.x * deltaTime;
    checkCollisionX(collidableEntities, collisionCheckCount);
    checkCollisionX(map);

    if (mTextureType == ATLAS)
        animate(deltaTime);
}

// -- Render --------------------------------------------------------------------

void Entity::render()
{
    if (mEntityStatus == INACTIVE) return;

    Rectangle textureArea;

    switch (mTextureType)
    {
        case SINGLE:
            textureArea = {
                0.0f, 0.0f,
                static_cast<float>(mCurrentTexture.width),
                static_cast<float>(mCurrentTexture.height)
            };
            break;
        case ATLAS:
            textureArea = getUVRectangle(
                &mCurrentTexture,
                mAnimationIndices[mCurrentFrameIndex],
                mSpriteSheetDimensions.x,
                mSpriteSheetDimensions.y
            );
            break;
        default: break;
    }

    Rectangle destinationArea = {
        mPosition.x, mPosition.y,
        static_cast<float>(mScale.x),
        static_cast<float>(mScale.y)
    };

    Vector2 originOffset = {
        static_cast<float>(mScale.x) / 2.0f,
        static_cast<float>(mScale.y) / 2.0f
    };

    DrawTexturePro(mCurrentTexture, textureArea, destinationArea, originOffset,
                   mAngle, mTint);

    // displayCollider();
}

void Entity::displayCollider()
{
    Rectangle colliderBox = {
        mPosition.x - mColliderDimensions.x / 2.0f,
        mPosition.y - mColliderDimensions.y / 2.0f,
        mColliderDimensions.x,
        mColliderDimensions.y
    };

    DrawRectangleLines(
        (int)colliderBox.x, (int)colliderBox.y,
        (int)colliderBox.width, (int)colliderBox.height,
        GREEN
    );
}
