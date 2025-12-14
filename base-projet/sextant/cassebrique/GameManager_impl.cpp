#include <sextant/cassebrique/GameManager.h>
#include <drivers/EcranBochs.h>
#include <drivers/Clavier.h>
#include <sextant/sprite.h>
#include <drivers/Ecran.h>
#include <drivers/PortSerie.h>

extern "C" void game_start()
{
    PortSerie serial;
    serial.ecrireMot("=== CASSE BRIQUE STARTING ===\n");

    if (EcranBochs::VRAM == 0)
    {
        serial.ecrireMot("ERROR: VRAM not mapped!\n");
        Ecran e;
        e.afficherMot("ERROR: VRAM not mapped");
        while (1)
            ;
    }

    serial.ecrireMot("Initializing VGA...\n");

    EcranBochs screen(640, 400, VBE_MODE::_8);
    Clavier keyboard;

    screen.init();
    screen.set_palette(palette_vga);
    screen.clear(0);
    screen.swapBuffer();

    serial.ecrireMot("VGA initialized. Starting game...\n");

    // Enable interrupts (for keyboard only, no timer)
    asm volatile("sti\n");

    // Game variables
    int ballX = 320, ballY = 200;
    int ballDx = 2, ballDy = 2;
    int paddle1X = 50, paddle1Y = 30;
    int paddle2X = 540, paddle2Y = 360;
    int paddleWidth = 80, paddleHeight = 16;
    int ballSize = 8;

    struct SimpleBrick
    {
        int x, y, width, height, color;
        bool active;
    } bricks[12];

    for (int i = 0; i < 12; i++)
    {
        int row = i / 6;
        int col = i % 6;
        bricks[i].x = 30 + col * 100;
        bricks[i].y = 80 + row * 30;
        bricks[i].width = 90;
        bricks[i].height = 25;
        bricks[i].color = 2 + row;
        bricks[i].active = true;
    }

    int lives = 3;
    int frameCount = 0;

    serial.ecrireMot("Entering main game loop...\n");

    while (lives > 0)
    {
        // Delay
        for (volatile int i = 0; i < 50000; i++)
            ;

        frameCount++;
        if (frameCount % 1000 == 0)
        {
            serial.ecrireMot("Still running...\n");
        }

        // Input
        if (keyboard.is_pressed(AZERTY::K_Q))
        {
            paddle1X -= 3;
            if (paddle1X < 0)
                paddle1X = 0;
        }
        if (keyboard.is_pressed(AZERTY::K_D))
        {
            paddle1X += 3;
            if (paddle1X > 640 - paddleWidth)
                paddle1X = 640 - paddleWidth;
        }

        // AI paddle
        int aiCenter = paddle2X + paddleWidth / 2;
        int ballCenter = ballX + ballSize / 2;
        if (ballCenter < aiCenter - 20)
        {
            paddle2X -= 2;
            if (paddle2X < 0)
                paddle2X = 0;
        }
        else if (ballCenter > aiCenter + 20)
        {
            paddle2X += 2;
            if (paddle2X > 640 - paddleWidth)
                paddle2X = 640 - paddleWidth;
        }

        // Ball physics
        ballX += ballDx;
        ballY += ballDy;

        // Wall collisions
        if (ballX <= 0 || ballX >= 640 - ballSize)
            ballDx = -ballDx;
        if (ballY <= 20)
            ballDy = -ballDy;

        // Paddle collisions
        if (ballX < paddle1X + paddleWidth && ballX + ballSize > paddle1X &&
            ballY < paddle1Y + paddleHeight && ballY + ballSize > paddle1Y)
        {
            ballDy = -ballDy;
        }
        if (ballX < paddle2X + paddleWidth && ballX + ballSize > paddle2X &&
            ballY < paddle2Y + paddleHeight && ballY + ballSize > paddle2Y)
        {
            ballDy = -ballDy;
        }

        // Brick collisions
        for (int i = 0; i < 12; i++)
        {
            if (bricks[i].active)
            {
                if (ballX < bricks[i].x + bricks[i].width &&
                    ballX + ballSize > bricks[i].x &&
                    ballY < bricks[i].y + bricks[i].height &&
                    ballY + ballSize > bricks[i].y)
                {
                    bricks[i].active = false;
                    ballDy = -ballDy;
                }
            }
        }

        // Ball lost
        if (ballY > 380)
        {
            lives--;
            ballX = 320;
            ballY = 200;
            ballDx = 2;
            ballDy = 2;
        }

        // Render
        screen.clear(0);

        // Ball
        for (int y = ballY; y < ballY + ballSize && y < 400; y++)
        {
            for (int x = ballX; x < ballX + ballSize && x < 640; x++)
            {
                if (x >= 0 && y >= 0)
                {
                    screen.plot_palette(x, y, 15);
                }
            }
        }

        // Paddle 1
        for (int y = paddle1Y; y < paddle1Y + paddleHeight && y < 400; y++)
        {
            for (int x = paddle1X; x < paddle1X + paddleWidth && x < 640; x++)
            {
                if (x >= 0 && y >= 0)
                {
                    screen.plot_palette(x, y, 4);
                }
            }
        }

        // Paddle 2
        for (int y = paddle2Y; y < paddle2Y + paddleHeight && y < 400; y++)
        {
            for (int x = paddle2X; x < paddle2X + paddleWidth && x < 640; x++)
            {
                if (x >= 0 && y >= 0)
                {
                    screen.plot_palette(x, y, 1);
                }
            }
        }

        // Bricks
        for (int i = 0; i < 12; i++)
        {
            if (bricks[i].active)
            {
                for (int y = bricks[i].y; y < bricks[i].y + bricks[i].height && y < 400; y++)
                {
                    for (int x = bricks[i].x; x < bricks[i].x + bricks[i].width && x < 640; x++)
                    {
                        if (x >= 0 && y >= 0)
                        {
                            screen.plot_palette(x, y, bricks[i].color);
                        }
                    }
                }
            }
        }

        screen.swapBuffer();
    }

    serial.ecrireMot("Game over!\n");
    screen.clear(0);
    screen.swapBuffer();

    while (1)
        ;
}
