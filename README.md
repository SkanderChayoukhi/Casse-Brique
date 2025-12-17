# Casse-Brique



# 🎯 EXPLICATIONS DÉTAILLÉES DES SLIDES - CASSE BRIQUE

## SLIDE 6: Fonction Principale - `threaded_breakout()`

### Speech Développé:

"Maintenant, parlons du cœur du système: la fonction principale `threaded_breakout()`. C'est le point d'entrée de notre jeu où tout commence.

**Qu'est-ce qu'elle fait concrètement?**

Imaginez que vous ouvrez un jeu vidéo. Il ne peut pas juste commencer directement - il faut d'abord:

1. **Initialiser l'état du jeu** - créer la balle, les raquettes, les briques
2. **Lancer les threads** - mettre en marche les différentes tâches
3. **Gérer la boucle de rendu** - afficher le jeu continuellement
4. **Gérer les écrans de fin** - afficher victoire ou défaite

La fonction `threaded_breakout()` fait exactement cela. C'est un **orchestrateur**.

### Structure générale:

```cpp
void threaded_breakout() {
    // ÉTAPE 1: Initialisation
    printf("Initialisation du jeu Breakout...\n");

    // Créer l'état global du jeu
    g_game_state = (BreakoutGameState) {
        .ball = { .x = 320, .y = 200, .vx = 3, .vy = -3 },
        .paddle1 = { .x = 300, .y = 370 },
        .paddle2 = { .x = 300, .y = 20 },
        .score1 = 0,
        .score2 = 0,
        .lives = 3,
        .game_state = GAME_RUNNING,
        .mutex = MUTEX_INITIALIZER
    };

    // ÉTAPE 2: Initialiser les drivers matériel
    vga_init();           // Initialiser VGA
    keyboard_init();      // Initialiser clavier
    timer_init();         // Initialiser timer

    // ÉTAPE 3: Créer les threads
    struct thread *ball_thread = create_kernel_thread(
        (kernel_thread_start_routine_t) ball_physics_thread_func,
        NULL
    );

    struct thread *ai_thread = create_kernel_thread(
        (kernel_thread_start_routine_t) paddle_ai_thread_func,
        NULL
    );

    struct thread *input_thread = create_kernel_thread(
        (kernel_thread_start_routine_t) input_thread_func,
        NULL
    );

    // ÉTAPE 4: Boucle principale de rendu
    while (g_game_state.game_state != GAME_OVER) {
        // Affiche l'état actuel du jeu
        render_game();

        // Petite pause pour ne pas surcharger le CPU
        for (volatile int i = 0; i < 1000000; i++);
    }

    // ÉTAPE 5: Afficher écran de fin
    if (g_game_state.game_state == GAME_WON) {
        display_win_screen();
    } else {
        display_lose_screen();
    }
}
```

### Pourquoi cette architecture?

**Avantage 1: Clarté**
Toute l'initialisation est au même endroit. Quelqu'un qui lit le code peut voir immédiatement:
'Ah, on initialise VGA, clavier, timer, puis on lance 3 threads.'

**Avantage 2: Séquence correcte**
L'ordre est CRUCIAL:

- Si on lance les threads AVANT d'initialiser le jeu, ils accèderont à des données invalides → CRASH!
- Si on initialise VGA APRÈS avoir lancé le rendu, l'écran sera noir → BUG!

**Avantage 3: Contrôle de fin**
La boucle while attend que le jeu se termine. Les threads modifient `g_game_state.game_state` quand victoire/défaite. La boucle s'arrête proprement → pas de hang!

### Point technique important:

Notez qu'on peut avoir **UNE BOUCLE DE RENDU SIMPLE** qui affiche simplement ce que les threads ont calculé. On ne code pas la physique ici - elle est dans `ball_physics_thread()`. On ne code pas les entrées ici - elles sont dans `input_thread()`.

C'est une **séparation des responsabilités**: chaque thread fait UNE chose, la boucle principale gère seulement l'affichage."

---

## SLIDE 7: État Partagé - `g_game_state`

### Speech Développé:

"Passons maintenant au concept FONDAMENTAL de notre système: l'**état partagé du jeu**.

Vous savez, quand plusieurs threads s'exécutent en parallèle, ils doivent se communiquer les informations. Comment?

La solution classique: **une structure globale** contenant TOUTES les informations du jeu.

### Structure complète:

```cpp
typedef struct {
    // --- POSITION & PHYSIQUE ---
    struct {
        int x, y;           // Position en pixels
        int vx, vy;         // Vélocité
        int radius;         // Rayon pour collision
        bool active;        // En jeu?
    } ball;

    // --- PADDLES (RAQUETTES) ---
    struct {
        int x, y;           // Position paddle 1
        int width, height;
        bool moving_left;
        bool moving_right;
    } paddle1;

    struct {
        int x, y;           // Position paddle 2 (IA)
        int width, height;
    } paddle2;

    // --- BRIQUES ---
    struct Brick {
        int x, y;
        bool destroyed;
    } bricks[GRID_ROWS][GRID_COLS];

    // --- SCORE & ÉTAT ---
    int score1, score2;
    int lives;
    GameState game_state;  // MENU, RUNNING, OVER, WON

    // --- SYNCHRONISATION ---
    Mutex game_mutex;      // ← TRÈS IMPORTANT!

} BreakoutGameState;

// Variable GLOBALE
BreakoutGameState g_game_state;
```

### Comment les threads l'utilisent?

**Thread Ball Physics:**

```cpp
void ball_physics_thread_func() {
    while (g_game_state.game_state == GAME_RUNNING) {
        game_mutex.lock();           // Acquiert le verrou

        // Lit l'état
        int old_x = g_game_state.ball.x;
        int old_y = g_game_state.ball.y;

        // Modifie l'état
        g_game_state.ball.x += g_game_state.ball.vx;
        g_game_state.ball.y += g_game_state.ball.vy;

        // Vérifie collisions et met à jour score
        check_collisions(&g_game_state);

        game_mutex.unlock();         // Libère le verrou

        thread_yield();              // Cède le CPU
    }
}
```

**Thread Entrée Clavier:**

```cpp
void input_thread_func() {
    while (g_game_state.game_state == GAME_RUNNING) {
        char key = keyboard_getchar();

        game_mutex.lock();

        if (key == 'Z')  // Z = Gauche
            g_game_state.paddle1.moving_left = true;
        if (key == 'S')  // S = Droite
            g_game_state.paddle1.moving_right = true;

        game_mutex.unlock();
    }
}
```

### La clé: LE MUTEX!

Vous voyez ces `game_mutex.lock()` et `game_mutex.unlock()`?

C'est LA CHOSE LA PLUS IMPORTANTE pour éviter les bugs!

Imaginez sans mutex:

**Scénario cauchemar (RACE CONDITION):**

```
t=0ms    Ball thread:    Lit  g_game_state.ball.x = 100
t=1ms    Input thread:   Lit  g_game_state.ball.x = 100 (STALE!)
t=2ms    Ball thread:    Écrit g_game_state.ball.x = 103
t=3ms    Input thread:   Écrit g_game_state.ball.x = 100 (OVERWRITES!)

Résultat: Position du paddle est erronée, collision manquée! BUG!
```

**Avec mutex (CORRECT):**

```
t=0ms    Ball thread:    lock() ✓ acquis
t=1ms    Input thread:   lock() ✗ ATTEND (bloqué)
t=2ms    Ball thread:    Lit x, calcule, écrit
t=3ms    Ball thread:    unlock()
t=4ms    Input thread:   lock() ✓ acquis (finalement!)
t=5ms    Input thread:   Lit x (valeur fraîche), écrit
t=6ms    Input thread:   unlock()

Résultat: Pas de corruption, tout est cohérent! ✓
```

### Sections critiques:

Une **section critique** est une partie du code où on ACCÈDE au même objet partagé.

Dans notre jeu, les sections critiques sont:

1. **Lecture/écriture ball.position**
   - Utilisée par: Ball physics thread + Input thread (pour collision) + Render
2. **Lecture/écriture paddle positions**
   - Utilisée par: Input thread + Render
3. **Lecture/écriture score**

   - Utilisée par: Ball thread (quand collision avec brique) + Render

4. **Lecture bricks[].destroyed**
   - Utilisée par: Ball thread (collision detection) + Render

CHACUNE de ces sections doit être protégée par `game_mutex.lock()` / `unlock()`."

---

## SLIDE 8: Les 3 Threads du Jeu

### Speech Développé:

"Passons maintenant aux trois threads qui animent notre jeu. Chaque thread a une responsabilité spécifique.

### Thread 1: `ball_physics_thread()`

**Rôle**: Tout ce qui concerne la balle

```cpp
void ball_physics_thread_func() {
    while (g_game_state.game_state == GAME_RUNNING) {
        game_mutex.lock();

        // --- ÉTAPE 1: Mise à jour position ---
        g_game_state.ball.x += g_game_state.ball.vx;
        g_game_state.ball.y += g_game_state.ball.vy;

        // --- ÉTAPE 2: Collision murs ---
        if (g_game_state.ball.x <= 0 ||
            g_game_state.ball.x >= 640) {
            g_game_state.ball.vx = -g_game_state.ball.vx;
        }
        if (g_game_state.ball.y <= 0) {
            g_game_state.ball.vy = -g_game_state.ball.vy;
        }
        if (g_game_state.ball.y >= 400) {
            g_game_state.lives--;
            g_game_state.ball.active = false;
            if (g_game_state.lives <= 0) {
                g_game_state.game_state = GAME_OVER;  // Défaite!
            }
        }

        // --- ÉTAPE 3: Collision paddles ---
        if (collides_with_paddle(&g_game_state.ball,
                                 &g_game_state.paddle1)) {
            g_game_state.ball.vy = -g_game_state.ball.vy;
        }
        if (collides_with_paddle(&g_game_state.ball,
                                 &g_game_state.paddle2)) {
            g_game_state.ball.vy = -g_game_state.ball.vy;
        }

        // --- ÉTAPE 4: Collision briques ---
        for (int r = 0; r < GRID_ROWS; r++) {
            for (int c = 0; c < GRID_COLS; c++) {
                if (!g_game_state.bricks[r][c].destroyed &&
                    collides_with_brick(&g_game_state.ball,
                                       &g_game_state.bricks[r][c])) {
                    g_game_state.bricks[r][c].destroyed = true;
                    g_game_state.ball.vy = -g_game_state.ball.vy;
                    g_game_state.score1++;
                }
            }
        }

        // --- ÉTAPE 5: Vérifier victoire ---
        if (all_bricks_destroyed(&g_game_state)) {
            g_game_state.game_state = GAME_WON;  // Victoire!
        }

        game_mutex.unlock();

        // Cède le CPU au prochain thread
        thread_yield();
    }
}
```

**Ce qu'il fait:**

1. Déplace la balle selon sa vélocité
2. Vérifie les rebonds sur les murs
3. Détecte si balle est perdue (défaite)
4. Vérifie collision avec les deux raquettes
5. Vérifie collision avec chaque brique
6. Incrément le score
7. Vérify victoire (toutes briques cassées)

**Important**: Tout cela est protégé par le `game_mutex`. Aucun autre thread ne peut modifier `g_game_state` pendant qu'on calcule les collisions!

---

### Thread 2: `paddle_ai_thread()`

**Rôle**: Contrôler la raquette 2 (IA) automatiquement

```cpp
void paddle_ai_thread_func() {
    while (g_game_state.game_state == GAME_RUNNING) {
        game_mutex.lock();

        // --- SIMPLE IA: SUIVRE LA BALLE ---

        // Si la balle est à gauche du paddle, va à gauche
        if (g_game_state.ball.x < g_game_state.paddle2.x) {
            g_game_state.paddle2.x -= PADDLE_SPEED;
        }

        // Si la balle est à droite du paddle, va à droite
        if (g_game_state.ball.x > g_game_state.paddle2.x +
                                   g_game_state.paddle2.width) {
            g_game_state.paddle2.x += PADDLE_SPEED;
        }

        // Vérifier limites écran
        if (g_game_state.paddle2.x < 0)
            g_game_state.paddle2.x = 0;
        if (g_game_state.paddle2.x + g_game_state.paddle2.width > 640)
            g_game_state.paddle2.x = 640 - g_game_state.paddle2.width;

        game_mutex.unlock();

        // Petite pause (ralentir l'IA pour pas qu'elle soit trop forte!)
        for (volatile int i = 0; i < 500000; i++);

        thread_yield();
    }
}
```

**Pourquoi cette IA est simple?**

L'objectif du projet n'était pas de faire une IA sophistiquée avec machine learning. C'est une démonstration OS!

Donc on utilise une IA triviale: "suit la balle". Même un enfant peut la battre!

Mais regardez: elle s'exécute CONCURREMMENT avec les autres threads. Elle ne bloque personne. C'est un bon exemple de multitâche!

**Note technique**: On ajoute une petite pause (`for` loop) pour que l'IA ne soit pas trop rapide. Sinon elle attraperait TOUJOURS la balle et le jeu serait ennuyeux!

---

### Thread 3: `input_thread()`

**Rôle**: Lire les entrées clavier et contrôler le paddle 1

```cpp
void input_thread_func() {
    while (g_game_state.game_state == GAME_RUNNING) {
        // Attend une touche (bloquant si clavier vide)
        keyboard_semaphore.P();  // ← Attendre interruption clavier

        char key = keyboard_buffer[keyboard_pos++];

        game_mutex.lock();

        // --- CONTRÔLE PADDLE 1 (JOUEUR HUMAIN) ---
        if (key == 'Z' || key == 'z') {
            // Z = Raquette va à gauche
            g_game_state.paddle1.x -= PADDLE_SPEED;
        }
        else if (key == 'S' || key == 's') {
            // S = Raquette va à droite
            g_game_state.paddle1.x += PADDLE_SPEED;
        }

        // Vérifier limites écran
        if (g_game_state.paddle1.x < 0)
            g_game_state.paddle1.x = 0;
        if (g_game_state.paddle1.x + g_game_state.paddle1.width > 640)
            g_game_state.paddle1.x = 640 - g_game_state.paddle1.width;

        game_mutex.unlock();
    }
}
```

**Point technique intéressant**:

Au lieu de faire du **polling** (demander continuellement 'y a-t-il une touche?'), on utilise un **sémaphore**:

```
Utilisateur appuie Z
    ↓
Clavier génère interruption (IRQ1)
    ↓
handler_clavier() sauvegarde 'Z' dans keyboard_buffer
    ↓
handler_clavier() appelle keyboard_semaphore.V()
    ↓
Input thread se réveille du P() bloquant
    ↓
Lit la touche et met à jour le paddle
```

**Avantage**: Le thread ne consomme pas de CPU en attendant. Il dort vraiment jusqu'à ce qu'une touche arrive!

---

## SLIDE 9: Ordonnancement - Préemptif Round-Robin

### Speech Développé:

"Maintenant, la pièce maîtresse du système: l'**ordonnancement préemptif round-robin**. C'est ce qui permet à nos 3 threads de s'exécuter en parallèle.

### Pourquoi préemptif?

**Équité**

```
Sans préemptif (coopératif):
- Thread Ball calcule les collisions (très long!)
- Ball thread oublie de céder le CPU
- Paddles gelés, input threads bloqués
- Jeu inutilisable! ✗

Avec préemptif:
- Ball thread calcule
- Après 10ms, timer interrompt
- Paddle AI peut s'exécuter
- Après 10ms, timer interrompt
- Input thread peut traiter clavier
- Équité garantie! ✓
```

**Réactivité**

```
Joueur appuie Z
    ↓ IRQ1 clavier → handler stocke 'Z'
    ↓ Input thread se réveille
    ↓ MAIS: Ball thread actuellement en CPU...
    ↓ Timer IRQ0 (10ms) → Context switch!
    ↓ CPU donné à Input thread
    ↓ Input thread traite Z
    ↓ Paddle se déplace

Total latence: < 20ms = Imperceptible à l'humain ✓
```

**Cadence stable**

```
Sans ordonnanceur régulier:
- Parfois pas assez de temps pour rendu
- Parfois Ball thread prend trop de temps
- Jeu lag par à-coups

Avec timer 100Hz (10ms):
- Rendu appelé tous les frames
- Consistent 30 FPS
- Jeu fluide ✓
```

### Configuration système Sextant:

**Algorithme: FIFO Round-Robin**

```
File READY (threads prêts):
┌──────────────┬──────────────┬──────────────┐
│ Ball Thread  │ AI Thread    │ Input Thread │
└──────────────┴──────────────┴──────────────┘
   ↑ CPU actuellement

Toutes les 10ms (IRQ0 Timer):
    sched_clk() est appelée
    ↓
    reschedule() enlève Ball Thread de la tête
    reschedule() l'ajoute à la fin

Nouvelle file:
┌──────────────┬──────────────┬──────────────┐
│ AI Thread    │ Input Thread │ Ball Thread  │
└──────────────┴──────────────┴──────────────┘
   ↑ CPU maintenant

Cela continue... Round-robin!
```

**Le Timer: IRQ0 périodique**

```cpp
// Dans main.cpp
void main() {
    // ...setup IDT, IRQ...

    // Configure le timer 8254 pour 100Hz
    Timer timer;
    timer.i8254_set_frequency(100);  // 1 interruption / 10ms

    // Connecte la routine d'ordonnanceur à l'interruption
    irq_set_routine(IRQ_TIMER, sched_clk);

    // Enable interruptions
    enable_interrupts();

    // Lance les threads
    threaded_breakout();
}
```

**Changement de contexte: automatique sur interruption**

```cpp
// Handler d'interruption timer
void sched_clk(int irq) {
    // Sauvegarde contexte thread courant (EIP, ESP, EBP, registres...)
    struct thread *current = thread_get_current();
    save_context(&current->context);

    // Choisit prochain thread READY
    struct thread *next = reschedule(current, YIELD_MYSELF);
    set_current_thread(next);

    // Restaure contexte du nouveau thread
    restore_context(&next->context);
    // ← Code reprend ici! Dans le nouveau thread!
}
```

Tout cela se passe en **< 1ms**, imperceptible!

### Timeline complète:

```
Sec 0.00  [Ball] calcule position, vérifie collision
Sec 0.05  [Ball] toujours en CPU
Sec 0.10  ← Timer IRQ0! Context switch
          [AI] suit la balle
Sec 0.15  [AI] toujours en CPU
Sec 0.20  ← Timer IRQ0! Context switch
          [Input] attend clavier...
Sec 0.25  (utilisateur appuie Z) ← IRQ1! Handler réveille Input
Sec 0.30  ← Timer IRQ0! Context switch (mais Input déjà prêt)
          [Ball] calcule nouvelle position

Résultat: Tous les threads coexécutent "en parallèle"
          Aucun ne peut bloquer les autres!
```

---

## SLIDE 10: Synchronisation - Le Mutex

### Speech Développé (TRÈS DÉTAILLÉ):

"Ici vient le concept LE PLUS IMPORTANT pour éviter les bugs dans un système multitâche: la **synchronisation avec Mutex**.

### Le problème fondamental:

Imaginez qu'on N'AVAIT PAS de Mutex. Juste la mémoire partagée.

```cpp
// SCÉNARIO CAUCHEMAR (sans synchronisation)

// État partagé
struct {
    int ball_x = 100;
    int ball_y = 200;
} g_state;

// Thread Ball
void ball_physics_thread() {
    while (1) {
        int x = g_state.ball_x;        // t=0: Lit 100
        int new_x = x + 3;             // t=1: Calcule 103

        // ← À CMOMENT: Timer IRQ0! Context switch!

        g_state.ball_x = new_x;        // t=20: Écrit 103 (ATTENDU!)
    }
}

// Thread Collision Detection
void collision_thread() {
    while (1) {
        int x = g_state.ball_x;        // t=5: Lit 100 (valeur stale!)

        if (x > 320) {                 // t=6: Faux! (100 n'est pas > 320)
            // Manqué la collision! BUG!
        }
    }
}

Résultat: Les threads VOIENT des états inconsistants!
```

### Solution: Mutex (Mutual Exclusion Lock)

**Définition simple**: Un verrou qui dit "une seule personne à la fois"

```cpp
class Mutex {
    int value = 1;           // 1 = libre, 0 = occupé
    Spinlock internal_lock;  // Protège le mutex lui-même

    void lock() {
        while (1) {
            internal_lock.Take();  // Atomique!
            if (value == 1) {
                value = 0;
                internal_lock.Release();
                break;  // Acquis!
            }
            internal_lock.Release();
            // Retry...
        }
    }

    void unlock() {
        internal_lock.Take();
        value = 1;
        internal_lock.Release();
    }
};

// MAINTENANT, version CORRECTE:

void ball_physics_thread() {
    while (1) {
        game_mutex.lock();           // ← Attendre mon tour

        // Section critique - je suis SEUL ici!
        int x = g_state.ball_x;
        int new_x = x + 3;

        // ← Timer IRQ0 peut survenir ici
        //   mais context switch ne me touchera pas
        //   car je tiens le mutex!

        g_state.ball_x = new_x;

        game_mutex.unlock();         // ← Lâcher le verrou
    }
}

void collision_thread() {
    while (1) {
        game_mutex.lock();           // ← Attendre ou acquérir immédiatement

        // Maintenant c'EST SÛR de lire
        int x = g_state.ball_x;      // Valeur fraîche et cohérente!

        if (x > 320) {               // Maintenant c'est correct!
            // Traite collision...
        }

        game_mutex.unlock();
    }
}
```

### Dans notre jeu: Sections critiques protégées

**Section critique #1: Mise à jour position balle**

```cpp
// DANS ball_physics_thread:

game_mutex.lock();
{
    // SECTION CRITIQUE
    g_state.ball.x += g_state.ball.vx;
    g_state.ball.y += g_state.ball.vy;

    // Vérifie collision
    if (collides_with_paddle(&g_state.ball, &g_state.paddle1)) {
        g_state.ball.vy = -g_state.ball.vy;
    }

    // Mise à jour score
    if (collides_with_brick(...)) {
        g_state.score1++;
    }
}
game_mutex.unlock();
```

**Section critique #2: Contrôle paddle 1**

```cpp
// DANS input_thread:

game_mutex.lock();
{
    // SECTION CRITIQUE
    if (key == 'Z') {
        g_state.paddle1.x -= SPEED;
    }
    if (key == 'S') {
        g_state.paddle1.x += SPEED;
    }
}
game_mutex.unlock();
```

**Section critique #3: Logique IA**

```cpp
// DANS paddle_ai_thread:

game_mutex.lock();
{
    // SECTION CRITIQUE
    if (g_state.ball.x < g_state.paddle2.x) {
        g_state.paddle2.x -= SPEED;
    }
}
game_mutex.unlock();
```

**Section critique #4: Rendu graphique**

```cpp
// DANS threaded_breakout (boucle de rendu):

game_mutex.lock();
{
    // SECTION CRITIQUE
    render_ball(&g_state.ball);
    render_paddle(&g_state.paddle1);
    render_paddle(&g_state.paddle2);
    render_score(g_state.score1);
}
game_mutex.unlock();
```

### Pourquoi TOUTES ces sections?

Vous pouvez vous demander: \"Mais pourquoi faut-il locker PARTOUT? Le rendu lit juste l'état...\"

**Réponse**: Même UNE LECTURE peut être inconsistante!

```cpp
// Imagine ball.x = 100 initialement

// Thread Ball:
g_state.ball.x = 1023;  // Écrit une grande valeur

// À l'instant du contexte switch (intra-écriture en mémoire):
// ball.x = 0x3FF (1023 en hex)
// Mais l'octet haut et bas ne sont pas synchronized!

// Thread Render lit:
int x = g_state.ball.x;  // Peut lire 0x03FF = 1023?
                          // Ou 0x0000 = 0? (partiellement read!)
                          // UNDEFINED!

// Avec Mutex:
Thread Ball:      lock() → écrit les 4 bytes d'un coup → unlock()
Thread Render:    lock() → lit les 4 bytes coherent → unlock()
                  Pas de partial read! ✓
```

### Pattern TOUJOURS utiliser:

```cpp
game_mutex.lock();
{
    // DO YOUR STUFF HERE
    // Tous les accès à g_state sont sûrs
}
game_mutex.unlock();
```

**Important**: Les sections critiques doivent être COURTES!

- ✓ Bon: lock() → faire 1-2 modifications → unlock()
- ✗ Mauvais: lock() → faire 100 calculs → unlock()

Pourquoi? Parce que pendant qu'un thread tient le mutex, les autres attendent (bloqués). Plus longue la section critique, plus les autres thread doivent attendre!

---

## SLIDE 12: Gestion mémoire et État partagé

### Speech Développé:

"Parlons maintenant de comment on gère la mémoire dans notre système bare-metal. C'est un point critique, car on n'a pas l'allocateur dynamique standard!

### Le problème: Pas d'allocateur fiable en bare-metal

En C/C++ classique, on écrirait:

```cpp
BreakoutGame *game = new BreakoutGame();  // Allocation dynamique
```

**Problème en bare-metal**: L'allocateur dynamic n'existe pas (ou est bugué). Il peut fragmenter la mémoire, causer des leaks, ou simplement crash!

### Solution: Placement-new avec mémoire statique

Au lieu de l'allocateur dynamic, on **alloue la mémoire au compile-time** et on construit l'objet dedans!

```cpp
// --- DANS main.cpp ---

// Allouer la mémoire au compile-time (partie statique)
static unsigned char game_storage[sizeof(BreakoutGame)];
static unsigned char vga_storage[sizeof(EcranBochs)];

// Construire les objets en place
BreakoutGame *g_game = new (game_storage) BreakoutGame();
EcranBochs *g_vga = new (vga_storage) EcranBochs(640, 400, VBE_MODE::_8);

// Pourquoi \"new (address)\"?
// C'est la syntaxe C++ de \"placement-new\"
// Elle appelle le constructeur SANS allocation
// Elle construit directement dans l'adresse fournie
```

**En détail:**

```cpp
// Syntaxe normale (avec allocateur):
BreakoutGame *game = new BreakoutGame();
// Internement:
// 1. malloc(sizeof(BreakoutGame))
// 2. Appelle BreakoutGame::BreakoutGame()
// 3. Retourne le pointeur

// Syntaxe placement-new:
static unsigned char storage[sizeof(BreakoutGame)];
BreakoutGame *game = new (storage) BreakoutGame();
// Internement:
// 1. Pas de malloc! On utilise 'storage' directement
// 2. Appelle BreakoutGame::BreakoutGame() dans 'storage'
// 3. Retourne &storage

// Résultat: Mêmes fonctionnalités, pas d'allocateur!
```

### État partagé: Variable globale

```cpp
// --- VARIABLES GLOBALES DU JEU ---

// État du jeu
BreakoutGameState g_game_state;

// Driver VGA
EcranBochs *g_vga;

// Taille: Déterminée au compile-time
// Adresse: Résolue au link-time
// Persistance: Tout au long du programme
```

**Avantages:**

- ✓ Pas de gestion heap dynamique
- ✓ Adresses fixes (important pour driver VGA!)
- ✓ Pas de fragmentation
- ✓ Pas de risque memory leak

**Inconvénients:**

- ✗ Moins flexible (taille fixée au compile-time)
- ✗ Utilise plus de mémoire (même si non-utilisé)
- ✗ Pas de destruction dynamique (destructeur pas appelé)

### Qu'est-ce qui va dans g_game_state?

```cpp
typedef struct {
    // Position et physique balle
    Ball ball;

    // Positions raquettes
    Paddle paddle1;
    Paddle paddle2;

    // Grille briques
    Brick bricks[GRID_ROWS][GRID_COLS];

    // Score et vies
    int score1, score2;
    int lives;

    // État jeu
    GameState state;  // MENU, RUNNING, PAUSED, OVER, WON

    // SYNCHRONISATION
    Mutex game_mutex;  // ← Protège TOUT ça!
    Semaphore update_sem;

} BreakoutGameState;

// Accès depuis n'importe où:
extern BreakoutGameState g_game_state;

// Usage:
g_game_state.ball.x += g_game_state.ball.vx;
g_game_state.score1++;
// ...etc
```

### Scope/visibilité:

```cpp
// --- main.cpp ---
BreakoutGameState g_game_state;  // Définition, allocation
void threaded_breakout() {
    g_game_state.score1++;  // Accès direct
}

// --- ball.cpp ---
extern BreakoutGameState g_game_state;  // Déclaration \"externe\"
void ball_physics_thread() {
    g_game_state.ball.x += 1;  // Accès au même objet global
}

// --- input.cpp ---
extern BreakoutGameState g_game_state;
void input_thread() {
    g_game_state.paddle1.x = 100;  // Même objet!
}
```

Tous les fichiers source voient le **même** objet `g_game_state`. C'est le cœur de la communication inter-threads!

---

## SLIDE 14: VGA et Double Buffering

### Speech Développé:

"Parlons maintenant du rendu graphique. Comment on affiche le jeu à l'écran sans scintiller?

### Le problème: Scintillement sans double buffering

Imaginez ce qui se passe en temps réel:

```
t=0ms   [Render thread] Efface l'écran (tous pixels noirs)
        [Moniteur lit VRAM] Voit écran noir!

t=2ms   [Render thread] Dessine la balle
        [Moniteur lit VRAM] Voit balle seule (sans paddles)

t=4ms   [Render thread] Dessine les paddles
        [Moniteur lit VRAM] Voit balle + paddles (mais pas briques!)

t=6ms   [Render thread] Dessine les briques
        [Moniteur lit VRAM] Voit image complète ENFIN

t=10ms  [Context switch] ← Avant que Render soit fini!
        [Render thread] s'arrête
        [Render thread] reprend...

Résultat: Scintillement visible!
L'utilisateur voit l'image se construire ligne par ligne!
C'est désagréable, très \"flicker-y\"
```

### Solution: Double Buffering

L'idée géniale: **Deux buffers à la place d'un!**

```
┌─────────────────────────────────────────────┐
│          VRAM Écran (2 buffers)             │
├─────────────────────────────────────────────┤
│                                             │
│  Buffer 0:     Buffer 1:                    │
│  ┌──────────┐  ┌──────────┐                 │
│  │          │  │          │                 │
│  │ Image    │  │ Image    │                 │
│  │ Actuelle │  │ Suivante │                 │
│  │ Affichée │  │ Prepare  │                 │
│  │          │  │          │                 │
│  └──────────┘  └──────────┘                 │
│  ← Moniteur    → Render thread dessine      │
│    regarde ici  ici                         │
│                                             │
└─────────────────────────────────────────────┘

```

**Architecture dans EcranBochs:**

```cpp
class EcranBochs {
    ui8_t* buffer0;      // Buffer visible (affichage)
    ui8_t* buffer1;      // Buffer caché (rendu)
    bool current_buffer; // 0 = buffer0 visible, 1 = buffer1 visible

    void swapBuffer() {
        if (current_buffer == 0) {
            current_buffer = 1;
            set_vbe_bank(1);  // Registre VBE pointe buffer1
        } else {
            current_buffer = 0;
            set_vbe_bank(0);  // Registre VBE pointe buffer0
        }
    }
};
```

### Pipeline de rendu:

```cpp
void threaded_breakout() {
    while (g_game_state.game_state != GAME_OVER) {
        // Étape 1: Efface le buffer caché
        g_vga->clear(1);  // 1 = buffer caché

        // Étape 2: Dessine tout dans le buffer caché
        game_mutex.lock();

        // Dessine balle
        g_vga->plot_sprite(
            sprite_ball,
            g_game_state.ball.x,
            g_game_state.ball.y
        );

        // Dessine paddles
        g_vga->plot_sprite(
            sprite_paddle,
            g_game_state.paddle1.x,
            g_game_state.paddle1.y
        );

        g_vga->plot_sprite(
            sprite_paddle,
            g_game_state.paddle2.x,
            g_game_state.paddle2.y
        );

        // Dessine briques
        for (int r = 0; r < GRID_ROWS; r++) {
            for (int c = 0; c < GRID_COLS; c++) {
                if (!g_game_state.bricks[r][c].destroyed) {
                    g_vga->plot_sprite(
                        sprite_brick,
                        BRICK_X(c),
                        BRICK_Y(r)
                    );
                }
            }
        }

        // Affiche score et vies
        render_text_score(g_game_state.score1);

        game_mutex.unlock();

        // Étape 3: ← Moment critique!
        // Échange les buffers instantanément
        g_vga->swapBuffer();

        // À partir d'ici, le moniteur affiche le nouveau buffer
        // Et le render thread dessine dans l'ancien buffer (maintenant caché)

        // Petit délai pour respecter la cadence
        for (volatile int i = 0; i < 1000000; i++);
    }
}
```

### Ce qui se passe concrètement:

```
AVANT (t=0ms):
  Écran montre: Buffer 0 (ancienne frame)
  Render prépare: Buffer 1

t=2ms - Étape 1: Effacer Buffer 1
  Écran montre: Buffer 0 (ancienne frame) ← Utilisateur voit ça
  Render dessine: Buffer 1 (en construction)

t=4ms - Étape 2a: Dessine balle dans Buffer 1
  Écran montre: Buffer 0 ← Utilisateur TOUJOURS voit Buffer 0!
  Render : Ball dans Buffer 1

t=6ms - Étape 2b: Dessine paddles dans Buffer 1
  Écran montre: Buffer 0 ← Pas changé!
  Render : Ball + Paddles dans Buffer 1

t=8ms - Étape 3: SWAP BUFFERS! ← Moment magique!
  Avant swap:
    Écran montrait: Buffer 0
  Après swap:
    Écran montre: Buffer 1 (image COMPLÈTE) ← Instantané!
  Render prépare: Buffer 0 (pour next frame)

t=10ms - Étape 1 (next frame): Effacer Buffer 0
  Écran montre: Buffer 1 (frame N) ← Utilisateur voit ça
  Render dessine: Buffer 0 (frame N+1)

Résultat: Pas de scintillement!
L'utilisateur ne voit que des images complètes!
```

### Timing critique:

Le moment CLÉR est le SWAP. En VGA, c'est ultra-rapide:

```cpp
void swapBuffer() {
    // Écriture dans le registre VBE
    outl(VBE_DISPI_IOPORT_INDEX, VBE_DISPI_Y_OFFSET);
    outl(VBE_DISPI_IOPORT_DATA, (current_buffer ? HEIGHT : 0));

    // Cette écriture est INSTANTANÉE au niveau moniteur
    // (Prochaine refresh de l'écran utilise le nouveau buffer)

    // Passer au prochain buffer
    current_buffer = !current_buffer;
}

// Durée: < 1 microseconde!
// Le moniteur rafraîchit ~60Hz = tous les 16ms
// Swap intervient entre deux rafraîchissements
// → Pas de visual glitch!
```

### Résultat visuel:

**Sans double buffering:**

```
Frame N:  ▓ ▓ ▓▓ □ ▓ ← Utilisateur voit construction
          ░░░░░░░
Frame N+1:▓░▓▓▓░▓
```

**Avec double buffering:**

```
Frame N:  ▓▓▓▓▓▓▓ ← Image COMPLÈTE
          ░░░░░░░
Frame N+1:□□□□□□□ ← Swap instantané! Nouvelle image COMPLÈTE
          ▓▓▓▓▓▓▓
```

---

## SLIDE 15: Compilation et Lancement - Explication des commandes

### Speech Développé:

"Maintenant, une question intéressante: pourquoi les commandes complexes et pas simplement `make` et `make run_gui`?

### Les deux approches:

**APPROCHE 1: Simple (ce qu'on attend classiquement)**

```bash
$ make              # Compile tout
$ make run_gui      # Lance QEMU
```

**Problème**: Sur un serveur Linux sans interface graphique, cela ne marche pas!

- Pas d'écran
- Pas de fenêtre QEMU
- Jeu pas visible

---

**APPROCHE 2: Complexe mais fonctionnelle (ce qu'on utilise)**

```bash
cd /workspaces/base-projet/build/boot && \
  qemu-system-i386 -kernel sextant.elf -vnc :0 >/dev/null 2>&1 &

websockify --web=/usr/share/novnc 5700 localhost:5900 >/dev/null 2>&1 &

# Puis ouvrir: http://localhost:5700/vnc.html
```

### Pourquoi cette approche?

#### Commande 1: Lancer QEMU sans interface graphique

```bash
cd /workspaces/base-projet/build/boot && \
  qemu-system-i386 \
    -kernel sextant.elf \    # ← Noyau à exécuter
    -vnc :0 \                # ← Affichage via VNC, pas GUI
    >/dev/null 2>&1 &        # ← Redirection output + background
```

**Détails:**

- `cd /workspaces/base-projet/build/boot`: aller là où sont les fichiers compilés
- `-kernel sextant.elf`: kernel OS à exécuter
- `-vnc :0`: **Serveur VNC** sur le port 5900 (au lieu d'interface graphique)

  - VNC = Virtual Network Computing
  - Le \":0\" signifie premier écran disponible
  - QEMU ne crée pas une fenêtre X11
  - L'écran est accessible via réseau (port 5900)

- `>/dev/null 2>&1`: Redirection des logs (on les ignore)
- `&`: Lancer en background (le processus continue, prompt revient)

**Avantage**: QEMU s'exécute même sans écran physique! Parfait pour serveur!

#### Commande 2: Bridge VNC vers WebSocket

```bash
websockify \
  --web=/usr/share/novnc \     # ← Interface web
  5700 \                         # ← Port web (local)
  localhost:5900 \               # ← Bridge vers QEMU VNC
  >/dev/null 2>&1 &
```

**Pourquoi?**

VNC utilise un protocole direct (port 5900) qui ne passe pas bien par HTTP.
Les navigateurs Web modernes n'aiment pas ça.

Solution: **WebSockify** = proxy qui convertit VNC → WebSocket (HTTP compatible)

```
┌────────────────────────────────────────┐
│         Navigateur Web                  │
│         (http://localhost:5700)         │
└────────┬─────────────────────┬──────────┘
         │                     │
         │ HTTP + WebSocket    │
         ▼                     │
┌────────────────────────────────────────┐
│      WebSockify (5700)                  │
│      (proxy HTTP ↔ VNC)                 │
└────────┬──────────────────────┬─────────┘
         │                      │
         │ VNC protocol         │
         ▼                      │
    QEMU VNC (5900)             │
    Serveur VNC du jeu ◄────────┘
```

#### Commande 3: Ouvrir dans le navigateur

```bash
http://localhost:5700/vnc.html
```

C'est la **page Web** fournie par noVNC (client VNC web).

Elle utilise WebSockets pour communiquer avec WebSockify, qui parle à QEMU.

### Comparaison avec make run_gui

**make run_gui (classique):**

```bash
make run_gui
```

**Internement, probablement:**

```bash
qemu-system-i386 -sdl -kernel sextant.elf
```

Problèmes:

- ✗ `-sdl` = interface graphique SDL (requires X11/framebuffer)
- ✗ Ne marche pas sur serveur sans écran
- ✗ Ne marche pas en SSH session
- ✗ Ne marche pas sur Windows (WSL sans GUI)

**Notre approche:**

```bash
cd ... && qemu-system-i386 -kernel ... -vnc :0 ... &
websockify --web=... 5700 localhost:5900 ... &
```

Avantages:

- ✓ Marche sur serveur sans écran
- ✓ Marche en SSH session
- ✓ Marche depuis n'importe quel OS (juste un navigateur!)
- ✓ Accès distant possible (changer localhost par IP machine)

### Pourquoi ces choix pour NOTRE projet?

Notre développement se fait probablement dans:

- Un **conteneur Docker** (pas d'écran physique)
- Une **VM Linux** sur Windows/Mac
- Un **serveur distant** en SSH
- Un **WSL** sur Windows

Donc on **NE PEUT PAS** utiliser d'interface graphique directe.

VNC + WebSockify = soluzione universelle!

### Alternatives (pourquoi pas utilisées):

**Option 1: Serial output**

```bash
qemu-system-i386 -serial stdio -kernel sextant.elf
```

Problème: Jeu en texte ASCII, pas graphique!

**Option 2: SSH X11 forwarding**

```bash
ssh -X user@server
cd /path && make run_gui
```

Problème: Lent, nécessite SSH avancé, firewalls complexes

**Option 3: Système graphique local (VMware, VirtualBox)**

```
Problème: Lourd, requires installation, pas portable
```

**Notre option: VNC Web**

- Léger ✓
- Portable ✓
- Pas de dépendances complexes ✓
- Fonctionne depuis navigateur ✓
- Support serveur natif ✓

### Procédure complète:

```bash
# 1. Compiler
cd /path/to/project
make clean
make

# 2. Lancer QEMU (background)
cd build/boot
qemu-system-i386 -kernel sextant.elf -vnc :0 >/dev/null 2>&1 &

# 3. Lancer WebSockify (background)
websockify --web=/usr/share/novnc 5700 localhost:5900 >/dev/null 2>&1 &

# 4. Ouvrir navigateur
# URL: http://localhost:5700/vnc.html

# 5. Jouez au jeu!
# (Via l'interface VNC dans le navigateur)

# 6. Pour arrêter:
killall qemu-system-i386
killall websockify
```

### Résultat final:

L'écran QEMU s'affiche dans votre navigateur!

- Vous voyez le jeu en 640×400 pixels
- Vous pouvez interagir (clavier works!)
- Pas de configuration graphique compliquée
- Fonctionne partout!

C'est l'approche utilisée dans les **cloud labs** et **platefes pédagogiques** modernes!
"

---

## CONCLUSION DE CES SLIDES

Ces 10 slides (6-15) couvrent:

1. **Slide 6**: Architecture générale (`threaded_breakout()`)
2. **Slide 7**: Communication entre threads (`g_game_state`)
3. **Slide 8**: Les acteurs du système (3 threads)
4. **Slide 9**: Timing et fairness (préemptif round-robin)
5. **Slide 10**: Concurrence sécurisée (Mutex)
6. **Slide 12**: Gestion mémoire (placement-new)
7. **Slide 14**: Rendu fluide (double buffering)
8. **Slide 15**: Mise en œuvre pratique (VNC/WebSockify)

Avec ces explications, vous couvrez:

- ✓ Architecture
- ✓ Multitâche
- ✓ Synchronisation
- ✓ Rendu
- ✓ Déploiement

Prêt pour la présentation!
