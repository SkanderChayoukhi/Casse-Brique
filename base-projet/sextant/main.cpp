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

extern char __e_kernel;
extern vaddr_t bootstrap_stack_bottom;
extern size_t bootstrap_stack_size;

static void NullProcess(void *)
{
	while (1)
	{
		thread_yield();
	}
}

// Global game state with Mutex protection
struct GameState
{
	int ballX, ballY, ballDx, ballDy;
	int paddle1X, paddle2X;
	int lives, bricksLeft;
	char bricks[5][10];
	bool running;
	Mutex gameMutex;
} gameState;

// Ball thread - updates ball physics
class BallThread : public Threads
{
public:
	virtual void run()
	{
		while (gameState.running && gameState.lives > 0 && gameState.bricksLeft > 0)
		{
			for (volatile int i = 0; i < 100000; i++)
				;

			gameState.gameMutex.lock();

			gameState.ballX += gameState.ballDx;
			gameState.ballY += gameState.ballDy;

			if (gameState.ballX <= 0 || gameState.ballX >= 79)
				gameState.ballDx = -gameState.ballDx;
			if (gameState.ballY <= 0)
				gameState.ballDy = -gameState.ballDy;

			// Paddle collisions
			if (gameState.ballY == 2 && gameState.ballX >= gameState.paddle1X && gameState.ballX < gameState.paddle1X + 10)
				gameState.ballDy = -gameState.ballDy;
			if (gameState.ballY == 22 && gameState.ballX >= gameState.paddle2X && gameState.ballX < gameState.paddle2X + 10)
				gameState.ballDy = -gameState.ballDy;

			// Brick collision
			if (gameState.ballY >= 8 && gameState.ballY < 13)
			{
				int br = gameState.ballY - 8, bc = (gameState.ballX - 15) / 5;
				if (bc >= 0 && bc < 10 && gameState.bricks[br][bc] == '#')
				{
					gameState.bricks[br][bc] = ' ';
					gameState.bricksLeft--;
					gameState.ballDy = -gameState.ballDy;
				}
			}

			// Ball lost
			if (gameState.ballY >= 24)
			{
				gameState.lives--;
				gameState.ballX = 40;
				gameState.ballY = 12;
				gameState.ballDx = 1;
				gameState.ballDy = 1;
			}

			gameState.gameMutex.unlock();
			Yield();
		}
	}
};

// Player paddle thread
class Paddle1Thread : public Threads
{
	Clavier *keyboard;

public:
	Paddle1Thread(Clavier *k) : keyboard(k) {}

	virtual void run()
	{
		while (gameState.running && gameState.lives > 0 && gameState.bricksLeft > 0)
		{
			for (volatile int i = 0; i < 50000; i++)
				;

			gameState.gameMutex.lock();
			if (keyboard->is_pressed(AZERTY::K_Q) && gameState.paddle1X > 0)
				gameState.paddle1X--;
			if (keyboard->is_pressed(AZERTY::K_D) && gameState.paddle1X < 70)
				gameState.paddle1X++;
			gameState.gameMutex.unlock();

			Yield();
		}
	}
};

// AI paddle thread
class Paddle2Thread : public Threads
{
public:
	virtual void run()
	{
		while (gameState.running && gameState.lives > 0 && gameState.bricksLeft > 0)
		{
			for (volatile int i = 0; i < 50000; i++)
				;

			gameState.gameMutex.lock();
			if (gameState.ballX < gameState.paddle2X + 5)
				gameState.paddle2X--;
			else if (gameState.ballX > gameState.paddle2X + 5)
				gameState.paddle2X++;
			if (gameState.paddle2X < 0)
				gameState.paddle2X = 0;
			if (gameState.paddle2X > 70)
				gameState.paddle2X = 70;
			gameState.gameMutex.unlock();

			Yield();
		}
	}
};

extern "C" void Sextant_main(unsigned long magic, unsigned long addr)
{
	Ecran ecran;
	ecran.effacerEcran(NOIR);
	ecran.afficherMot("=== CASSE BRIQUE ===", BLANC);
	ecran.sautDeLigne();

	idt_setup();
	irq_setup();

	Timer timer;
	timer.i8254_set_frequency(1000);
	irq_set_routine(IRQ_KEYBOARD, handler_clavier);

	multiboot_info_t *mbi;
	mbi = (multiboot_info_t *)addr;
	mem_setup(&__e_kernel, (mbi->mem_upper << 10) + (1 << 20), &ecran);

	ecran.afficherMot("Initializing threads...", VERT);
	ecran.sautDeLigne();

	// CRITICAL: Initialize threading subsystem BEFORE creating threads
	thread_subsystem_setup(bootstrap_stack_bottom, bootstrap_stack_size);
	sched_subsystem_setup();
	create_kernel_thread((kernel_thread_start_routine_t)NullProcess, NULL);

	ecran.afficherMot("Thread system OK", VERT);
	ecran.sautDeLigne();
	ecran.afficherMot("Ready! Q=Left D=Right", VERT);
	asm volatile("sti\n");

	Clavier keyboard;
	while (!keyboard.is_pressed(AZERTY::K_Q) && !keyboard.is_pressed(AZERTY::K_D))
	{
		for (volatile int i = 0; i < 1000000; i++)
			;
	}

	// Initialize global game state
	gameState.ballX = 40;
	gameState.ballY = 12;
	gameState.ballDx = 1;
	gameState.ballDy = 1;
	gameState.paddle1X = 35;
	gameState.paddle2X = 35;
	gameState.lives = 3;
	gameState.bricksLeft = 50;
	gameState.running = true;

	for (int r = 0; r < 5; r++)
		for (int c = 0; c < 10; c++)
			gameState.bricks[r][c] = '#';

	ecran.afficherMot("Starting threaded game...", VERT);
	ecran.sautDeLigne();

	// Create and start game threads
	BallThread ballThread;
	Paddle1Thread paddle1Thread(&keyboard);
	Paddle2Thread paddle2Thread;

	ballThread.start();
	paddle1Thread.start();
	paddle2Thread.start();

	ecran.afficherMot("Threads started!", VERT);
	ecran.sautDeLigne();

	// Main rendering loop
	while (gameState.lives > 0 && gameState.bricksLeft > 0)
	{
		for (volatile int i = 0; i < 200000; i++)
			;

		gameState.gameMutex.lock();

		// Rendering
		ecran.effacerEcran(NOIR);
		ecran.afficherMot("Lives:", BLANC);
		if (gameState.lives == 3)
			ecran.afficherMot(" ***", ROUGE);
		else if (gameState.lives == 2)
			ecran.afficherMot(" **", ROUGE);
		else
			ecran.afficherMot(" *", ROUGE);
		ecran.afficherMot("  BREAKOUT [THREADED]", VERT);
		ecran.sautDeLigne();

		for (int i = 0; i < 10; i++)
		{
			ecran.afficherCaractere(2, gameState.paddle1X + i, ROUGE, NOIR, '=');
			ecran.afficherCaractere(22, gameState.paddle2X + i, BLEU, NOIR, '=');
		}

		for (int r = 0; r < 5; r++)
			for (int c = 0; c < 10; c++)
				if (gameState.bricks[r][c] == '#')
					ecran.afficherCaractere(8 + r, 15 + c * 5, JAUNE, NOIR, '#');

		ecran.afficherCaractere(gameState.ballY, gameState.ballX, BLANC, NOIR, 'O');

		gameState.gameMutex.unlock();
	}

	gameState.running = false;

	ecran.effacerEcran(NOIR);
	if (gameState.bricksLeft == 0)
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

	while (1)
		;
}
