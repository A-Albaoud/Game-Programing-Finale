#ifndef ENTITY_H
#define ENTITY_H

#include "Map.h"

enum Animation {
    ANIM_DOWN = 0,
    ANIM_DOWN_LEFT,
    ANIM_LEFT,
    ANIM_UP_LEFT,
    ANIM_UP,
    ANIM_UP_RIGHT,
    ANIM_RIGHT,
    ANIM_DOWN_RIGHT
};

enum EntityStatus { ACTIVE, INACTIVE                    };
enum EntityType   { PLAYER, BLOCK, PLATFORM, NPC, EMPTY };
enum AIType       { WANDERER, FOLLOWER                  };
enum AIState      { WALKING, AI_IDLE, FOLLOWING         };

class Entity
{
private:
    Vector2 mPosition;
    Vector2 mMovement;
    Vector2 mVelocity;
    Vector2 mAcceleration;

    Vector2 mScale;
    Vector2 mColliderDimensions;

    Texture2D   mCurrentTexture;
    TextureType mTextureType;

    Vector2 mSpriteSheetDimensions;   // {rows, cols} for the whole sheet

    std::map<Animation, std::vector<int>> mAnimationAtlas;
    std::vector<int> mAnimationIndices;
    Animation mAnimation;
    int mFrameSpeed;

    int   mCurrentFrameIndex = 0;
    float mAnimationTime     = 0.0f;

    bool  mIsJumping    = false;
    float mJumpingPower = 0.0f;

    bool  mIsStatic = false;
    Color mTint     = WHITE;

    int   mSpeed;
    float mAngle;

    bool mIsCollidingTop    = false;
    bool mIsCollidingBottom = false;
    bool mIsCollidingRight  = false;
    bool mIsCollidingLeft   = false;

    EntityStatus mEntityStatus = ACTIVE;
    EntityType   mEntityType;

    AIType  mAIType;
    AIState mAIState;

    bool isColliding(Entity *other) const;

    void checkCollisionY(Entity *collidableEntities, int collisionCheckCount);
    void checkCollisionY(Map *map);

    void checkCollisionX(Entity *collidableEntities, int collisionCheckCount);
    void checkCollisionX(Map *map);

    void resetColliderFlags()
    {
        mIsCollidingTop    = false;
        mIsCollidingBottom = false;
        mIsCollidingRight  = false;
        mIsCollidingLeft   = false;
    }

    void animate(float deltaTime);
    void AIActivate(Entity *target);
    void AIWander();
    void AIFollow(Entity *target);

public:
    static constexpr int   DEFAULT_SIZE          = 250;
    static constexpr int   DEFAULT_SPEED         = 400;
    static constexpr int   DEFAULT_FRAME_SPEED   = 14;
    static constexpr float Y_COLLISION_THRESHOLD = 0.5f;

    Entity();

    // Static / single-texture entity (enemies, props)
    Entity(Vector2 position, Vector2 scale, const char *textureFilepath,
           EntityType entityType);

    // Animated entity — one sprite sheet, per-direction frame lists
    Entity(Vector2 position, Vector2 scale, const char *textureFilepath,
           Vector2 spriteSheetDimensions,
           std::map<Animation, std::vector<int>> animationAtlas,
           EntityType entityType);

    ~Entity();

    void update(float deltaTime, Entity *player, Map *map,
                Entity *collidableEntities, int collisionCheckCount);
    void collideWith(Entity *other);  // single-entity collision pass (used for extra collidables)
    void render();
    void normaliseMovement() { Normalise(&mMovement); }

    void jump()       { mIsJumping    = true;    }
    void activate()   { mEntityStatus = ACTIVE;  }
    void deactivate() { mEntityStatus = INACTIVE; }
    void displayCollider();

    bool isActive() const { return mEntityStatus == ACTIVE; }

    // Movement — animate() derives direction from mMovement each tick
    void moveLeft()  { mMovement.x = -1.0f; }
    void moveRight() { mMovement.x =  1.0f; }
    void moveUp()    { mMovement.y = -1.0f; }
    void moveDown()  { mMovement.y =  1.0f; }

    void resetMovement() { mMovement = { 0.0f, 0.0f }; }

    Vector2     getPosition()           const { return mPosition;           }
    Vector2     getMovement()           const { return mMovement;           }
    Vector2     getVelocity()           const { return mVelocity;           }
    Vector2     getAcceleration()       const { return mAcceleration;       }
    Vector2     getScale()              const { return mScale;              }
    Vector2     getColliderDimensions() const { return mColliderDimensions; }
    Texture2D   getCurrentTexture()     const { return mCurrentTexture;     }
    TextureType getTextureType()        const { return mTextureType;        }
    Animation   getAnimation()          const { return mAnimation;          }
    int         getFrameSpeed()         const { return mFrameSpeed;         }
    float       getJumpingPower()       const { return mJumpingPower;       }
    bool        isJumping()             const { return mIsJumping;          }
    int         getSpeed()              const { return mSpeed;              }
    float       getAngle()              const { return mAngle;              }
    EntityType  getEntityType()         const { return mEntityType;         }
    AIType      getAIType()             const { return mAIType;             }
    AIState     getAIState()            const { return mAIState;            }

    bool isCollidingTop()    const { return mIsCollidingTop;    }
    bool isCollidingBottom() const { return mIsCollidingBottom; }

    void setPosition(Vector2 p)            { mPosition       = p;          }
    void setMovement(Vector2 m)            { mMovement       = m;          }
    void setAcceleration(Vector2 a)        { mAcceleration   = a;          }
    void setScale(Vector2 s)               { mScale          = s;          }
    void setColliderDimensions(Vector2 d)  { mColliderDimensions = d;      }
    void setSpeed(int s)                   { mSpeed          = s;          }
    void setFrameSpeed(int s)              { mFrameSpeed     = s;          }
    void setJumpingPower(float p)          { mJumpingPower   = p;          }
    void setAngle(float a)                 { mAngle          = a;          }
    void setEntityType(EntityType t)       { mEntityType     = t;          }
    void setAIState(AIState s)             { mAIState        = s;          }
    void setAIType(AIType t)               { mAIType         = t;          }
    void setStatic(bool s)                 { mIsStatic       = s;          }
    void setTint(Color c)                  { mTint           = c;          }
    bool isStatic() const                  { return mIsStatic;             }

    void setAnimation(Animation dir)
    {
        mAnimation = dir;
        if (mTextureType == ATLAS)
            mAnimationIndices = mAnimationAtlas.at(mAnimation);
    }

    void swapTexture(const char *filepath)
    {
        if (mCurrentTexture.id > 0) UnloadTexture(mCurrentTexture);
        mCurrentTexture = LoadTexture(filepath);
        mTextureType    = SINGLE;
    }
};

#endif // ENTITY_H
