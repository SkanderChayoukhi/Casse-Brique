# FIXES APPLIED TO CASSE BRIQUE GAME

## Date: December 14, 2025
## Critical Bug Fixes

### Problem: QEMU Crashing/Blinking Display

**Root Causes Identified:**
1. **Stack-allocated synchronization objects going out of scope**
2. **Main thread blocking the game loop preventing scheduler operation**
3. **Mutex deadlock between game loop and entity threads**
4. **Missing separate rendering thread**

### Solutions Implemented:

#### 1. **Heap-Allocated Game Objects** (GameManager_impl.cpp)
**Problem**: Local variables (Mutex, Semaphore, Ball, Paddle, Brick) were stack-allocated and going out of scope when `game_start()` continued executing.

**Fix**: Changed all game objects to heap allocation using `new`:
```cpp
// Before (BROKEN):
Mutex positionMutex;
Ball ball(&positionMutex, &collisionSem);

// After (FIXED):
g_positionMutex = new Mutex();
g_ball = new Ball(g_positionMutex, collisionSem);
```

#### 2. **Separated Rendering from Main Thread**
**Problem**: Main thread was doing all rendering while holding mutexes, causing deadlock with Ball/Paddle threads.

**Fix**: Created dedicated `GameRenderer` thread class:
```cpp
class GameRenderer : public Threads {
    virtual void run() {
        while (g_running && g_lives > 0) {
            // Render game state
            // Collision detection
            // Update AI
            Yield();
        }
    }
};
```

#### 3. **Main Thread Becomes Idle Loop**
**Problem**: Main thread was blocking on game loop, preventing scheduler from running.

**Fix**: Main thread now just yields:
```cpp
while (g_running && g_lives > 0) {
    thread_yield();  // Let other threads run
}
```

#### 4. **Increased Delay Timings**
**Problem**: Ball and paddles updating too fast (30000 iterations), causing rapid mutex contention.

**Fix**: Increased delay to 100000 iterations to slow down updates and reduce lock contention.

#### 5. **Proper Thread Lifecycle**
**Before**:
- Create threads
- Enable interrupts immediately
- Start game loop (DEADLOCK)

**After**:
- Create all objects on heap
- Start all threads
- Enable interrupts
- Main thread yields (CLEAN)

## Key Concepts Demonstrated

### 1. Threading (Lab 4)
- **Ball Thread**: Continuous physics simulation
- **Paddle1 Thread**: Human player input handling
- **Paddle2 Thread**: AI opponent logic
- **Renderer Thread**: Game rendering and collision detection
- **NullProcess**: Idle thread to prevent scheduler crash

### 2. Synchronization (Lab 5)
- **Mutex** (`g_positionMutex`): Protects ball/paddle position data
- Lock pattern:
  ```cpp
  positionLock->lock();
  // Critical section
  positionLock->unlock();
  ```

### 3. Preemptive Scheduling (Lab 4)
- Timer interrupt (`IRQ_TIMER`) calls `sched_clk()`
- Threads use `Yield()` for cooperative scheduling
- `NullProcess` ensures scheduler always has a thread to run

### 4. VGA Graphics (Lab 1 + VGA support)
- EcranBochs class: 640x400 8-bit palette mode
- Double buffering with `swapBuffer()`
- Palette-based color rendering

## Testing Checklist

- [x] Game compiles without errors
- [ ] QEMU boots without crashing
- [ ] Ball moves and bounces off walls
- [ ] Paddle 1 responds to Q/D keys
- [ ] Paddle 2 AI tracks ball
- [ ] Ball collides with paddles
- [ ] Bricks break on collision
- [ ] Score increases on hits
- [ ] Lives decrease when ball is lost
- [ ] Game ends after 3 lives

## Lab Concepts Used

| Lab | Concept | Implementation |
|-----|---------|----------------|
| Lab 1 | Screen/VGA | EcranBochs for graphics |
| Lab 2 | Interrupts | IRQ_KEYBOARD for input, IRQ_TIMER for scheduling |
| Lab 3 | Memory | Heap allocation with new/delete |
| Lab 4 | Threading | Threads class, Yield(), create_kernel_thread |
| Lab 4 | Scheduling | Preemptive scheduler with sched_clk, NullProcess |
| Lab 5 | Mutex | Protecting shared position data |
| Lab 5 | Semaphore | (Available for future use) |

## Files Modified

1. `sextant/cassebrique/GameManager_impl.cpp` - Complete rewrite with heap allocation and rendering thread
2. `sextant/cassebrique/Ball.cpp` - Increased delay timing
3. `sextant/cassebrique/Paddle.cpp` - Increased delay timing
4. `sextant/main.cpp` - No changes needed (already correct)

## Build Command

```bash
cd /workspaces/Casse-Brique/base-projet
make clean
make
```

## Run Command

```bash
qemu-system-i386 -display curses -net nic,model=ne2k_isa -net user,tftp=./build/boot -cdrom ./build/boot/grub.iso
```

Or use the VS Code task: "Run Sextant"
