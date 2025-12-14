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
ecran.afficherMot("=== CASSE BRIQUE GAME ===", BLANC);
ecran.sautDeLigne();

idt_setup();
irq_setup();

Timer timer;
timer.i8254_set_frequency(1000);
irq_set_routine(IRQ_KEYBOARD, handler_clavier);

multiboot_info_t *mbi;
mbi = (multiboot_info_t *)addr;
mem_setup(&__e_kernel, (mbi->mem_upper << 10) + (1 << 20), &ecran);

ecran.afficherMot("Ready! Q=Left D=Right", VERT);

asm volatile("sti\n");

Clavier keyboard;
while (!keyboard.is_pressed(AZERTY::K_Q) && !keyboard.is_pressed(AZERTY::K_D)) {
for (volatile int i = 0; i < 1000000; i++);
}

int ballX = 40, ballY = 12, ballDx = 1, ballDy = 1;
int paddle1X = 35, paddle2X = 35;
int lives = 3, bricksLeft = 50;

char bricks[5][10];
for (int r = 0; r < 5; r++)
for (int c = 0; c < 10; c++)
bricks[r][c] = '#';

while (lives > 0 && bricksLeft > 0) {
for (volatile int i = 0; i < 5000000; i++);

if (keyboard.is_pressed(AZERTY::K_Q) && paddle1X > 0) paddle1X--;
if (keyboard.is_pressed(AZERTY::K_D) && paddle1X < 70) paddle1X++;

if (ballX < paddle2X + 5) paddle2X--;
else if (ballX > paddle2X + 5) paddle2X++;
if (paddle2X < 0) paddle2X = 0;
if (paddle2X > 70) paddle2X = 70;

ballX += ballDx;
ballY += ballDy;

if (ballX <= 0 || ballX >= 79) ballDx = -ballDx;
if (ballY <= 0) ballDy = -ballDy;

if (ballY == 2 && ballX >= paddle1X && ballX < paddle1X + 10) ballDy = -ballDy;
if (ballY == 22 && ballX >= paddle2X && ballX < paddle2X + 10) ballDy = -ballDy;

if (ballY >= 8 && ballY < 13) {
int br = ballY - 8, bc = (ballX - 15) / 5;
if (bc >= 0 && bc < 10 && bricks[br][bc] == '#') {
bricks[br][bc] = ' ';
bricksLeft--;
ballDy = -ballDy;
}
}

if (ballY >= 24) {
lives--;
ballX = 40; ballY = 12; ballDx = 1; ballDy = 1;
}

ecran.effacerEcran(NOIR);
ecran.afficherMot("Lives:", BLANC);
if (lives == 3) ecran.afficherMot(" ***", ROUGE);
else if (lives == 2) ecran.afficherMot(" **", ROUGE);
else ecran.afficherMot(" *", ROUGE);
ecran.afficherMot("  BREAKOUT", VERT);
ecran.sautDeLigne();

for (int i = 0; i < 10; i++) {
ecran.afficherCaractere(2, paddle1X + i, ROUGE, NOIR, '=');
ecran.afficherCaractere(22, paddle2X + i, BLEU, NOIR, '=');
}

for (int r = 0; r < 5; r++)
for (int c = 0; c < 10; c++)
if (bricks[r][c] == '#')
ecran.afficherCaractere(8 + r, 15 + c*5, JAUNE, NOIR, '#');

ecran.afficherCaractere(ballY, ballX, BLANC, NOIR, 'O');
}

ecran.effacerEcran(NOIR);
if (bricksLeft == 0) {
ecran.afficherMot("*** YOU WIN! ***", VERT);
ecran.sautDeLigne();
ecran.afficherMot("All bricks destroyed!", JAUNE);
} else {
ecran.afficherMot("*** GAME OVER! ***", ROUGE);
ecran.sautDeLigne();
ecran.afficherMot("No lives remaining", BLANC);
}

while (1);
}
