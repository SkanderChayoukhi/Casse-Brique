#include <hal/multiboot.h>
#include <drivers/Ecran.h>
#include <drivers/Clavier.h>
#include <drivers/timer.h>
#include <sextant/interruptions/idt.h>
#include <sextant/interruptions/irq.h>
#include <sextant/interruptions/handler/handler_clavier.h>
#include <sextant/memoire/memoire.h>

extern char __e_kernel;
extern vaddr_t bootstrap_stack_bottom;
extern size_t bootstrap_stack_size;

extern "C" void Sextant_main(unsigned long magic, unsigned long addr)
{
    Ecran ecran;
    ecran.effacerEcran(NOIR);
    ecran.afficherMot("=== CASSE BRIQUE ===", BLANC);
    ecran.sautDeLigne();
    ecran.sautDeLigne();

    idt_setup();
    irq_setup();

    Timer timer;
    timer.i8254_set_frequency(1000);
    irq_set_routine(IRQ_KEYBOARD, handler_clavier);

    multiboot_info_t *mbi;
    mbi = (multiboot_info_t *)addr;
    mem_setup(&__e_kernel, (mbi->mem_upper << 10) + (1 << 20), &ecran);

    ecran.afficherMot("System OK!", VERT);
    ecran.sautDeLigne();

    asm volatile("sti\n");

    Clavier keyboard;

    // Auto-play game, allow replay without hanging
    bool playAgain = true;
    int gameCount = 0;

    // Tunables for pacing and duration
    const int maxFrames = 300;            // how many frames per round
    const int frameDelayLoops = 20000000; // per-frame delay to slow visuals (~1.6s/frame)

    // Game state variables (declared outside loop so they're accessible after)
    int ballX, ballY, ballDx, ballDy;
    int paddle1X, paddle1Y, paddle2X, paddle2Y, paddleW;
    int lives, score, bricksLeft;
    bool won;
    char bricks[5][10];

    while (playAgain)
    {
        gameCount++;
        int roundNumber = gameCount;
        // TEXT MODE GAME - Initialize
        ballX = 40;
        ballY = 12;
        ballDx = 1;
        ballDy = 1;
        paddle1X = 35;
        paddle1Y = 2;
        paddle2X = 35;
        paddle2Y = 22;
        paddleW = 10;
        lives = 3;
        score = 0;
        bricksLeft = 50;
        won = false;

        for (int r = 0; r < 5; r++)
            for (int c = 0; c < 10; c++)
                bricks[r][c] = '#';

        // Display game title and stats
        ecran.effacerEcran(NOIR);
        ecran.afficherMot("GAME RUNNING", VERT);
        ecran.sautDeLigne();
        ecran.afficherMot("Lives: 3", ROUGE);
        ecran.sautDeLigne();
        ecran.afficherMot("Score: 0", JAUNE);
        ecran.sautDeLigne();
        ecran.afficherMot("Bricks: 50", BLANC);
        ecran.sautDeLigne();
        ecran.afficherMot("Round: ", BLANC);
        ecran.afficherBase(roundNumber, 10, BLANC);
        ecran.sautDeLigne();
        ecran.afficherMot("Press Q/D to move, ball auto-plays", BLANC);

        // Hold this screen for 3 seconds, then continue
        for (volatile int i = 0; i < 15000000; i++)
            ;

        // NOW run the actual game loop - limited frames to keep demo paced
        int frameCount = 0;
        while ((lives > 0 && !won) && frameCount < maxFrames)
        {
            frameCount++;
            bool updateStep = (frameCount % 2 == 0); // slow physics a bit
            if (keyboard.is_pressed(AZERTY::K_Q))
            {
                if (paddle1X > 0)
                    paddle1X--;
            }
            if (keyboard.is_pressed(AZERTY::K_D))
            {
                if (paddle1X < 70)
                    paddle1X++;
            }

            if (updateStep)
            {
                // AI
                if (ballX < paddle2X + 5)
                    paddle2X--;
                else if (ballX > paddle2X + 5)
                    paddle2X++;
                if (paddle2X < 0)
                    paddle2X = 0;
                if (paddle2X > 70)
                    paddle2X = 70;

                // Ball physics
                ballX += ballDx;
                ballY += ballDy;

                if (ballX <= 0 || ballX >= 79)
                    ballDx = -ballDx;
                if (ballY <= 0)
                    ballDy = -ballDy;

                // Paddle collision
                if (ballY == paddle1Y && ballX >= paddle1X && ballX < paddle1X + paddleW)
                    ballDy = -ballDy;
                if (ballY == paddle2Y && ballX >= paddle2X && ballX < paddle2X + paddleW)
                    ballDy = -ballDy;

                // Brick collision
                if (ballY >= 8 && ballY < 13)
                {
                    int br = ballY - 8;
                    int bc = (ballX - 15) / 5;
                    if (bc >= 0 && bc < 10 && bricks[br][bc] == '#')
                    {
                        bricks[br][bc] = ' ';
                        bricksLeft--;
                        score += 10;
                        ballDy = -ballDy;
                        if (bricksLeft == 0)
                            won = true;
                    }
                }

                // Lost ball
                if (ballY >= 24)
                {
                    lives--;
                    ballX = 40;
                    ballY = 12;
                    ballDx = 1;
                    ballDy = 1;
                }
            }

            // RENDER
            ecran.effacerEcran(NOIR);
            ecran.afficherMot("Lives:", BLANC);
            if (lives == 3)
                ecran.afficherMot(" ***", ROUGE);
            else if (lives == 2)
                ecran.afficherMot(" **", ROUGE);
            else
                ecran.afficherMot(" *", ROUGE);
            ecran.afficherMot("  Score:", VERT);
            ecran.afficherBase(score, 10, VERT);
            ecran.afficherMot("  Bricks:", JAUNE);
            ecran.afficherBase(bricksLeft, 10, JAUNE);
            ecran.sautDeLigne();

            // Paddle 1
            for (int i = 0; i < paddleW; i++)
                ecran.afficherCaractere(paddle1Y, paddle1X + i, ROUGE, NOIR, '=');

            // Paddle 2
            for (int i = 0; i < paddleW; i++)
                ecran.afficherCaractere(paddle2Y, paddle2X + i, BLEU, NOIR, '=');

            // Bricks
            for (int r = 0; r < 5; r++)
            {
                for (int c = 0; c < 10; c++)
                {
                    if (bricks[r][c] == '#')
                        ecran.afficherCaractere(8 + r, 15 + c * 5, JAUNE, NOIR, '#');
                }
            }

            // Ball
            ecran.afficherCaractere(ballY, ballX, BLANC, NOIR, 'O');

            // Frame rate limiter (~0.8s per frame)
            for (volatile int frameDelay = 0; frameDelay < frameDelayLoops; frameDelay++)
                ;
        } // END inner game loop (while lives > 0 && !won)

        // Show end game screen
        ecran.effacerEcran(NOIR);
        if (won)
        {
            ecran.afficherMot("*** YOU WIN! ***", VERT);
            ecran.sautDeLigne();
            ecran.afficherMot("All bricks destroyed!", JAUNE);
        }
        else
        {
            ecran.afficherMot("*** GAME OVER! ***", ROUGE);
            ecran.sautDeLigne();
            ecran.afficherMot("No lives left!", BLANC);
        }
        ecran.sautDeLigne();
        ecran.afficherMot("Final Score: ", BLANC);
        ecran.afficherBase(score, 10, BLANC);
        ecran.sautDeLigne();
        ecran.sautDeLigne();
        ecran.afficherMot("Round: ", BLANC);
        ecran.afficherBase(roundNumber, 10, BLANC);
        ecran.sautDeLigne();
        ecran.afficherMot("Thanks for playing!", JAUNE);
        ecran.sautDeLigne();
        ecran.afficherMot("Press D to replay, Q to quit (15s)", BLANC);

        // Poll for replay/quit for ~15 seconds; default is replay
        playAgain = true;
        for (volatile int i = 0; i < 150000000; i++)
        {
            if (keyboard.is_pressed(AZERTY::K_D))
            {
                playAgain = true;
                break;
            }
            if (keyboard.is_pressed(AZERTY::K_Q))
            {
                playAgain = false;
                break;
            }
        }
    } // END outer loop

    // Final infinite loop - kernel still running
    while (1)
        ;
}
