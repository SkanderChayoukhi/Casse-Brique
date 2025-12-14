#include <sextant/cassebrique/Ball.h>

Ball::Ball(Mutex *posMutex, Semaphore *collisionSem)
    : x(320), y(200), dx(2), dy(2), active(true),
      positionLock(posMutex), collisionSem(collisionSem)
{
}

void Ball::run()
{
    while (true)
    {
        // Delay to control ball speed
        for (volatile int i = 0; i < 100000; i++)
            ;

        // Update position with mutex protection
        positionLock->lock();

        x += dx;
        y += dy;

        // Wall collision detection
        if (x <= 0 || x >= SCREEN_WIDTH - BALL_SIZE)
            dx = -dx;

        if (y <= GAME_AREA_TOP)
            dy = -dy;

        // Ball lost at bottom
        if (y > GAME_AREA_BOTTOM)
        {
            active = false;
        }

        positionLock->unlock();

        // Yield to allow other threads to run
        Yield();
    }
}

// Thread-safe getter
void Ball::getPosition(int &outX, int &outY)
{
    positionLock->lock();
    outX = x;
    outY = y;
    positionLock->unlock();
}

// Thread-safe setter
void Ball::setPosition(int newX, int newY)
{
    positionLock->lock();
    x = newX;
    y = newY;
    positionLock->unlock();
}

// Thread-safe velocity setter
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
    x = SCREEN_WIDTH / 2;
    y = SCREEN_HEIGHT / 2;
    dx = 2;
    dy = 2;
    active = true;
    positionLock->unlock();
}

bool Ball::checkWallCollision()
{
    positionLock->lock();
    bool hitWall = (x <= 0 || x >= SCREEN_WIDTH - BALL_SIZE ||
                    y <= GAME_AREA_TOP);
    positionLock->unlock();
    return hitWall;
}

bool Ball::checkPaddleCollision(int paddleX, int paddleY,
                                int paddleWidth, int paddleHeight)
{
    positionLock->lock();

    bool collisionX = (x < paddleX + paddleWidth) &&
                      (x + BALL_SIZE > paddleX);
    bool collisionY = (y < paddleY + paddleHeight) &&
                      (y + BALL_SIZE > paddleY);

    bool collision = collisionX && collisionY;

    positionLock->unlock();

    return collision;
}
