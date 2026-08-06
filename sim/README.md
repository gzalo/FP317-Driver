# FP317 Local Simulator

This is a small SDL-based host-side harness for running the existing Arduino
sketch locally. It shadows the Arduino and IRremote APIs, includes the current
firmware code, and decodes the driver's pin writes back into a framebuffer.

The sketch runs on its own worker thread while the main thread owns SDL events
and rendering. That lets the firmware set or clear several dots between
presented frames, so slow redraws do not throttle the simulated display writes.
Millisecond delays keep the same duration as the sketch, while microsecond
hardware pulse delays are skipped because desktop OS sleep granularity can make
each dot write much slower than the real hardware pulse.

## Build

From the repository root:

```powershell
g++ -std=c++17 -pthread -Isim -I. sim/main.cpp -o sim/fp317_sim.exe -lSDL2
```

## Run

```powershell
.\sim\fp317_sim.exe
```

Controls:

- Arrow keys or `WASD`: move; in Tetris, Up rotates and Down soft-drops one row
- `Enter`: hard-drops a Tetris piece, launches the ball in Breakout, and fires in Space Invaders; `R` restarts after game over
- `Q` or `Esc`: quit

The simulator is meant for developing the display/game behavior. It does not
model FP2800A electrical timing or real flip-dot mechanics.
