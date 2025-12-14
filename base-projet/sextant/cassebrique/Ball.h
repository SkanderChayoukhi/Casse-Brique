/*
 * Ball.h - Ball entity for Casse Brique
 *
 * Represents the game ball with physics simulation.
 * Uses Mutex for thread-safe position updates.
 * Runs as a dedicated thread for continuous movement.
 * Implements:
 *  - Wall collision detection
 *  - Paddle collision detection
 *  - Brick collision detection (delegated to GameManager)
 */

#ifndef BALL_H_
#define BALL_H_

#include <sextant/Activite/Threads.h>
#include <sextant/Synchronisation/Mutex/Mutex.h>
#include <sextant/Synchronisation/Semaphore/Semaphore.h>

#define BALL_SIZE 8
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 400
#define GAME_AREA_TOP 20
#define GAME_AREA_BOTTOM 380

class Ball : public Threads
{
private:
    int x, y;                // Ball position
    int dx, dy;              // Ball velocity
    bool active;             // Game status
    Mutex *positionLock;     // Protects x, y, dx, dy
    Semaphore *collisionSem; // Signals collision events

public:
    /**
     * Constructor
     * @param posMutex Pointer to mutex protecting position data
     * @param collisionSem Pointer to semaphore for collision events
     */
    Ball(Mutex *posMutex, Semaphore *collisionSem);

    /**
     * Main thread loop - continuous ball movement and collision detection
     * Implements preemptive scheduling via timer interrupts
     */
    virtual void run();

    /**
     * Thread-safe getters and setters for ball state
     */
    void getPosition(int &outX, int &outY);
    void setPosition(int newX, int newY);
    void setVelocity(int newDx, int newDy);
    void reset();

    int getX() const { return x; }
    int getY() const { return y; }
    int getDx() const { return dx; }
    int getDy() const { return dy; }
    bool isActive() const { return active; }

    /**
     * Collision checking methods
     */
    bool checkWallCollision();
    bool checkPaddleCollision(int paddleX, int paddleY, int paddleWidth, int paddleHeight);
};

#endif
