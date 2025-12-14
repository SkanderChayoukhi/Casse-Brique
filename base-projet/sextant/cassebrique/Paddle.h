/*
 * Paddle.h - Paddle entity for Casse Brique
 */

#ifndef PADDLE_H_
#define PADDLE_H_

#include <sextant/Activite/Threads.h>
#include <sextant/Synchronisation/Mutex/Mutex.h>
#include <sextant/Synchronisation/Semaphore/Semaphore.h>
#include <drivers/Clavier.h>

enum PaddleType
{
    PLAYER_HUMAN,
    PLAYER_AI
};

class Paddle : public Threads
{
private:
    int x, y;
    int width, height;
    PaddleType type;
    Mutex *positionLock;
    Semaphore *inputSem;
    Clavier *keyboard;
    int ballX, ballY;

public:
    Paddle(int startX, int startY, PaddleType paddleType,
           Mutex *posMutex, Semaphore *inputSem, Clavier *kb = nullptr);

    virtual void run();

    void setPosition(int newX, int newY);
    int getX() const;
    int getY() const;
    int getWidth() const;
    int getHeight() const;

    void updateBallPos(int ballX, int ballY);
};

#endif
