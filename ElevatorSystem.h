#pragma once

#include <memory>
#include <vector>

#include "Direction.h"
#include "Elevator.h"

// Facade over the bank of elevators.
//
// Owns exactly NUM_ELEVATORS cars via unique_ptr (composition: cars live and
// die with the system). Serves a building with floors MIN_FLOOR..MAX_FLOOR.
//
// Typical flow:
//   int id = system.requestPick(3, Direction::UP);  // call from floor 3
//   system.requestDrop(id, 8);                       // once inside, pick floor 8
//   system.step();  // advance the simulation one tick
class ElevatorSystem {
public:
    static constexpr int NUM_ELEVATORS = 3;
    static constexpr int MIN_FLOOR = 1;
    static constexpr int MAX_FLOOR = 10;

private:
    std::vector<std::unique_ptr<Elevator>> elevators;

    // Pick the best car for a call at `floor` heading `direction`.
    // Candidates are the nearest REST car and the nearest car already
    // approaching `floor` in the same direction; the closer of the two wins.
    // Returns an index into `elevators`.
    int selectElevator(int floor, Direction direction) const;

    bool isValidFloor(int floor) const;
    Elevator& elevatorById(int elevatorId);
    const Elevator& elevatorById(int elevatorId) const;

public:
    ElevatorSystem();

    // A user on `floor` presses the up/down hall button.
    // Assigns a car, registers the pick-up stop, and returns its elevatorId.
    int requestPick(int floor, Direction direction);

    // A user already inside `elevatorId` selects `targetFloor`.
    void requestDrop(int elevatorId, int targetFloor);

    // Advance every elevator by one floor (one simulation tick).
    void step();

    // Run ticks until every elevator is idle.
    void run();

    // Read-only access for drivers / tests.
    const Elevator& getElevator(int elevatorId) const;

    // Print each car's id, state, floor, and pending stops.
    void printStatus() const;
};
