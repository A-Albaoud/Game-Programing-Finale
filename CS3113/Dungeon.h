#include "Scene.h"

#ifndef DUNGEON_H
#define DUNGEON_H

class Dungeon : public Scene {
private:
    Entity *mMetaton   = nullptr;
    Entity *mZsaber    = nullptr;
    Entity *mProhobot  = nullptr;
    Entity *mProhobot2 = nullptr;
    Entity *mNeo       = nullptr;

public:
    Dungeon();
    Dungeon(Vector2 origin, const char *bgHexCode);
    ~Dungeon();

    void initialise() override;
    void update(float deltaTime) override;
    void render() override;
    void shutdown() override;

    void switchToMap2();

    Entity* getMetaton()   const { return mMetaton;   }
    Entity* getZsaber()    const { return mZsaber;    }
    Entity* getProhobot()  const { return mProhobot;  }
    Entity* getProhobot2() const { return mProhobot2; }
    Entity* getNeo()       const { return mNeo;       }
    void    consumeZsaber()     { if (mZsaber) mZsaber->deactivate(); }
    void    consumeNeo()        { if (mNeo)    mNeo->deactivate();    }
};

#endif
