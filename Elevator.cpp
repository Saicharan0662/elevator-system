#include "Elevator.h"

#include <cstdlib>
#include <iterator>

Elevator::Elevator(int elevatorId, int startFloor)
    : currentState(ElevatorState::REST),
      elevatorId(elevatorId),
      currentFloor(startFloor) {}

// --- Getters ---------------------------------------------------------------

ElevatorState Elevator::getCurrentState() const {
    return currentState;
}

int Elevator::getElevatorId() const {
    return elevatorId;
}

int Elevator::getCurrentFloor() const {
    return currentFloor;
}

const std::set<int>& Elevator::getUpwardRequests() const {
    return upward;
}

const std::set<int>& Elevator::getDownwardRequests() const {
    return downward;
}

const std::vector<std::pair<int, int>>& Elevator::getPendingDrops() const {
    return pendingDrops;
}

// --- Setters -------------------------------------------------------------

void Elevator::setCurrentState(ElevatorState state) {
    currentState = state;
}

void Elevator::setCurrentFloor(int floor) {
    currentFloor = floor;
}

// --- Request bookkeeping -----------------------------------------------

void Elevator::addUpwardRequest(int floor) {
    upward.insert(floor);
}

void Elevator::addDownwardRequest(int floor) {
    downward.insert(floor);
}

void Elevator::registerPickup(int boardingFloor) {
    boardingQueue.push_back(boardingFloor);
}

bool Elevator::bindDrop(int destinationFloor) {
    if (boardingQueue.empty()) {
        return false;  // no waiting pick to attach to; rider is already aboard
    }
    int boardingFloor = boardingQueue.front();
    boardingQueue.pop_front();
    pendingDrops.emplace_back(boardingFloor, destinationFloor);
    return true;
}

void Elevator::promotePendingDropsAt(int floor) {
    for (auto it = pendingDrops.begin(); it != pendingDrops.end();) {
        if (it->first != floor) {
            ++it;
            continue;
        }
        int dest = it->second;
        if (dest > floor) {
            upward.insert(dest);
        } else if (dest < floor) {
            downward.insert(dest);
        }
        it = pendingDrops.erase(it);
    }
}

// --- SCAN algorithm ---------------------------------------------------

bool Elevator::isIdle() const {
    return upward.empty() && downward.empty();
}

int Elevator::nextStop() const {
    if (isIdle()) {
        return -1;
    }

    switch (currentState) {
        case ElevatorState::UP: {
            // Keep sweeping up: nearest pending floor at or above us.
            auto it = upward.lower_bound(currentFloor);
            if (it != upward.end()) {
                return *it;
            }
            // Up-sweep finished: turn around to the far end of the down set.
            if (!downward.empty()) {
                return *downward.rbegin();
            }
            return *upward.begin();
        }
        case ElevatorState::DOWN: {
            // Keep sweeping down: nearest pending floor at or below us.
            auto it = downward.upper_bound(currentFloor);  // first strictly above
            if (it != downward.begin()) {
                return *std::prev(it);
            }
            // Down-sweep finished: turn around to the far end of the up set.
            if (!upward.empty()) {
                return *upward.begin();
            }
            return *downward.begin();
        }
        case ElevatorState::REST:
        default: {
            // Not moving yet: head toward the closest pending floor overall.
            int best = -1;
            int bestDist = -1;
            for (int floor : upward) {
                int d = std::abs(floor - currentFloor);
                if (bestDist < 0 || d < bestDist) {
                    bestDist = d;
                    best = floor;
                }
            }
            for (int floor : downward) {
                int d = std::abs(floor - currentFloor);
                if (bestDist < 0 || d < bestDist) {
                    bestDist = d;
                    best = floor;
                }
            }
            return best;
        }
    }
}

void Elevator::step() {
    if (isIdle()) {
        currentState = ElevatorState::REST;
        return;
    }

    int target = nextStop();

    // A stop registered at the floor we are already on: serve it in place.
    if (target == currentFloor) {
        promotePendingDropsAt(currentFloor);
        upward.erase(currentFloor);
        downward.erase(currentFloor);
        if (isIdle()) {
            currentState = ElevatorState::REST;
        }
        return;
    }

    currentState = (target > currentFloor) ? ElevatorState::UP : ElevatorState::DOWN;
    currentFloor += (currentState == ElevatorState::UP) ? 1 : -1;

    // A rider boarding here now has a destination; fold it into the sets before
    // deciding what this stop serves, so it counts toward the turnaround test.
    promotePendingDropsAt(currentFloor);

    // Serve the floor we just arrived at. The in-direction set is always
    // cleared; the opposite set is cleared only when this floor is the
    // turnaround point (nothing left to serve further along the sweep), which
    // is exactly when a waiting passenger for the other direction can board.
    if (currentState == ElevatorState::UP) {
        upward.erase(currentFloor);
        bool moreAbove = !upward.empty() && *upward.rbegin() > currentFloor;
        if (!moreAbove) {
            downward.erase(currentFloor);
        }
    } else {
        downward.erase(currentFloor);
        bool moreBelow = !downward.empty() && *downward.begin() < currentFloor;
        if (!moreBelow) {
            upward.erase(currentFloor);
        }
    }

    if (isIdle()) {
        currentState = ElevatorState::REST;
    }
}

// --- Assignment helpers --------------------------------------------------

int Elevator::distanceTo(int floor) const {
    return std::abs(currentFloor - floor);
}

bool Elevator::canServe(int floor, Direction direction) const {
    if (currentState == ElevatorState::REST) {
        return true;
    }
    if (direction == Direction::UP) {
        return currentState == ElevatorState::UP && currentFloor <= floor;
    }
    return currentState == ElevatorState::DOWN && currentFloor >= floor;
}
