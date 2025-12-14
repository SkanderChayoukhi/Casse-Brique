# How to Run Casse Brique Game

## Build the Game
```bash
cd /workspaces/Casse-Brique/base-projet
make clean
make
```

## Run in QEMU

### Option 1: Run in Terminal (Recommended for dev container)
```bash
cd /workspaces/Casse-Brique/base-projet
qemu-system-i386 -display curses -net nic,model=ne2k_isa -net user,tftp=./build/boot -cdrom ./build/boot/grub.iso
```

### Option 2: Run with VNC (Access via browser)
```bash
cd /workspaces/Casse-Brique/base-projet
qemu-system-i386 -vnc :0 -net nic,model=ne2k_isa -net user,tftp=./build/boot -cdrom ./build/boot/grub.iso
```
Then open http://localhost:5900 in your browser (use a VNC client)

### Option 3: Use VS Code Task
Press `Ctrl+Shift+P` and select "Run Task" > "Run Sextant"

## Game Controls

- **Q**: Move left paddle LEFT
- **D**: Move left paddle RIGHT
- AI controls the right paddle automatically

## Game Architecture

The game demonstrates:
1. **Threading**: Ball, Paddle1 (human), Paddle2 (AI), Renderer - all run as separate threads
2. **Synchronization**: Mutex protects shared position data between threads
3. **Preemptive Scheduling**: Timer interrupt (IRQ_TIMER) calls sched_clk() for thread switching
4. **VGA Graphics**: 640x400 8-bit palette mode with double buffering

## Troubleshooting

### QEMU crashes/blinks
- Make sure you built with `make clean && make`
- Check that interrupts are enabled AFTER threads are created
- Verify NullProcess thread exists to prevent scheduler deadlock

### No display
- PCI bus detection might have failed
- Check serial output: `cat build/boot/sortieserieqemu.txt`

### Keyboard not working
- Make sure AZERTY layout is configured in grub
- Try Q and D keys (not A and D)
