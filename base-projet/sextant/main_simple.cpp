#include <hal/multiboot.h>
#include <drivers/Ecran.h>

extern char __e_kernel;
extern vaddr_t bootstrap_stack_bottom;
extern size_t bootstrap_stack_size;

extern "C" void Sextant_main(unsigned long magic, unsigned long addr)
{
Ecran ecran;
ecran.effacerEcran(NOIR);
ecran.afficherMot("MINIMAL BOOT TEST", BLANC);
ecran.sautDeLigne();
ecran.afficherMot("System is stable!", VERT);

// Just loop forever
while (1) {
for (volatile int i = 0; i < 10000000; i++);
}
}
