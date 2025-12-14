/*
 * Paddle.cpp - Paddle movement thread (human and AI)
 */

#include <sextant/cassebrique/Paddle.h>

Paddle::Paddle(int startX, int startY, PaddleType paddleType,
               Mutex *posMutex, Semaphore *inputSem, Clavier *kb)
    : x(startX), y(startY), width(80), height(16),
      type(paddleType), positionLock(posMutex),
      inputSem(inputSem), keyboard(kb), ballX(320), ballY(200)
{
}

void Paddle::run()
{
    // Paddle movement loop - runs in preemptive thread
    while (true)
    {
        // Delay
        for (volatile int i = 0; i < 30000; i++)
            ;

        if (type == PLAYER_HUMAN && keyboard)
        {
            // Human control via keyboard
            positionLock->lock();

            if (keyboard->is_pressed(AZERTY::K_Q))
            {
                x -= 3;
                if (x < 0)
                    x = 0;
            }
            if (keyboard->is_pressed(AZERTY::K_D))
            {
                x += 3;
                if (x > 640 - width)
                    x = 640 - width;
            }

            positionLock->unlock();
        }
        else if (type == PLAYER_AI)
        {
            // AI control - simple tracking of ball
            positionLock->lock();

            int paddleCenter = x + (width / 2);

            if (ballX < paddleCenter - 20)
            {
                x -= 2;
                if (x < 0)
                    x = 0;
            }
            else if (ballX > paddleCenter + 20)
            {
                x += 2;
                if (x > 640 - width)
                    x = 640 - width;
            }

            positionLock->unlock();
        }

        Yield();
    }
}

void Paddle::setPosition(int newX, int newY)
{
    positionLock->lock();
    x = newX;
    y = newY;
    positionLock->unlock();
}

int Paddle::getX() const { return x; }
int Paddle::getY() const { return y; }
int Paddle::getWidth() const { return width; }
int Paddle::getHeight() const { return height; }

void Paddle::updateBallPos(int ballX, int ballY)
{
    this->ballX = ballX;
    this->ballY = ballY;
}
