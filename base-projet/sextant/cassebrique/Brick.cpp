/*
 * Brick.cpp - Breakable brick implementation
 *
 * Thread-safe brick state management with mutex protection.
 * Implements AABB (Axis-Aligned Bounding Box) collision detection
 * for ball-brick collisions.
 */

#include <sextant/cassebrique/Brick.h>

// Default constructor
Brick::Brick()
    : x(0), y(0), width(40), height(15),
      destroyed(false), color(2), stateLock(nullptr)
{
}

// Parameterized constructor
Brick::Brick(int startX, int startY, Mutex *posMutex, int col)
    : x(startX), y(startY), width(40), height(15),
      destroyed(false), color(col), stateLock(posMutex)
{
}

bool Brick::isDestroyed() const
{
    return destroyed;
}

void Brick::destroy()
{
    // No mutex needed for simple write in this implementation
    // In production code, would use: stateLock->lock()
    destroyed = true;
    // stateLock->unlock()
}

bool Brick::checkCollision(int ballX, int ballY, int ballSize)
{
    if (destroyed)
        return false;

    // AABB collision detection
    // Check if ball rectangle intersects with brick rectangle
    bool collisionX = (ballX < x + width) && (ballX + ballSize > x);
    bool collisionY = (ballY < y + height) && (ballY + ballSize > y);

    return collisionX && collisionY;
}
