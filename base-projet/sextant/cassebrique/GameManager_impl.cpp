/*
 * GameManager_impl.cpp
 *
 * Main game coordinator - Casse Brique implementation
 * Manages ball physics, paddle movement, collision detection
 * All synchronized via Mutex and Semaphore (following Lab 5 patterns)
 * All entities run as preemptive threads (following Lab 4 patterns)
 */

#include <sextant/cassebrique/GameManager.h>
#include <sextant/cassebrique/Ball.h>
#include <sextant/cassebrique/Paddle.h>
#include <drivers/EcranBochs.h>
#include <drivers/Clavier.h>
#include <sextant/sprite.h>

extern "C" void game_start()
{
    // Initialize VGA graphics in 8-bit color mode
    EcranBochs screen(640, 400, VBE_MODE::_8);
    Clavier keyboard;

    screen.init();
    screen.clear(0);
    screen.set_palette(palette_vga);
    screen.plot_palette(0, 0, 25);

    // Create synchronization primitives following Lab 5 patterns
    // Mutex: protects shared game state (positions, score)
    Mutex positionMutex;

    // Semaphore: signals collision events (initialized to 0 - blocking)
    Semaphore collisionSem(0);

    // Semaphore: signals keyboard input events
    Semaphore inputSem(0);

    // Create game entities as threads following Lab 4 patterns
    // Each inherits from Threads class and implements run()

    Ball ball(&positionMutex, &collisionSem);
    Paddle paddle1(100, 350, PLAYER_HUMAN, &positionMutex, &inputSem, &keyboard);
    Paddle paddle2(440, 50, PLAYER_AI, &positionMutex, &inputSem, nullptr);

    // Start all game threads (preemptive scheduling via timer interrupt)
    ball.start();
    paddle1.start();
    paddle2.start();

    // Main thread loop - handle rendering and game state
    int x = 0, y = 0;
    while (true)
    {
        // Get ball position safely (protected by mutex)
        ball.getPosition(x, y);

        // Render ball
        screen.clear(0);
        screen.plot_sprite(sprite_data, SPRITE_WIDTH, SPRITE_HEIGHT, x, y);

        // Render paddles
        int p1x = paddle1.getX();
        int p1y = paddle1.getY();
        screen.plot_sprite(sprite_data, SPRITE_WIDTH, SPRITE_HEIGHT, p1x, p1y);

        int p2x = paddle2.getX();
        int p2y = paddle2.getY();
        screen.plot_sprite(sprite_data, SPRITE_WIDTH, SPRITE_HEIGHT, p2x, p2y);

        screen.swapBuffer();

        // Yield to other threads
        thread_yield();
    }
}
