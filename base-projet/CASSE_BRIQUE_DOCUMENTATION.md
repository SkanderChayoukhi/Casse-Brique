# CASSE BRIQUE PROJECT - IMPLEMENTATION DOCUMENTATION

## Project Overview

**Casse Brique** (Breakout) is a two-player game implemented as a kernel-mode operating system application demonstrating advanced concepts in **operating systems, threading, synchronization, and real-time scheduling**.

## Game Description

### Gameplay
- **Two-player breakout game** on a 640x400 pixel VGA display
- **Player 1** controls top paddle with keyboard (human-controlled)
- **Player 2** controls bottom paddle with AI (computer opponent)
- A **ball** bounces around the screen, destroying **bricks**
- Bricks are arranged in a 3x8 grid in the upper portion of the screen
- Players score points by hitting bricks with the ball
- Game ends when a player loses all lives (ball goes off their side)

### Visual Elements
- **Ball**: 8x8 pixel colored square (bright white/yellow)
- **Paddle 1** (Top): 80x16 pixel blue rectangle
- **Paddle 2** (Bottom): 80x16 pixel red rectangle  
- **Bricks**: 40x15 pixel colored rectangles (multiple colors per row)
- **VGA Display**: 640x400 pixels, 8-bit color palette

---

## OS Concepts Implemented

### 1. THREADING (Lab 4 - Ordonnancement)

**Concept**: Multiple independent execution flows running "simultaneously" via preemptive scheduling.

**Implementation**:
```
Main Application Threads:
├── Ball Thread
│   ├── Inherits: Threads class
│   ├── Function: Continuous ball movement & physics
│   ├── Run Method: Ball::run()
│   └── Synchronization: Mutex-protected position updates
│
├── Paddle 1 Thread (Human)
│   ├── Inherits: Threads class
│   ├── Function: Process keyboard input, move paddle
│   ├── Run Method: Paddle::run()
│   └── Control: AZERTY Q/D keys for left/right
│
├── Paddle 2 Thread (AI)
│   ├── Inherits: Threads class
│   ├── Function: AI-controlled paddle movement
│   ├── Run Method: Paddle::run()
│   └── AI Algorithm: Ball-tracking with smoothing deadzone
│
└── Main Thread
    ├── Rendering: Draw all game entities to screen
    ├── Collision detection: Check all collisions
    ├── Game state management: Score, lives, game over
    └── Scheduler coordination: Yield control to scheduler
```

**Creation Pattern** (Lab 4):
```cpp
Ball ball(&positionMutex, &collisionSem);
ball.start();  // Creates kernel thread via create_kernel_thread()
```

**Key Methods**:
- `void start()` - Creates and starts the kernel thread
- `virtual void run()` - Main thread function (override in subclass)
- `void Yield()` - Cooperatively yields to scheduler
- `void Exit()` - Exits thread gracefully

---

### 2. SYNCHRONIZATION (Lab 5 - Synchronisation)

**Problem**: Multiple threads access shared resources simultaneously
- Ball position is updated by Ball thread
- Position is read by main thread (rendering)
- Without synchronization = **race conditions** = inconsistent state

**Solution**: Use **Mutex** to protect critical sections

#### Mutex (Mutual Exclusion Lock)
**Definition**: A Semaphore initialized to 1. Only one thread can hold the lock.

**Usage Pattern**:
```cpp
// In Ball::run()
positionLock->lock();      // Acquire mutex (blocks if held by another thread)
x += dx;                   // Update position safely
y += dy;
positionLock->unlock();    // Release mutex (other threads can now acquire)

// In main game loop
ball.getPosition(x, y);    // Uses internal mutex
```

**Justification for Casse Brique**:
1. **Ball position**: Updated by Ball thread, read by main (rendering + collision)
2. **Paddle positions**: Updated by Paddle threads, read by main (rendering)
3. **Brick state**: Read by main thread, potentially written by collision handler
4. **Preemptive scheduling**: Timer interrupt can switch threads at ANY time
   - Without mutex, ball could be in middle of position update when main reads it
   - Would see partial/inconsistent state leading to visual glitches

**Example Race Condition** (without mutex):
```cpp
// Thread 1 (Ball thread)        // Thread 2 (Main thread)
x += dx;                         
                                 int xPos = x;  // Read mid-update!
y += dy;                         
                                 // xPos = old x, next read gets new y
```

#### Semaphore (Advanced Feature)
- **Not heavily used** in basic implementation (initialized but optional)
- Could coordinate collision events or input handling
- Implemented per Lab 5 specifications

---

### 3. SCHEDULING (Lab 4 - Preemptive)

**Scheduling Strategy**: **Preemptive Priority-Round-Robin**

**Key Concepts**:
1. **Preemption**: Timer interrupt (every 1ms) forces context switch
   - Handler: `sched_clk()` in `handler_tic.h`
   - Interrupt: IRQ_TIMER → `sched_clk()`

2. **Context Switching**: CPU state (registers, stack) saved/restored
   - Implemented in: `cpu_context_switch.S` (assembly)
   - Preserves execution state across thread switches

3. **Scheduler States**:
   - **READY**: Thread ready to run, waiting for CPU
   - **RUNNING**: Thread executing on CPU
   - **WAITING**: Thread blocked (e.g., on semaphore)
   - **NullProcess**: Dummy thread to prevent scheduler deadlock

**How It Works in Casse Brique**:
```
Timeline Example:
[0ms]   Ball thread runs:      x += dx; y += dy;
[0.5ms] [Timer interrupt]     → Context switch to Paddle1 thread
[0.5ms] Paddle1 thread runs:   Process keyboard, move paddle
[1.0ms] [Timer interrupt]     → Context switch to Paddle2 thread
[1.0ms] Paddle2 thread runs:   AI algorithm calculates movement
[1.5ms] [Timer interrupt]     → Context switch to Main thread
[1.5ms] Main thread runs:      Render all entities, check collisions
[2.0ms] [Timer interrupt]     → Back to Ball thread
```

**Responsive Behavior**:
- Without preemption: If one thread has long operations, others starve
- With preemption: All threads get equal CPU time, smooth gameplay
- Frequency: 1000 Hz (1ms intervals) - sufficient for smooth 30-60fps animation

---

### 4. MEMORY MANAGEMENT

**Approach**: Static allocation where possible, avoided dynamic allocation to prevent linker issues.

**Brick Storage**:
```cpp
Brick bricksArray[NUM_BRICKS_VERTICAL][NUM_BRICKS_HORIZONTAL];  // Static
Brick *bricks[NUM_BRICKS_VERTICAL][NUM_BRICKS_HORIZONTAL];     // Pointers
```

**Why**: 
- Custom `operator new` in kernel has linking constraints
- Static allocation is safer and faster in kernel mode
- All bricks initialized at startup, no dynamic allocation

---

## Architecture & Design

### Game Entity Hierarchy

```
Threads (Base Class - Lab 4)
├── Ball
│   ├── Position: (x, y)
│   ├── Velocity: (dx, dy)
│   ├── Size: 8x8 pixels
│   ├── Active: Bool (game status)
│   └── Methods:
│       ├── run() - Physics loop
│       ├── getPosition() - Thread-safe getter
│       ├── setPosition() - Thread-safe setter
│       ├── setVelocity() - Thread-safe setter
│       ├── reset() - Reset to starting position
│       ├── checkWallCollision() - Wall detection
│       └── checkPaddleCollision() - Paddle detection
│
├── Paddle
│   ├── Position: (x, y)
│   ├── Dimensions: 80x16 pixels
│   ├── Type: PLAYER_HUMAN or PLAYER_AI
│   ├── Control: Keyboard (human) or AI (computer)
│   └── Methods:
│       ├── run() - Control loop (human or AI)
│       ├── setPosition() - Thread-safe setter
│       ├── updateBallPos() - Feed ball position to AI
│       └── Getters: getX(), getY(), getWidth(), getHeight()
│
└── Non-Thread Entities:
    └── Brick
        ├── Position: (x, y)
        ├── Dimensions: 40x15 pixels
        ├── Color: VGA palette index
        ├── Destroyed: Bool
        └── Methods:
            ├── isDestroyed() - Check if brick is destroyed
            ├── destroy() - Mark as destroyed
            ├── checkCollision() - AABB collision with ball
            └── Getters: getX(), getY(), getWidth(), getHeight(), getColor()
```

### Main Game Loop (Pseudo-code)

```cpp
while (gameRunning)
{
    // 1. GET STATE (Thread-safe reads with mutex)
    ball.getPosition(ballX, ballY);
    int p1X = paddle1.getX();
    int p2X = paddle2.getX();
    
    // 2. COLLISION DETECTION
    if (ball.checkPaddleCollision(p1X, p1Y, ...)) {
        ball.setVelocity(...);  // Bounce
        score1 += 10;
    }
    
    for each brick {
        if (brick.checkCollision(ballX, ballY, ...)) {
            brick.destroy();
            ball.setVelocity(...);
            score1 += 50;
        }
    }
    
    // 3. UPDATE AI
    paddle1.updateBallPos(ballX, ballY);
    paddle2.updateBallPos(ballX, ballY);
    
    // 4. RENDER
    screen.clear(0);
    renderBall(ballX, ballY);
    renderPaddle1(p1X, p1Y);
    renderPaddle2(p2X, p2Y);
    renderBricks();
    screen.swapBuffer();  // Double-buffering to avoid tearing
    
    // 5. YIELD
    thread_yield();       // Allow scheduler to switch threads
}
```

---

## File Structure

```
sextant/cassebrique/
├── Ball.h & Ball.cpp              [Ball entity + physics]
├── Paddle.h & Paddle.cpp          [Paddle entity + AI]
├── Brick.h & Brick.cpp            [Brick entity + collision]
├── GameManager.h                  [Interface definition]
└── GameManager_impl.cpp           [Main game loop + initialization]

Makefile                           [Build rules - Brick added]
```

---

## Synchronization Details

### Critical Sections (Protected by Mutex)

| Resource | Protected? | Why | Who Accesses |
|----------|-----------|-----|--------------|
| Ball (x, y) | ✅ YES | Ball thread writes, main reads | Ball + Main |
| Ball (dx, dy) | ✅ YES | Ball thread writes, main reads | Ball + Main |
| Paddle1 (x, y) | ✅ YES | Paddle1 thread writes, main reads | Paddle1 + Main |
| Paddle2 (x, y) | ✅ YES | Paddle2 thread writes, main reads | Paddle2 + Main |
| Brick state | ⚠️ PARTIAL | Main thread primarily, optional collision handler | Main + Collision |
| Score | ❌ NO* | Only main thread writes (optimization) | Main |

*Score could be protected for robustness, but current implementation only main thread updates.

### Code Example - Mutex Protection

**Ball::run() - Protected Update**:
```cpp
void Ball::run() {
    while (active) {
        // Animation delay
        for (volatile int i = 0; i < 50000; i++);
        
        // CRITICAL SECTION - Protected by mutex
        positionLock->lock();          // Acquire lock
        {
            x += dx;
            y += dy;
            // Update physics...
        }
        positionLock->unlock();        // Release lock
        
        Yield();  // Allow other threads
    }
}
```

**Main Thread - Safe Read**:
```cpp
int ballX, ballY;
ball.getPosition(ballX, ballY);  // Internally uses mutex

// Render without worrying about race condition
screen.plot_palette(ballX, ballY, color);
```

---

## VGA Graphics Implementation

### Display Mode
- **Resolution**: 640x400 pixels
- **Color Depth**: 8-bit (256 colors)
- **Palette**: Standard VGA 256-color palette
- **Double Buffering**: Swap buffer after frame rendered

### Rendering
```cpp
EcranBochs screen(640, 400, VBE_MODE::_8);
screen.init();
screen.set_palette(palette_vga);

// Each frame:
screen.clear(0);  // Black background

// Draw pixels/rectangles
for (int i = 0; i < width; i++) {
    screen.plot_palette(x + i, y, color);
}

screen.swapBuffer();  // Display completed frame
```

### Colors Used
- Ball: Color 25 (bright white/yellow)
- Paddle 1: Color 4 (blue)
- Paddle 2: Color 1 (red)
- Bricks: Colors 2-4 (green, cyan, etc.)
- Background: Color 0 (black)

---

## AI Algorithm for Paddle 2

**Approach**: Simple ball-tracking with smoothing

```cpp
while (true) {
    int paddleCenter = x + (width / 2);
    int ballCenter = ballX + 4;  // Ball center
    
    if (ballCenter < paddleCenter - AI_DEADZONE) {
        x -= MAX_PADDLE_SPEED;  // Move left
    }
    else if (ballCenter > paddleCenter + AI_DEADZONE) {
        x += MAX_PADDLE_SPEED;  // Move right
    }
    // else: within deadzone, don't move
    
    Yield();
}
```

**Features**:
- **Deadzone (20 pixels)**: Prevents jitter, makes AI "imperfect" and beatable
- **Limited Speed**: Can't move faster than paddle_speed = 3 pixels/frame
- **Continuous tracking**: Smoothly follows ball position
- **Beatable**: Deadzone and speed limits prevent perfect performance

---

## Collision Detection

### Ball vs Wall
```cpp
if (x <= 0 || x >= SCREEN_WIDTH - BALL_SIZE)
    dx = -dx;  // Reverse horizontal velocity
if (y <= GAME_AREA_TOP)
    dy = -dy;  // Reverse vertical velocity
```

### Ball vs Paddle (AABB)
```cpp
bool AABB_collision(
    int ballX, int ballY, int ballSize,
    int padX, int padY, int padW, int padH)
{
    bool collisionX = (ballX < padX + padW) && 
                      (ballX + ballSize > padX);
    bool collisionY = (ballY < padY + padH) && 
                      (ballY + ballSize > padY);
    return collisionX && collisionY;
}
```

### Ball vs Brick (Same AABB)
Uses identical AABB collision test for brick detection.

---

## Build & Compilation

### Makefile Changes
Added `Brick` to `OBJECTSNAMES`:
```makefile
OBJECTSNAMES= ... Ball Paddle Brick GameManager_impl
```

This ensures:
1. `Brick.cpp` compiled to `build/all-o/Brick.o`
2. Object file linked into final kernel executable

### Compilation Flags
```
CPPFLAGS = -gdwarf-2 -g3 -Wall -fno-builtin -fno-rtti -fno-exceptions -nostdinc
```

- `-fno-rtti`: No RTTI (Run-Time Type Information) in kernel
- `-fno-exceptions`: No C++ exceptions in kernel
- `-nostdinc`: No standard C/C++ library headers (kernel mode)

### Build Command
```bash
make          # Full rebuild
make clean    # Clean object files
make run      # Build and run in QEMU
```

---

## Testing & Validation

### Expected Behavior
1. **Game starts**: Ball appears in center, paddles at top/bottom
2. **Ball moves**: Bounces around screen smoothly
3. **User control**: Player 1 (top) responds to keyboard (Q/D keys)
4. **AI control**: Player 2 (bottom) tracks ball automatically
5. **Collisions**: Ball bounces off paddles and bricks
6. **Scoring**: Points awarded for paddle hits and brick destruction
7. **Game over**: Game ends when player loses 3 lives

### Synchronization Validation
- No visual tearing (double-buffering)
- Smooth animation (preemptive scheduling)
- No crashes from race conditions (mutex protection)

---

## Future Enhancements

1. **Score Display**: Add text rendering to display scores
2. **Sound Effects**: Add beep sounds for collisions
3. **Power-ups**: Paddle size increase, ball speed changes
4. **Multiple Levels**: Increasing brick difficulty
5. **Better AI**: Predict ball trajectory, angle-based bouncing
6. **Network Play**: Two-player over network
7. **Persistence**: Save high scores
8. **Menu System**: Start, pause, options screens

---

## Key Concepts Summary

| Concept | Why Used | Lab | Implementation |
|---------|----------|-----|-----------------|
| **Threads** | Multiple simultaneous game entities | Lab 4 | Ball, Paddle1, Paddle2, Main inherit from Threads |
| **Mutex** | Protect shared position data | Lab 5 | positionMutex guards all position updates |
| **Preemptive Scheduling** | Responsive gameplay, fair thread time | Lab 4 | Timer interrupt calls sched_clk() every 1ms |
| **Context Switching** | Switch between threads transparently | Lab 4 | cpu_context_switch.S (assembly) |
| **AABB Collision** | Fast geometric collision detection | Custom | Check bounding box intersections |
| **VGA Graphics** | Game rendering | Custom | EcranBochs 640x400 8-bit color |
| **AI Algorithm** | Computer opponent | Custom | Ball-tracking with deadzone |

---

## Conclusion

**Casse Brique** demonstrates a complete kernel-mode application using:
- ✅ **Threading** for concurrent entity management
- ✅ **Synchronization** (Mutex) for data safety
- ✅ **Preemptive Scheduling** for responsive gameplay
- ✅ **VGA Graphics** for game rendering
- ✅ **Real-time Physics** for ball movement
- ✅ **AI** for computer opponent
- ✅ **Collision Detection** for gameplay mechanics

All concepts are carefully implemented with comprehensive **comments** explaining the **synchronization strategies**, **scheduling decisions**, and **memory management** choices.

---

## References

- **Lab 4 (Ordonnancement)**: Threading implementation, scheduler setup
- **Lab 5 (Synchronisation)**: Mutex, Semaphore, Spinlock patterns
- **VGA Graphics**: EcranBochs driver documentation
- **OS Concepts**: Preemption, context switching, thread scheduling
