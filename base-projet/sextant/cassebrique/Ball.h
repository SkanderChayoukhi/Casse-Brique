/*
 * Ball.h - Ball entity for Casse Brique
 */

#ifndef BALL_H_
#define BALL_H_

#include <sextant/Activite/Threads.h>
#include <sextant/Synchronisation/Mutex/Mutex.h>
#include <sextant/Synchronisation/Semaphore/Semaphore.h>

class Ball : public Threads
{
private:
    int x, y;
    int dx, dy;
    bool active;
    Mutex *positionLock;
    Semaphore *collisionSem;

public:
    Ball(Mutex *posMutex, Semaphore *collisionSem);
    virtual void run();

    void getPosition(int &outX, int &outY);
    void setPosition(int newX, int newY);
    void setVelocity(int newDx, int newDy);
    void reset();

    int getX() const { return x; }
    int getY() const { return y; }
    bool isActive() const { return active; }
};

#endif
