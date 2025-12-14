#include <sextant/cassebrique/Paddle.h>

#define SCREEN_WIDTH 640
#define MAX_PADDLE_SPEED 3
#define AI_DEADZONE 20

Paddle::Paddle(int startX, int startY, PaddleType paddleType,
               Mutex *posMutex, Semaphore *inputSem, Clavier *kb)
    : x(startX), y(startY), width(80), height(16),
      type(paddleType), positionLock(posMutex),
      inputSem(inputSem), keyboard(kb), ballX(320), ballY(200), direction(0)
{
}

void Paddle::run()
{
    while (true)
    {
        // Delay to control update rate
        for (volatile int i = 0; i < 100000; i++)
            ;

        // Human player input
        if (type == PLAYER_HUMAN && keyboard)
        {
            positionLock->lock();

            if (keyboard->is_pressed(AZERTY::K_Q))
            {
                x -= MAX_PADDLE_SPEED;
                if (x < 0)
                    x = 0;
            }
            if (keyboard->is_pressed(AZERTY::K_D))
            {
                x += MAX_PADDLE_SPEED;
                if (x > SCREEN_WIDTH - width)
                    x = SCREEN_WIDTH - width;
            }

            positionLock->unlock();
        }
        // AI player logic
        else if (type == PLAYER_AI)
        {
            positionLock->lock();

            int paddleCenter = x + (width / 2);
            int ballCenter = ballX + 4;

            if (ballCenter < paddleCenter - AI_DEADZONE)
            {
                x -= MAX_PADDLE_SPEED;
                if (x < 0)
                    x = 0;
            }
            else if (ballCenter > paddleCenter + AI_DEADZONE)
            {
                x += MAX_PADDLE_SPEED;
                if (x > SCREEN_WIDTH - width)
                    x = SCREEN_WIDTH - width;
            }

            positionLock->unlock();
        }

        // Yield to allow other threads to run
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

void Paddle::updateBallPos(int newBallX, int newBallY)
{
    ballX = newBallX;
    ballY = newBallY;
}
