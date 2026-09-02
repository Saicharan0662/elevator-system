#pragma once

#include <deque>
#include <set>
#include <utility>
#include <vector>

#include "Direction.h"
#include "ElevatorState.h"

// A single elevator car.
//
// Pending stops are split into two ordered sets so the SCAN algorithm can be
// expressed directly:
//   upward   - floors to visit while travelling UP   (served in ascending order)
//   downward - floors to visit while travelling DOWN (served in descending order)
//
// While in a given direction the car keeps moving that way until the matching
// set is exhausted, then reverses if the other set is non-empty, else goes REST.
//
// Rider destinations (requestDrop) cannot be routed immediately: a rider who
// called for a car at floor 7 to go DOWN is not aboard until the car reaches 7,
// so a destination of 1 must not become an active stop while the car is still
// deadheading up to 7. Destinations are therefore parked in `pendingDrops`
// keyed by boarding floor and promoted into upward/downward only when the car
// arrives at that boarding floor. `boardingQueue` binds each requestDrop to the
// oldest pick that is still waiting for a destination (FIFO: assumes a rider
// calls requestDrop before the next rider calls requestPick on the same car).
class Elevator {
private:
    ElevatorState currentState;
    int elevatorId;
    int currentFloor;
    std::set<int> upward;
    std::set<int> downward;

    // Picks that have no destination bound yet, oldest first.
    std::deque<int> boardingQueue;
    // {boardingFloor, destinationFloor} held until the car reaches boardingFloor.
    std::vector<std::pair<int, int>> pendingDrops;

    // Move every pendingDrops entry whose boarding floor == `floor` into the
    // matching directional set (the rider is now aboard).
    void promotePendingDropsAt(int floor);

public:
    Elevator(int elevatorId, int startFloor);

    // Getters
    ElevatorState getCurrentState() const;
    int getElevatorId() const;
    int getCurrentFloor() const;
    const std::set<int>& getUpwardRequests() const;
    const std::set<int>& getDownwardRequests() const;
    const std::vector<std::pair<int, int>>& getPendingDrops() const;

    // Setters
    void setCurrentState(ElevatorState state);
    void setCurrentFloor(int floor);

    // Request bookkeeping (called by ElevatorSystem once this car is chosen).
    void addUpwardRequest(int floor);
    void addDownwardRequest(int floor);

    // Record a pick at `boardingFloor` that is awaiting a destination.
    void registerPickup(int boardingFloor);

    // Bind `destinationFloor` to the oldest pick still awaiting a destination
    // and park it until the car reaches that boarding floor. Returns false if
    // no such pick exists (rider already aboard); the caller then routes the
    // destination directly.
    bool bindDrop(int destinationFloor);

    // --- SCAN algorithm ------------------------------------------------------

    // Advance the car by one floor according to SCAN, updating currentState and
    // dropping the served floor from the relevant set on arrival.
    void step();

    // True when both request sets are empty (car is or should become REST).
    bool isIdle() const;

    // Next floor this car will stop at given its current direction, or -1.
    int nextStop() const;

    // --- Assignment helpers (used by ElevatorSystem::selectElevator) --------

    // Absolute floor distance between this car and the given floor.
    int distanceTo(int floor) const;

    // True if this car can pick up a call at `floor` heading `direction`
    // without reversing: it is REST, or it is already moving toward `floor`
    // in the same direction.
    bool canServe(int floor, Direction direction) const;
};
