/*
 * Ball.cpp - Ball physics and movement thread
 */

#include <sextant/cassebrique/Ball.h>

Ball::Ball(Mutex *posMutex, Semaphore *collisionSem)
    : x(320), y(200), dx(2), dy(2), active(true),
      positionLock(posMutex), collisionSem(collisionSem)
{
}

void Ball::run()
{
    // Ball movement loop - runs in preemptive thread
    while (active)
    {
        // Delay for animation speed
        for (volatile int i = 0; i < 50000; i++)
            ;

        // Update position with mutex protection
        positionLock->lock();

        x += dx;
        y += dy;

        // Simple wall collisions
        if (x <= 0 || x >= 640 - 32)
            dx = -dx;
        if (y <= 0 || y >= 400 - 32)
            dy = -dy;

        positionLock->unlock();

        // Yield to allow other threads to run
        Yield();
    }
}

void Ball::getPosition(int &outX, int &outY)
{
    positionLock->lock();
    outX = x;
    outY = y;
    positionLock->unlock();
}

void Ball::setPosition(int newX, int newY)
{
    positionLock->lock();
    x = newX;
    y = newY;
    positionLock->unlock();
}

void Ball::setVelocity(int newDx, int newDy)
{
    positionLock->lock();
    dx = newDx;
    dy = newDy;
    positionLock->unlock();
}

void Ball::reset()
{
    positionLock->lock();
    x = 320;
    y = 200;
    dx = 2;
    dy = 2;
    active = true;
    positionLock->unlock();
}
