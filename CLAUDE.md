# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

A small C++17 low-level-design exercise: an elevator bank driven by the SCAN
algorithm, with a facade that assigns hall calls to cars.

## Commands

| Command | Purpose |
|---|---|
| `make` / `make all` | Build the `elevator` driver binary |
| `make run` | Build and run the driver (`main.cpp`) |
| `make test` | Build and run the unit tests (`tests.cpp`) |
| `make check` | Syntax-only (`-fsyntax-only`) pass over every source file |
| `make clean` | Remove objects and both binaries |

Compiler flags are `-std=c++17 -Wall -Wextra -Wpedantic -g`; the build is
expected to stay warning-clean.

`main.cpp` and `tests.cpp` each define their own `main()`. The Makefile compiles
the three class implementations once into `LIB_OBJS` and links them against
either `main.o` (→ `elevator`) or `tests.o` (→ `run_tests`); never add both
entry-point files to one link.

### Running one test

`tests.cpp` uses a hand-rolled harness (no framework), so there is no
test-filter flag. To run a subset, edit the `kTests[]` array near the bottom of
`tests.cpp` (comment out entries) and re-run `make test`. Each entry is
`{"name", fn}`; `main()` runs them in order and prints `PASS`/`FAIL` per test
plus a check/test tally, exiting non-zero on any failure.

## Architecture

Three classes, one per `.h`/`.cpp` pair, plus two enum-only headers
(`ElevatorState.h`: `REST`/`UP`/`DOWN`; `Direction.h`: `UP`/`DOWN`).

- **`User`** — inert `{userId, name}` record. Not wired into the system; the
  driver uses it only to attach names to log lines.
- **`Elevator`** — one car. Owns its SCAN state and movement.
- **`ElevatorSystem`** — facade. Owns exactly 3 cars (ids 1..3) in an
  `unordered_map<int, unique_ptr<Elevator>>` keyed by id; building has floors
  `MIN_FLOOR`..`MAX_FLOOR` (1..10). Entry points: `requestPick`, `requestDrop`,
  `step`, `run`, `allIdle`, `getElevator`, `printStatus`.

### Simulation model

Time is discrete. `ElevatorSystem::step()` calls `Elevator::step()` on every
car; one tick moves a car at most one floor. `run()` steps until `allIdle()`
(bounded at 1000 ticks). Cars are independent — `step()` order does not matter.
There is no threading, no real time, no passenger objects.

### SCAN inside `Elevator`

Pending stops live in **two `std::set<int>`**: `upward` (served ascending) and
`downward` (served descending). `nextStop()` reads the current `ElevatorState`
and returns the next floor: continue in the current direction while that set has
work beyond `currentFloor`, otherwise turn to the far end of the other set;
from `REST`, head to the nearest pending floor in either set. `step()` moves one
floor toward `nextStop()` and clears the arrived floor.

**Turnaround rule** (in `step()`): while moving, a car always clears the arrived
floor from the *in-direction* set, but clears it from the *opposite* set only
when nothing remains further along the current sweep (`moreAbove`/`moreBelow`
false) — i.e. only at the point where the car actually reverses. This is what
stops an up-moving car from "serving" a down hall call it merely passes.

### Deferred drops — the non-obvious part

`requestDrop(elevatorId, targetFloor)` carries no boarding floor, and in the
normal flow it is called *before* the car has moved to pick the rider up, so
routing the destination by comparing it to `currentFloor` is wrong (this was a
real bug — see commit `e374c1d`). Instead:

1. `requestPick` inserts the boarding floor into `upward`/`downward` **and**
   calls `Elevator::registerPickup(floor)`, which pushes it onto
   `boardingQueue` (a `deque`, FIFO).
2. `requestDrop` calls `Elevator::bindDrop(dest)`, which pops the oldest
   boarding floor and stores `{boardingFloor, dest}` in `pendingDrops` — the
   destination is **not** yet an active stop. If `boardingQueue` is empty
   (rider already aboard), `bindDrop` returns `false` and `ElevatorSystem`
   falls back to routing `dest` by the car's current floor.
3. `Elevator::step()` calls `promotePendingDropsAt(currentFloor)` on arrival:
   any `pendingDrops` entry whose boarding floor matches is now moved into
   `upward`/`downward` (rider is aboard), and the pair erased.

FIFO binding assumes each rider calls `requestDrop` before the next rider calls
`requestPick` on the same car; the driver and tests honor this.

### Assignment — `ElevatorSystem::selectElevator`

For a call at `(floor, direction)`, one pass over the cars tracks two
candidates: the nearest car where `Elevator::canServe(floor, direction)` is true
(`REST`, or already moving toward `floor` in the same direction), and the
globally nearest car as a fallback. The preferred candidate wins if it exists,
else the fallback. Ties break to the lowest id (needed because `unordered_map`
iteration order is unspecified). `canServe` and `distanceTo` are the only hooks
`selectElevator` uses.

### Invariants worth preserving

- Every `pendingDrops` entry's boarding floor is present in `upward`/`downward`
  until the car reaches it, so `isIdle()` (both sets empty) remains a correct
  "no work" signal without consulting `pendingDrops`.
- `requestPick` rejects out-of-range floors, `UP` from the top floor, and
  `DOWN` from the bottom floor (throws `std::invalid_argument`); `requestDrop`
  rejects out-of-range floors and unknown elevator ids.
- `printStatus` / the driver iterate ids `1..NUM_ELEVATORS` explicitly for
  stable output despite the hash-map storage.
