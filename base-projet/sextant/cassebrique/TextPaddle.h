#ifndef TEXT_PADDLE_H
#define TEXT_PADDLE_H

#include <sextant/Activite/Threads.h>
#include <sextant/Synchronisation/Mutex/Mutex.h>
#include <drivers/Clavier.h>

class TextPaddle : public Threads
{
private:
    int x, y;
    int width;
    Mutex *positionMutex;
    bool *gameRunning;
    Clavier *keyboard;
    bool isHuman;
    int *ballX; // For AI tracking

public:
    TextPaddle(int startY, Mutex *mutex, bool *running, Clavier *kb, bool human, int *bx = nullptr)
        : x(35), y(startY), width(10), positionMutex(mutex),
          gameRunning(running), keyboard(kb), isHuman(human), ballX(bx) {}

    void getPosition(int &outX, int &outY, int &outW)
    {
        positionMutex->lock();
        outX = x;
        outY = y;
        outW = width;
        positionMutex->unlock();
    }

    virtual void run()
    {
        while (*gameRunning)
        {
            for (volatile int i = 0; i < 50000; i++)
                ;

            positionMutex->lock();

            if (isHuman)
            {
                // Human control with Q/D
                if (keyboard->is_pressed(AZERTY::K_Q) && x > 0)
                {
                    x--;
                }
                if (keyboard->is_pressed(AZERTY::K_D) && x < 70)
                {
                    x++;
                }
            }
            else
            {
                // AI control - track ball
                if (ballX != nullptr)
                {
                    if (*ballX < x + 5 && x > 0)
                    {
                        x--;
                    }
                    else if (*ballX > x + 5 && x < 70)
                    {
                        x++;
                    }
                }
            }

            positionMutex->unlock();

            Yield();
        }
    }
};

#endif
