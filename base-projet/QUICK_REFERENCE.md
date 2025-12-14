# CASSE BRIQUE - QUICK REFERENCE GUIDE

## Important Code Sections with Comments

### Threading & Synchronization Comments

#### Ball::run() - Ball Physics Thread
**File**: [sextant/cassebrique/Ball.cpp](sextant/cassebrique/Ball.cpp) - Lines 25-65

```cpp
/**
 * THREAD FUNCTION - Ball movement loop
 * 
 * Context: Executes in dedicated thread, scheduled preemptively
 * Key Concepts:
 *  1. MUTEX PROTECTION: Locks before accessing shared position state
 *  2. YIELD: Calls Yield() to allow other threads to run
 *  3. PREEMPTION: Timer interrupt can switch threads at any time
 */
void Ball::run() {
    while (active) {
        // ... delay ...
        
        // CRITICAL SECTION: Protect all position/velocity updates
        positionLock->lock();
        x += dx;
        y += dy;
        // ... collision detection ...
        positionLock->unlock();
        
        // YIELD: Give other threads a chance to run
        Yield();
    }
}
```

**Key Takeaway**: 
- Mutex protects x, y, dx, dy from concurrent access
- Yield() allows scheduler to switch to other ready threads
- Without Yield(), main thread would be starved

---

#### Paddle::run() - Control Thread (Human & AI)
**File**: [sextant/cassebrique/Paddle.cpp](sextant/cassebrique/Paddle.cpp) - Lines 30-90

```cpp
/**
 * THREAD FUNCTION - Paddle movement loop
 * 
 * Context: Executes in dedicated thread, scheduled preemptively
 * 
 * Key Concepts:
 *  1. TWO CONTROL MODES: Human (keyboard) vs AI (auto-tracking)
 *  2. MUTEX PROTECTION: Locks before updating x position
 *  3. YIELD: Allows scheduler to switch to other threads
 *  4. PREEMPTION: Timer interrupt can interrupt execution at any time
 */
void Paddle::run() {
    while (true) {
        // ... delay ...
        
        if (type == PLAYER_HUMAN && keyboard) {
            // HUMAN CONTROL MODE
            positionLock->lock();
            if (keyboard->is_pressed(AZERTY::K_Q)) {
                x -= MAX_PADDLE_SPEED;
            }
            // ...
            positionLock->unlock();
        }
        else if (type == PLAYER_AI) {
            // AI CONTROL MODE - Ball-tracking algorithm
            positionLock->lock();
            // Simple pursuit algorithm with deadzone
            positionLock->unlock();
        }
        
        Yield();
    }
}
```

**Key Takeaway**:
- Two distinct control modes: human (keyboard) and AI (tracking)
- AI uses deadzone to prevent jitter and make it beatable
- Both acquire mutex before updating position

---

#### GameManager - Main Loop & Synchronization
**File**: [sextant/cassebrique/GameManager_impl.cpp](sextant/cassebrique/GameManager_impl.cpp) - Lines 150-220

```cpp
/**
 * GAME LOOP STRUCTURE:
 * 
 * While game running:
 *   1. Get ball position (thread-safe via mutex)
 *   2. Check collisions:
 *      a) Ball vs Paddle1
 *      b) Ball vs Paddle2
 *      c) Ball vs Bricks
 *   3. Update game state (score, lives)
 *   4. Render all entities to screen
 *   5. Handle game-over condition
 *   6. Yield to other threads (preemptive scheduler)
 * 
 * CRITICAL SECTIONS:
 *   - Getting ball/paddle positions (protected by mutex)
 *   - All rendering (screen buffer access)
 */

bool gameRunning = true;
while (gameRunning) {
    // Get current ball position (thread-safe read)
    int ballX, ballY;
    ball.getPosition(ballX, ballY);  // Uses mutex internally
    
    // === COLLISION DETECTION ===
    if (ball.checkPaddleCollision(...)) {
        ball.setVelocity(...);  // Bounce ball
        score1 += 10;           // Increment score
    }
    
    // === RENDERING ===
    screen.clear(0);
    // ... draw entities ...
    screen.swapBuffer();  // Double-buffering to avoid tearing
    
    /**
     * YIELD TO SCHEDULER
     * 
     * This call allows:
     * 1. Preemptive scheduler to switch to other ready threads
     * 2. Ball thread to update position
     * 3. Paddle threads to process input
     */
    thread_yield();
}
```

**Key Takeaway**:
- Main thread coordinates all rendering and collision detection
- Must yield to allow Ball and Paddle threads to run
- Double-buffering prevents screen tearing

---

### Synchronization Justification

#### Why Mutex is Essential
**File**: [CASSE_BRIQUE_DOCUMENTATION.md](CASSE_BRIQUE_DOCUMENTATION.md#synchronization-lab-5---synchronisation)

**Problem Without Mutex** (Race Condition):
```cpp
// Thread 1 (Ball thread) - Writer
x = 100;
y = 200;

// Thread 2 (Main thread) - Reader (during preemption)
int ballX = x;  // Might read x=150 (from next update!)
int ballY = y;  // Might read y=200 (not updated yet!)
```

Result: Inconsistent state → visual glitches or crashes

**Solution With Mutex**:
```cpp
// Thread 1 (Ball thread) - Writer
mutex.lock();
x = 150;
y = 250;
mutex.unlock();

// Thread 2 (Main thread) - Reader
mutex.lock();
int ballX = x;  // GUARANTEED: x=150, y=250 (consistent)
int ballY = y;
mutex.unlock();
```

---

### Key Concepts Summary Table

**Copy this table to understand what's being used:**

| Concept | Lab | Used For | File |
|---------|-----|----------|------|
| **Threads** | 4 | Ball, Paddle entities | Ball.h, Paddle.h |
| **Mutex** | 5 | Protect position data | Ball::run(), Paddle::run() |
| **Preemption** | 4 | Force thread switches | handler_tic.h (timer) |
| **Context Switch** | 4 | Save/restore CPU state | cpu_context_switch.S |
| **VGA Graphics** | Custom | Game rendering | GameManager_impl.cpp |
| **AABB Collision** | Custom | Collision detection | Ball.cpp, Brick.cpp |
| **AI Algorithm** | Custom | Computer opponent | Paddle.cpp |

---

## Code Walkthrough Flow

### Starting the Game

```
1. Sextant_main()                [sextant/main.cpp]
   ├─ Initialize hardware (VGA, keyboard, timer)
   ├─ Setup IDT/IRQ handlers
   ├─ Enable interrupts (sti)
   ├─ Call game_start()
   │
   └─> game_start()               [GameManager_impl.cpp]
       ├─ Initialize graphics
       ├─ Create synchronization objects (Mutex, Semaphore)
       ├─ Create Ball instance
       │
       ├─ ball.start()             [Threads.h]
       │   └─> create_kernel_thread() [thread.h]
       │       └─> Launch Ball::run() in separate thread
       │
       ├─ paddle1.start()
       │   └─> Launch Paddle::run() (HUMAN mode)
       │
       ├─ paddle2.start()
       │   └─> Launch Paddle::run() (AI mode)
       │
       └─ Main game loop
           └─ Continuous: render, detect collisions, yield
```

### During Runtime - Thread Scheduling

```
Timeline with Preemptive Scheduling:

[0ms]   Timer fires → sched_clk() → switch to Ball thread
        Ball thread: Update x += dx; y += dy

[1ms]   Timer fires → sched_clk() → switch to Paddle1 thread
        Paddle1 thread: Read keyboard, move paddle1

[2ms]   Timer fires → sched_clk() → switch to Paddle2 thread
        Paddle2 thread: Track ball, move paddle2

[3ms]   Timer fires → sched_clk() → switch to Main thread
        Main thread: Render, check collisions, yield

[4ms]   Timer fires → sched_clk() → switch to Ball thread
        (cycle repeats)

Note: Each thread uses mutex when accessing shared data!
```

---

## How to Find Key Comments in Code

### Threading Comments
1. **Ball::run()** - Search for "THREAD FUNCTION - Ball movement loop"
2. **Paddle::run()** - Search for "THREAD FUNCTION - Paddle movement loop"
3. **Threads.h** - Search for "startme" to see thread creation pattern

### Synchronization Comments
1. **Ball.cpp** - Search for "CRITICAL SECTION"
2. **Paddle.cpp** - Search for "MUTEX PROTECTION"
3. **GameManager_impl.cpp** - Search for "YIELD TO SCHEDULER"

### Graphics Comments
1. **GameManager_impl.cpp** - Search for "RENDERING" section
2. **GameManager_impl.cpp** - Search for "Double-buffering"

### Collision Detection Comments
1. **Ball.cpp** - Search for "COLLISION DETECTION: Simple wall bouncing"
2. **Brick.cpp** - Search for "AABB collision detection"

---

## Common Questions Answered

### Q1: Why do we need a Mutex if the scheduler can interrupt at any time?

**Answer**: Preemption means interrupts can happen ANY instruction. Without mutex:
```
Ball::run()                  Main thread (render)
x = 150;
                            int ballX = x;  ← INTERRUPTED MID-WRITE!
y = 200;                    // reads x=150, but old y or vice versa
```

Mutex ensures atomic operations on multiple variables.

### Q2: How does the AI work?

**Answer**: Simple ball-tracking with deadzone:
```cpp
// Calculate paddle center
int center = paddleX + width/2;

if (ballX < center - 20) {
    paddleX -= 3;  // Move left
}
else if (ballX > center + 20) {
    paddleX += 3;  // Move right
}
// else: within deadzone, don't move
```

The 20-pixel deadzone prevents jitter and makes AI beatable.

### Q3: Why do we Yield()?

**Answer**: If we don't yield, other threads starve:
```
Without Yield():
Main thread loops infinitely, never yields
→ Ball thread never gets CPU time
→ Ball doesn't move
→ Game is broken

With Yield():
Main thread yields
→ Scheduler runs other ready threads
→ Ball thread gets CPU time and moves
→ Game works!
```

### Q4: How is the screen not tearing?

**Answer**: Double-buffering:
```cpp
screen.clear(0);          // Clear off-screen buffer
// ... draw to buffer ...
screen.swapBuffer();      // Atomically swap to display
```

Display always shows complete frames, never partial renders.

### Q5: What if two threads access position simultaneously without mutex?

**Answer**: Race condition - data inconsistency:
```
Thread A (Ball)         Thread B (Main)
x = new_x;             int read_x = x;  ← May read partially-written value!
y = new_y;
```

With mutex, both operations are atomic - either all-or-nothing.

---

## Building and Running

### Quick Build & Test

```bash
# Navigate to project
cd /workspaces/Casse-Brique/base-projet

# Build
make clean
make

# Run in QEMU
make run

# Debug with GDB
make debug
```

### Expected Output
- QEMU window opens with 640x400 VGA display
- Ball visible in center, moving
- Paddles visible at top and bottom
- Bricks visible in upper area
- Game is playable with keyboard (Q/D for paddle 1)

---

## Checklist for Code Review

When reviewing the code, verify these points:

### Threading
- [ ] Ball, Paddle1, Paddle2 inherit from Threads
- [ ] Each has virtual void run() method
- [ ] start() called to create kernel thread
- [ ] Yield() called to allow other threads

### Synchronization
- [ ] Mutex created for shared data
- [ ] lock() called before accessing shared data
- [ ] unlock() called after critical section
- [ ] No deadlocks (lock acquired and released properly)

### Graphics
- [ ] VGA initialized in game_start()
- [ ] Palette set
- [ ] Clear screen before drawing
- [ ] Draw all entities (ball, paddles, bricks)
- [ ] swapBuffer() called for display update

### Game Logic
- [ ] Ball moves with correct velocity
- [ ] Wall collisions reverse velocity
- [ ] Paddle collisions bounce ball
- [ ] Brick collisions mark destroyed
- [ ] Score increments correctly
- [ ] Game over condition detected

### Documentation
- [ ] Comments explain threading strategy
- [ ] Comments explain synchronization needs
- [ ] Comments explain game loop structure
- [ ] No obvious code smells or bugs

---

## Summary

This Casse Brique implementation demonstrates:

1. **Threading**: 4 concurrent threads (Ball, Paddle1, Paddle2, Main)
2. **Synchronization**: Mutex protection for shared game state
3. **Preemptive Scheduling**: Timer-based context switching every 1ms
4. **VGA Graphics**: 640x400 8-bit color rendering with double-buffering
5. **Collision Detection**: AABB collision system
6. **Game AI**: Ball-tracking computer opponent
7. **Documentation**: Comprehensive inline comments

All code is carefully commented to explain:
- **Why** synchronization is needed (race condition prevention)
- **How** threading works (create thread, run, yield, exit)
- **What** happens during rendering (clear, draw, swap)
- **When** collisions are checked (main game loop)

**Ready for presentation and evaluation!**
