#pragma once

#include <set>

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
class Elevator {
private:
    ElevatorState currentState;
    int elevatorId;
    int currentFloor;
    std::set<int> upward;
    std::set<int> downward;

public:
    Elevator(int elevatorId, int startFloor);

    // Getters
    ElevatorState getCurrentState() const;
    int getElevatorId() const;
    int getCurrentFloor() const;
    const std::set<int>& getUpwardRequests() const;
    const std::set<int>& getDownwardRequests() const;

    // Setters
    void setCurrentState(ElevatorState state);
    void setCurrentFloor(int floor);

    // Request bookkeeping (called by ElevatorSystem once this car is chosen).
    void addUpwardRequest(int floor);
    void addDownwardRequest(int floor);

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
