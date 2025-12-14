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
ecran.afficherMot("Controls: Q=Left D=Right", JAUNE);
ecran.sautDeLigne();
ecran.afficherMot("Press Q or D to start...", BLANC);

asm volatile("sti\n");

Clavier keyboard;

while (!keyboard.is_pressed(AZERTY::K_Q) && !keyboard.is_pressed(AZERTY::K_D)) {
for (volatile int i = 0; i < 1000000; i++);
}

// TEXT MODE GAME
int ballX = 40, ballY = 12;
int ballDx = 1, ballDy = 1;
int paddle1X = 35, paddle1Y = 2;
int paddle2X = 35, paddle2Y = 22;
int paddleW = 10;
int lives = 3;

char bricks[5][10];
for (int r = 0; r < 5; r++)
for (int c = 0; c < 10; c++)
bricks[r][c] = '#';

while (lives > 0) {
for (volatile int i = 0; i < 5000000; i++);

if (keyboard.is_pressed(AZERTY::K_Q)) {
if (paddle1X > 0) paddle1X--;
}
if (keyboard.is_pressed(AZERTY::K_D)) {
if (paddle1X < 70) paddle1X++;
}

// AI
if (ballX < paddle2X + 5) paddle2X--;
else if (ballX > paddle2X + 5) paddle2X++;
if (paddle2X < 0) paddle2X = 0;
if (paddle2X > 70) paddle2X = 70;

// Ball physics
ballX += ballDx;
ballY += ballDy;

if (ballX <= 0 || ballX >= 79) ballDx = -ballDx;
if (ballY <= 0) ballDy = -ballDy;

// Paddle collision
if (ballY == paddle1Y && ballX >= paddle1X && ballX < paddle1X + paddleW) ballDy = -ballDy;
if (ballY == paddle2Y && ballX >= paddle2X && ballX < paddle2X + paddleW) ballDy = -ballDy;

// Brick collision
if (ballY >= 8 && ballY < 13) {
int br = ballY - 8;
int bc = (ballX - 15) / 5;
if (bc >= 0 && bc < 10 && bricks[br][bc] == '#') {
bricks[br][bc] = ' ';
ballDy = -ballDy;
}
}

// Lost ball
if (ballY >= 24) {
lives--;
ballX = 40; ballY = 12;
ballDx = 1; ballDy = 1;
}

// RENDER
ecran.effacerEcran(NOIR);
ecran.afficherMot("Lives:", BLANC);
if (lives == 3) ecran.afficherMot(" ***", ROUGE);
else if (lives == 2) ecran.afficherMot(" **", ROUGE);
else ecran.afficherMot(" *", ROUGE);
ecran.afficherMot("  CASSE BRIQUE", VERT);
ecran.sautDeLigne();

// Paddle 1
for (int i = 0; i < paddleW; i++)
ecran.afficherCaractere(paddle1Y, paddle1X + i, ROUGE, NOIR, '=');

// Paddle 2
for (int i = 0; i < paddleW; i++)
ecran.afficherCaractere(paddle2Y, paddle2X + i, BLEU, NOIR, '=');

// Bricks
for (int r = 0; r < 5; r++) {
for (int c = 0; c < 10; c++) {
if (bricks[r][c] == '#')
ecran.afficherCaractere(8 + r, 15 + c*5, JAUNE, NOIR, '#');
}
}

// Ball
ecran.afficherCaractere(ballY, ballX, BLANC, NOIR, 'O');
}

ecran.effacerEcran(NOIR);
ecran.afficherMot("GAME OVER!", ROUGE);
ecran.sautDeLigne();
ecran.afficherMot("Thanks for playing!", BLANC);

while (1);
}
