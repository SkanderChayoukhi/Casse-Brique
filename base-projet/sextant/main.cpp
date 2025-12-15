#include <hal/multiboot.h>
#include <drivers/Ecran.h>
#include <drivers/Clavier.h>
#include <drivers/timer.h>
#include <sextant/interruptions/idt.h>
#include <sextant/interruptions/irq.h>
#include <sextant/interruptions/handler/handler_clavier.h>
#include <sextant/memoire/memoire.h>
#include <sextant/ordonnancements/preemptif/thread.h>
#include <sextant/ordonnancements/preemptif/sched.h>
#include <sextant/Activite/Threads.h>
#include <sextant/Synchronisation/Mutex/Mutex.h>
#include <sextant/Synchronisation/Semaphore/Semaphore.h>

extern char __e_kernel;
extern vaddr_t bootstrap_stack_bottom;
extern size_t bootstrap_stack_size;

// NullProcess - prevents scheduler deadlock (from Lab 5)
static void NullProcess(void *) {
    while (1) { thread_yield(); }
}

// ========== GAME STATE (SHARED RESOURCES) ==========
struct GameState {
    // Ball
    int ballX, ballY, ballDx, ballDy;
    // Paddles
    int paddle1X, paddle2X;
    // Game state
    int lives, score;
    int bricksLeft;
    bool running;
    bool gameOver;
    bool won;
    // Bricks (5 rows x 10 columns)
    char bricks[5][10];
};

GameState gameState;
Mutex *gameMutex = 0;        // Protects all shared game state
Semaphore *inputSem = 0;     // Keyboard input coordination
Clavier *keyboard = 0;

// ========== BALL THREAD ==========
class BallThread : public Threads {
public:
    virtual void run() {
        while (gameState.running) {
            // Delay for ball speed
            for (volatile int i = 0; i < 100000; i++);
            
            gameMutex->lock();
            
            if (gameState.gameOver) {
                gameMutex->unlock();
                break;
            }
            
            // Move ball
            gameState.ballX += gameState.ballDx;
            gameState.ballY += gameState.ballDy;
            
            // Wall collisions (left/right)
            if (gameState.ballX <= 0 || gameState.ballX >= 79) {
                gameState.ballDx = -gameState.ballDx;
                gameState.ballX += gameState.ballDx; // Correct position
            }
            
            // Top wall
            if (gameState.ballY <= 1) {
                gameState.ballDy = -gameState.ballDy;
                gameState.ballY = 2;
            }
            
            // Paddle1 collision (player - row 2)
            if (gameState.ballY == 3 && gameState.ballDx != 0) {
                if (gameState.ballX >= gameState.paddle1X && 
                    gameState.ballX < gameState.paddle1X + 10) {
                    gameState.ballDy = -gameState.ballDy;
                }
            }
            
            // Paddle2 collision (AI - row 22)
            if (gameState.ballY == 21 && gameState.ballDy > 0) {
                if (gameState.ballX >= gameState.paddle2X && 
                    gameState.ballX < gameState.paddle2X + 10) {
                    gameState.ballDy = -gameState.ballDy;
                }
            }
            
            // Brick collision (rows 8-12)
            if (gameState.ballY >= 8 && gameState.ballY < 13) {
                int br = gameState.ballY - 8;
                int bc = (gameState.ballX - 15) / 5;
                if (bc >= 0 && bc < 10 && gameState.bricks[br][bc] == '#') {
                    gameState.bricks[br][bc] = ' ';
                    gameState.bricksLeft--;
                    gameState.score += 10;
                    gameState.ballDy = -gameState.ballDy;
                    
                    // Win condition
                    if (gameState.bricksLeft == 0) {
                        gameState.won = true;
                        gameState.gameOver = true;
                    }
                }
            }
            
            // Ball lost (bottom)
            if (gameState.ballY >= 24) {
                gameState.lives--;
                if (gameState.lives <= 0) {
                    gameState.gameOver = true;
                } else {
                    // Reset ball
                    gameState.ballX = 40;
                    gameState.ballY = 12;
                    gameState.ballDx = 1;
                    gameState.ballDy = 1;
                }
            }
            
            gameMutex->unlock();
            Yield(); // Cooperative scheduling
        }
    }
};

// ========== PLAYER PADDLE THREAD ==========
class Paddle1Thread : public Threads {
public:
    virtual void run() {
        while (gameState.running) {
            for (volatile int i = 0; i < 50000; i++);
            
            gameMutex->lock();
            
            if (gameState.gameOver) {
                gameMutex->unlock();
                break;
            }
            
            // Q = left, D = right
            if (keyboard->is_pressed(AZERTY::K_Q) && gameState.paddle1X > 0) {
                gameState.paddle1X--;
            }
            if (keyboard->is_pressed(AZERTY::K_D) && gameState.paddle1X < 70) {
                gameState.paddle1X++;
            }
            
            gameMutex->unlock();
            Yield();
        }
    }
};

// ========== AI PADDLE THREAD ==========
class Paddle2Thread : public Threads {
public:
    virtual void run() {
        while (gameState.running) {
            for (volatile int i = 0; i < 50000; i++);
            
            gameMutex->lock();
            
            if (gameState.gameOver) {
                gameMutex->unlock();
                break;
            }
            
            // Simple AI: follow ball
            int targetX = gameState.ballX - 5; // Center paddle on ball
            if (targetX < gameState.paddle2X && gameState.paddle2X > 0) {
                gameState.paddle2X--;
            } else if (targetX > gameState.paddle2X && gameState.paddle2X < 70) {
                gameState.paddle2X++;
            }
            
            gameMutex->unlock();
            Yield();
        }
    }
};

// ========== MAIN GAME FUNCTION ==========
extern "C" void Sextant_main(unsigned long magic, unsigned long addr)
{
    Ecran ecran;
    ecran.effacerEcran(NOIR);
    ecran.afficherMot("=== CASSE BRIQUE ===", BLANC);
    ecran.sautDeLigne();
    
    // Setup interrupts
    idt_setup();
    irq_setup();
    
    // Setup timer (1000 Hz)
    Timer timer;
    timer.i8254_set_frequency(1000);
    irq_set_routine(IRQ_KEYBOARD, handler_clavier);
    
    // Setup memory
    multiboot_info_t *mbi = (multiboot_info_t *)addr;
    mem_setup(&__e_kernel, (mbi->mem_upper << 10) + (1 << 20), &ecran);
    
    ecran.afficherMot("Initializing threads...", VERT);
    ecran.sautDeLigne();
    
    // Initialize threading subsystem (from Lab 4 & 5)
    thread_subsystem_setup(bootstrap_stack_bottom, bootstrap_stack_size);
    sched_subsystem_setup();
    create_kernel_thread((kernel_thread_start_routine_t)NullProcess, NULL);
    
    ecran.afficherMot("Thread system OK", VERT);
    ecran.sautDeLigne();
    
    // Enable interrupts
    asm volatile("sti\n");
    
    // Enable preemptive scheduling (Lab 4)
    irq_set_routine(IRQ_TIMER, sched_clk);
    
    ecran.afficherMot("Press Q or D to start...", JAUNE);
    ecran.sautDeLigne();
    
    // Create keyboard object
    keyboard = new Clavier();
    
    // Wait for input
    while (!keyboard->is_pressed(AZERTY::K_Q) && !keyboard->is_pressed(AZERTY::K_D)) {
        for (volatile int i = 0; i < 1000000; i++);
    }
    
    // Game loop
    bool playAgain = true;
    
    while (playAgain) {
        // Initialize game state
        gameState.ballX = 40;
        gameState.ballY = 12;
        gameState.ballDx = 1;
        gameState.ballDy = 1;
        gameState.paddle1X = 35;
        gameState.paddle2X = 35;
        gameState.lives = 3;
        gameState.score = 0;
        gameState.bricksLeft = 50;
        gameState.running = true;
        gameState.gameOver = false;
        gameState.won = false;
        
        // Initialize bricks
        for (int r = 0; r < 5; r++) {
            for (int c = 0; c < 10; c++) {
                gameState.bricks[r][c] = '#';
            }
        }
        
        // Create synchronization objects (AFTER threading init - Lab 5)
        Mutex localMutex;
        gameMutex = &localMutex;
        
        Semaphore localSem(0);
        inputSem = &localSem;
        
        ecran.effacerEcran(NOIR);
        ecran.afficherMot("Starting game...", VERT);
        ecran.sautDeLigne();
        
        // Create game threads
        BallThread ballThread;
        Paddle1Thread paddle1Thread;
        Paddle2Thread paddle2Thread;
        
        ballThread.start();
        paddle1Thread.start();
        paddle2Thread.start();
        
        // Main rendering loop
        while (!gameState.gameOver) {
            for (volatile int i = 0; i < 200000; i++);
            
            gameMutex->lock();
            
            // Render
            ecran.effacerEcran(NOIR);
            
            // Display HUD
            ecran.afficherMot("Lives: ", BLANC);
            for (int i = 0; i < gameState.lives; i++) {
                ecran.afficherCaractere(ROUGE, NOIR, '*');
            }
            ecran.afficherMot("  Score: ", BLANC);
            ecran.afficherChiffre(0, 30, gameState.score);
            ecran.afficherMot("  Bricks: ", JAUNE);
            ecran.afficherChiffre(0, 50, gameState.bricksLeft);
            ecran.sautDeLigne();
            
            // Draw paddle 1 (player)
            for (int i = 0; i < 10; i++) {
                ecran.afficherCaractere(2, gameState.paddle1X + i, ROUGE, NOIR, '=');
            }
            
            // Draw paddle 2 (AI)
            for (int i = 0; i < 10; i++) {
                ecran.afficherCaractere(22, gameState.paddle2X + i, BLEU, NOIR, '=');
            }
            
            // Draw bricks
            for (int r = 0; r < 5; r++) {
                for (int c = 0; c < 10; c++) {
                    if (gameState.bricks[r][c] == '#') {
                        ecran.afficherCaractere(8 + r, 15 + c*5, JAUNE, NOIR, '#');
                    }
                }
            }
            
            // Draw ball
            ecran.afficherCaractere(gameState.ballY, gameState.ballX, BLANC, NOIR, 'O');
            
            gameMutex->unlock();
        }
        
        // Stop threads
        gameState.running = false;
        
        // Wait a bit for threads to finish
        for (volatile int i = 0; i < 5000000; i++);
        
        // Display end game message
        ecran.effacerEcran(NOIR);
        if (gameState.won) {
            ecran.afficherMot("*** YOU WIN! ***", VERT);
            ecran.sautDeLigne();
            ecran.afficherMot("All bricks destroyed!", JAUNE);
        } else {
            ecran.afficherMot("*** GAME OVER! ***", ROUGE);
            ecran.sautDeLigne();
            ecran.afficherMot("No lives left!", BLANC);
        }
        ecran.sautDeLigne();
        ecran.afficherMot("Final Score: ", BLANC);
        ecran.afficherChiffre(3, 13, gameState.score);
        ecran.sautDeLigne();
        ecran.sautDeLigne();
        ecran.afficherMot("Press Q to play again, D to quit", JAUNE);
        
        // Wait for input
        bool waiting = true;
        while (waiting) {
            for (volatile int i = 0; i < 1000000; i++);
            if (keyboard->is_pressed(AZERTY::K_Q)) {
                playAgain = true;
                waiting = false;
            } else if (keyboard->is_pressed(AZERTY::K_D)) {
                playAgain = false;
                waiting = false;
            }
        }
    }
    
    ecran.effacerEcran(NOIR);
    ecran.afficherMot("Thanks for playing!", VERT);
    
    while (1);
}
