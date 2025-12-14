/*
 * Brick.h - Breakable brick entity for Casse Brique
 *
 * Represents a single brick on the playing field.
 * Bricks can be destroyed by ball collision.
 * Uses Mutex for thread-safe state changes.
 */

#ifndef BRICK_H_
#define BRICK_H_

#include <sextant/Synchronisation/Mutex/Mutex.h>

class Brick
{
private:
    int x, y;
    int width, height;
    bool destroyed;
    int color;
    Mutex *stateLock;

public:
    /**
     * Default constructor
     */
    Brick();

    /**
     * Constructor - create a brick at given position
     * @param startX X coordinate
     * @param startY Y coordinate
     * @param posMutex Pointer to mutex for synchronization
     * @param col Color index for VGA palette
     */
    Brick(int startX, int startY, Mutex *posMutex, int col = 2);

    /**
     * Check if brick has been destroyed
     * @return true if destroyed, false otherwise
     */
    bool isDestroyed() const;

    /**
     * Mark brick as destroyed (thread-safe)
     */
    void destroy();

    /**
     * Check collision with ball rectangle
     * @param ballX Ball X coordinate
     * @param ballY Ball Y coordinate
     * @param ballSize Size of ball sprite
     * @return true if collision detected
     */
    bool checkCollision(int ballX, int ballY, int ballSize);

    /**
     * Getters
     */
    int getX() const { return x; }
    int getY() const { return y; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    int getColor() const { return color; }
};

#endif
