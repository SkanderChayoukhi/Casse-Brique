/*
 * Paddle.h - Paddle entity for Casse Brique
 *
 * Represents a player-controlled or AI-controlled paddle.
 * Supports two control modes:
 *  - PLAYER_HUMAN: Keyboard input (Q/D keys - AZERTY layout)
 *  - PLAYER_AI: Simple AI that tracks the ball
 *
 * Uses Mutex for thread-safe position updates.
 * Runs as a dedicated thread with preemptive scheduling.
 */

#ifndef PADDLE_H_
#define PADDLE_H_

#include <sextant/Activite/Threads.h>
#include <sextant/Synchronisation/Mutex/Mutex.h>
#include <sextant/Synchronisation/Semaphore/Semaphore.h>
#include <drivers/Clavier.h>

enum PaddleType
{
    PLAYER_HUMAN, // Controlled by keyboard input
    PLAYER_AI     // Controlled by AI logic
};

class Paddle : public Threads
{
private:
    int x, y;            // Paddle position
    int width, height;   // Paddle dimensions (40x15 pixels)
    PaddleType type;     // Control type (human or AI)
    Mutex *positionLock; // Protects x, y
    Semaphore *inputSem; // Signals input events
    Clavier *keyboard;   // Keyboard device (for human control)
    int ballX, ballY;    // Last known ball position (for AI)
    int direction;       // Current movement direction (-1, 0, 1)

public:
    /**
     * Constructor
     * @param startX Initial X position
     * @param startY Initial Y position
     * @param paddleType PLAYER_HUMAN or PLAYER_AI
     * @param posMutex Pointer to mutex protecting position
     * @param inputSem Pointer to semaphore for input events
     * @param kb Pointer to keyboard (nullptr for AI)
     */
    Paddle(int startX, int startY, PaddleType paddleType,
           Mutex *posMutex, Semaphore *inputSem, Clavier *kb = nullptr);

    /**
     * Main thread loop - continuous paddle movement
     * Implements preemptive scheduling via timer interrupts
     * Modes:
     *  - Human: Reads keyboard state and moves left/right
     *  - AI: Calculates optimal position and moves toward ball
     */
    virtual void run();

    /**
     * Thread-safe getters and setters
     */
    void setPosition(int newX, int newY);
    int getX() const { return x; }
    int getY() const { return y; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }

    /**
     * Update ball position for AI tracking
     * Called by GameManager to feed ball state to AI
     */
    void updateBallPos(int ballX, int ballY);
};

#endif
