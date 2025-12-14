#ifndef TEXT_BALL_H
#define TEXT_BALL_H

#include <sextant/Activite/Threads.h>
#include <sextant/Synchronisation/Mutex/Mutex.h>

class TextBall : public Threads
{
private:
    int x, y;
    int dx, dy;
    Mutex *positionMutex;
    bool *gameRunning;

public:
    TextBall(Mutex *mutex, bool *running)
        : x(40), y(12), dx(1), dy(1), positionMutex(mutex), gameRunning(running) {}

    void getPosition(int &outX, int &outY)
    {
        positionMutex->lock();
        outX = x;
        outY = y;
        positionMutex->unlock();
    }

    void setDirection(int newDx, int newDy)
    {
        positionMutex->lock();
        dx = newDx;
        dy = newDy;
        positionMutex->unlock();
    }

    void reset()
    {
        positionMutex->lock();
        x = 40;
        y = 12;
        dx = 1;
        dy = 1;
        positionMutex->unlock();
    }

    virtual void run()
    {
        while (*gameRunning)
        {
            // Small delay
            for (volatile int i = 0; i < 100000; i++)
                ;

            positionMutex->lock();

            // Update position
            x += dx;
            y += dy;

            // Wall collision
            if (x <= 0 || x >= 79)
                dx = -dx;
            if (y <= 0)
                dy = -dy;

            // Lost ball check (will be handled by main)
            if (y >= 24)
            {
                y = 24; // Stop at bottom
            }

            positionMutex->unlock();

            Yield(); // Give other threads a chance
        }
    }
};

#endif
