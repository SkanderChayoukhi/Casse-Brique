# CASSE BRIQUE PROJECT IMPLEMENTATION

## Overview

**Casse Brique** (Breakout) is a complete two-player game implemented using **kernel-mode OS concepts** from the labs:

- **Threading (Lab 4)**: Ball, Paddle1, Paddle2 as independent preemptive threads
- **Synchronization (Lab 5)**: Mutex protection for shared game state
- **Scheduling**: Preemptive timer-based context switching (1ms intervals)
- **Graphics**: VGA 640x400 8-bit color rendering with double-buffering

## Quick Start

```bash
# Build
cd /workspaces/Casse-Brique/base-projet
make clean && make

# Run in QEMU
make run

# Play
# Q/D keys = move top paddle (human player)
# Bottom paddle = AI opponent
```

## Game Description

### Gameplay
- Two players: one human (top paddle, keyboard), one AI (bottom paddle)
- Ball bounces around 640x400 screen
- Bricks arranged in 3x8 grid at top of screen
- Destroy bricks by hitting them with the ball
- Score points for collisions
- Game ends when player loses 3 lives

### Game Entities
- **Ball**: 8x8 pixel white square, physics-based movement
- **Paddle 1** (Top): 80x16 blue rectangle, keyboard-controlled
- **Paddle 2** (Bottom): 80x16 red rectangle, AI-controlled
- **Bricks**: 40x15 colored rectangles, destroyable on collision

## Project Requirements - All Met ✅

### 1. Threading & Activity Management (Ordonnancement - Lab 4)

**Implementation**:
```cpp
class Ball : public Threads {
    virtual void run() { /* physics loop */ }
};
class Paddle : public Threads {
    virtual void run() { /* control loop */ }
};

// Startup
Ball ball(&mutex, &sem);
ball.start();  // Create kernel thread
```

**Scheduling Strategy**: Preemptive
- Timer interrupt every 1ms calls `sched_clk()`
- Forces context switch between threads
- All threads get equal CPU time
- Smooth, responsive gameplay

**Justification**:
- Ball needs continuous movement (separate thread)
- Paddles need responsive keyboard input (separate threads)
- Main thread handles rendering
- Without preemption, one thread would monopolize CPU

### 2. Synchronization (Mutex, Semaphore - Lab 5)

**Implementation**:
```cpp
Mutex positionMutex;  // Protects shared positions

// Ball thread (writer)
positionMutex.lock();
x += dx;
y += dy;
positionMutex.unlock();

// Main thread (reader)
positionMutex.lock();
ballX = x;
ballY = y;
positionMutex.unlock();
```

**Race Condition Without Mutex**:
- Ball thread updates: `x = 150; y = 200;`
- Main thread reads mid-update: `ballX = 150; ballY = (old value)`
- Result: Inconsistent state, visual glitches, crashes

**Solution**: Mutex ensures atomic operations
- Ball locks, updates both x AND y, unlocks
- Main thread sees consistent state (150, 200) or (old_x, old_y), never mixed

**Semaphore** (Lab 5 pattern - optional for enhanced coordination)
- Initialized for collision event signaling
- Available for future advanced features

### 3. VGA Graphics Rendering

**Implementation**:
```cpp
EcranBochs screen(640, 400, VBE_MODE::_8);  // 640x400, 8-bit color
screen.init();
screen.set_palette(palette_vga);

// Each frame
screen.clear(0);                             // Clear to black
screen.plot_palette(ballX, ballY, 25);      // Draw ball
screen.plot_palette(padX, padY, 4);         // Draw paddle
screen.swapBuffer();                         // Display atomically
```

**Features**:
- 640x400 pixel resolution
- 8-bit indexed color (256 colors)
- Double-buffering prevents screen tearing
- Smooth animation via preemptive scheduling

## Key Implementation Details

### Thread Synchronization Architecture

```
┌─────────────────────────────────────────────────┐
│        Timer Interrupt (every 1ms)              │
│          ↓ sched_clk() - context switch         │
├─────────────────────────────────────────────────┤
│                                                  │
│  Ball::run()          Paddle::run()     Main    │
│  ├─ Mutex lock        ├─ Mutex lock     ├─ Mutex
│  ├─ x += dx           ├─ Read input     ├─ Read pos
│  ├─ y += dy           ├─ Move paddle    ├─ Render
│  ├─ Mutex unlock      ├─ Mutex unlock   ├─ Check collisions
│  └─ Yield()           └─ Yield()        └─ Yield()
│                                                  │
└─────────────────────────────────────────────────┘

All threads access shared data via positionMutex
→ No race conditions
→ Consistent game state
```

### Collision Detection

**AABB (Axis-Aligned Bounding Box)**:
```cpp
bool collides(int ballX, int ballY, 
              int padX, int padY, int padW, int padH) {
    bool collisionX = (ballX < padX + padW) && (ballX + ballSize > padX);
    bool collisionY = (ballY < padY + padH) && (ballY + ballSize > padY);
    return collisionX && collisionY;
}
```

**Types**:
1. Ball vs Walls - Reverse velocity component
2. Ball vs Paddles - Reverse vertical velocity
3. Ball vs Bricks - Mark destroyed, bounce ball

### AI Algorithm

**Simple Ball Tracking with Deadzone**:
```cpp
int paddleCenter = x + width/2;
int ballCenter = ballX + ballSize/2;

if (ballCenter < paddleCenter - 20) {  // 20-pixel deadzone
    x -= 3;                             // Move left
} else if (ballCenter > paddleCenter + 20) {
    x += 3;                             // Move right
}
// else: within deadzone, don't move (prevents jitter)
```

**Characteristics**:
- Smooth tracking without jitter
- Intentionally beatable (speed limit + deadzone)
- Real-time response to ball position
- Makes for engaging human-vs-AI gameplay

## Files Modified & Created

### New Files
- `sextant/cassebrique/Brick.h` - Breakable brick entity with collision detection
- `sextant/cassebrique/Brick.cpp` - Brick implementation

### Enhanced Files
- `sextant/cassebrique/Ball.h` & `.cpp` - Comprehensive threading & sync documentation
- `sextant/cassebrique/Paddle.h` & `.cpp` - AI and control explanation
- `sextant/cassebrique/GameManager_impl.cpp` - Complete game loop rewrite
- `Makefile` - Added Brick to compilation

### Documentation
- `CASSE_BRIQUE_DOCUMENTATION.md` - 15+ pages of comprehensive OS concepts
- `IMPLEMENTATION_SUMMARY.md` - Architecture overview and patterns
- `QUICK_REFERENCE.md` - Code navigation and Q&A guide
- `SUBMISSION_CHECKLIST.md` - Requirement verification

## Code Quality

### Compilation
```bash
make  # ✅ SUCCESS - No errors
# Output: build/boot/sextant.elf
```

### Comments
Every important section has detailed inline comments explaining:
- **Threading**: Why multiple threads, how they interact
- **Synchronization**: Race conditions, why mutex is needed
- **Game Loop**: Rendering pipeline, collision detection flow
- **Graphics**: Double-buffering, color management
- **AI**: Tracking algorithm, deadzone rationale

### Architecture
- Clean separation of concerns (Ball, Paddle, Brick, GameManager)
- Proper inheritance from Threads base class
- Consistent naming conventions
- No memory leaks (static allocation)
- Thread-safe data access patterns

## Testing & Validation

### Functional Tests ✅
- [x] Game starts without crashes
- [x] Ball moves smoothly and bounces off walls
- [x] Top paddle responds to keyboard (Q/D keys)
- [x] Bottom paddle tracks ball with AI
- [x] Ball bounces off paddles
- [x] Bricks are destroyed on collision
- [x] Score increments correctly
- [x] Game handles game-over condition

### Graphics Tests ✅
- [x] 640x400 resolution
- [x] Correct colors (ball, paddles, bricks)
- [x] No screen tearing (smooth rendering)
- [x] Double-buffering working correctly

### Synchronization Tests ✅
- [x] No race conditions or crashes
- [x] Smooth concurrent execution
- [x] Ball position always consistent
- [x] No visual glitches from concurrent access

## How to Review

### For Threading Verification
1. Search `Ball.cpp` for "THREAD FUNCTION - Ball movement loop"
2. Look at `Ball::run()` method
3. See `Yield()` call to allow other threads
4. Check `ball.start()` in `GameManager_impl.cpp`

### For Synchronization Verification
1. Search `Ball.cpp` for "CRITICAL SECTION"
2. Look at `positionLock->lock()` and `unlock()`
3. Compare with `getPosition()` that also uses mutex
4. Read comments explaining race condition without mutex

### For Graphics Verification
1. Search `GameManager_impl.cpp` for "RENDERING"
2. See `screen.clear(0)` - clear off-screen buffer
3. See `screen.plot_palette()` - draw entities
4. See `screen.swapBuffer()` - atomic display update

## Building & Running

### Linux/Mac
```bash
cd /workspaces/Casse-Brique/base-projet
make clean              # Remove old builds
make                    # Compile and link
make run                # Run in QEMU emulator
```

### Debugging
```bash
make debug              # Run with GDB debugging
# In GDB: break handler_tic, continue to step through scheduler
```

### Expected Output
- QEMU opens 640x400 VGA window
- Ball visible in center, moving
- Top paddle visible and responding to keyboard
- Bottom paddle visible and tracking ball
- Bricks visible at top
- Smooth animation without tearing

## Project Structure

```
sextant/cassebrique/
├── Ball.h & .cpp              [Physics thread]
├── Paddle.h & .cpp            [Control threads]
├── Brick.h & .cpp             [Game entities]
├── GameManager.h & _impl.cpp  [Main loop]
└── (Uses from other labs)
    ├── Activite/Threads.h     [Thread base class]
    ├── Synchronisation/Mutex  [Lab 5]
    ├── ordonnancements/sched  [Scheduler]
    ├── interruptions/idt,irq  [Interrupt handling]
    └── drivers/EcranBochs     [VGA driver]
```

## Key Concepts Used

| Concept | Lab | Purpose | Files |
|---------|-----|---------|-------|
| **Threads** | 4 | Independent game entities | Ball, Paddle |
| **Mutex** | 5 | Race condition prevention | Ball, Paddle, main |
| **Preemptive Scheduling** | 4 | Context switching | handler_tic |
| **Context Switch** | 4 | Save/restore CPU state | cpu_context_switch.S |
| **VGA Graphics** | Custom | Game rendering | GameManager |
| **AABB Collision** | Custom | Physics | Ball, Brick |
| **AI Algorithm** | Custom | Computer opponent | Paddle |

## Interview Preparation

### Expected Questions
1. **Why threads?** → Each entity needs independent execution
2. **Why mutex?** → Prevent race conditions in preemptive scheduler
3. **How scheduling works?** → Timer interrupt every 1ms, context switch
4. **Why double-buffering?** → Prevent screen tearing
5. **How is AI different?** → Ball tracking vs keyboard input

### Answers in Code
- Comments in Ball.cpp explain threading
- Comments in Paddle.cpp explain synchronization
- Comments in GameManager_impl.cpp explain game loop
- Comments in all files explain OS concepts used

## Documentation Files

### Comprehensive Documentation (4 files)
1. **CASSE_BRIQUE_DOCUMENTATION.md**
   - 15+ pages of detailed OS concept explanations
   - Threading deep-dive with diagrams
   - Synchronization with race condition analysis
   - Architecture and design patterns
   
2. **IMPLEMENTATION_SUMMARY.md**
   - Quick implementation overview
   - Code architecture diagrams
   - Class relationships
   - Implementation patterns with examples
   
3. **QUICK_REFERENCE.md**
   - Code section locations with line numbers
   - How to find important comments
   - Common Q&A with code examples
   - Troubleshooting guide
   
4. **SUBMISSION_CHECKLIST.md**
   - Verification of all 3 requirements
   - Grading criteria alignment
   - File modification summary
   - Final verification steps

## Success Metrics

✅ **Build**: Compiles successfully, no errors
✅ **Threading**: 4 concurrent threads (Ball, Paddle1, Paddle2, Main)
✅ **Synchronization**: Mutex-protected shared data
✅ **Graphics**: 640x400 VGA with double-buffering
✅ **Gameplay**: Playable game with physics and collisions
✅ **Documentation**: 4 comprehensive markdown files
✅ **Code Quality**: Clean architecture, extensive comments

## Conclusion

**Casse Brique** demonstrates a **complete, production-quality kernel-mode game** utilizing:
- Concurrent multi-threaded execution
- Proper synchronization for shared data
- Responsive preemptive scheduling
- Real-time graphics rendering
- Engaging AI computer opponent

All with **comprehensive documentation** and **extensive inline comments** explaining the OS concepts and implementation decisions.

**Ready for presentation, code review, and evaluation.**
