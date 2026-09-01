#include "ElevatorSystem.h"

#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {

const char* stateName(ElevatorState state) {
    switch (state) {
        case ElevatorState::REST: return "REST";
        case ElevatorState::UP:   return "UP  ";
        case ElevatorState::DOWN: return "DOWN";
    }
    return "?";
}

}  // namespace

ElevatorSystem::ElevatorSystem() {
    for (int id = 1; id <= NUM_ELEVATORS; ++id) {
        elevators.emplace(id, std::make_unique<Elevator>(id, MIN_FLOOR));
    }
}

// --- Private helpers -----------------------------------------------------

bool ElevatorSystem::isValidFloor(int floor) const {
    return floor >= MIN_FLOOR && floor <= MAX_FLOOR;
}

Elevator& ElevatorSystem::elevatorById(int elevatorId) {
    auto it = elevators.find(elevatorId);
    if (it == elevators.end()) {
        throw std::invalid_argument("unknown elevatorId: " + std::to_string(elevatorId));
    }
    return *it->second;
}

const Elevator& ElevatorSystem::elevatorById(int elevatorId) const {
    auto it = elevators.find(elevatorId);
    if (it == elevators.end()) {
        throw std::invalid_argument("unknown elevatorId: " + std::to_string(elevatorId));
    }
    return *it->second;
}

int ElevatorSystem::selectElevator(int floor, Direction direction) const {
    int preferredId = -1;
    int preferredDist = std::numeric_limits<int>::max();
    int nearestId = -1;
    int nearestDist = std::numeric_limits<int>::max();

    for (const auto& [id, car] : elevators) {
        int dist = car->distanceTo(floor);

        // Global fallback candidate.
        if (dist < nearestDist || (dist == nearestDist && id < nearestId)) {
            nearestDist = dist;
            nearestId = id;
        }

        // Preferred: REST, or already approaching `floor` the same way.
        if (car->canServe(floor, direction)) {
            if (dist < preferredDist || (dist == preferredDist && id < preferredId)) {
                preferredDist = dist;
                preferredId = id;
            }
        }
    }

    return preferredId != -1 ? preferredId : nearestId;
}

// --- Public API --------------------------------------------------------

int ElevatorSystem::requestPick(int floor, Direction direction) {
    if (!isValidFloor(floor)) {
        throw std::invalid_argument("floor out of range: " + std::to_string(floor));
    }
    if (floor == MAX_FLOOR && direction == Direction::UP) {
        throw std::invalid_argument("cannot request UP from the top floor");
    }
    if (floor == MIN_FLOOR && direction == Direction::DOWN) {
        throw std::invalid_argument("cannot request DOWN from the bottom floor");
    }

    int id = selectElevator(floor, direction);
    Elevator& car = elevatorById(id);

    if (direction == Direction::UP) {
        car.addUpwardRequest(floor);
    } else {
        car.addDownwardRequest(floor);
    }
    return id;
}

void ElevatorSystem::requestDrop(int elevatorId, int targetFloor) {
    if (!isValidFloor(targetFloor)) {
        throw std::invalid_argument("floor out of range: " + std::to_string(targetFloor));
    }

    Elevator& car = elevatorById(elevatorId);
    int here = car.getCurrentFloor();
    if (targetFloor == here) {
        return;  // passenger is already on their floor
    }

    if (targetFloor > here) {
        car.addUpwardRequest(targetFloor);
    } else {
        car.addDownwardRequest(targetFloor);
    }
}

void ElevatorSystem::step() {
    for (auto& [id, car] : elevators) {
        (void)id;
        car->step();
    }
}

void ElevatorSystem::run() {
    // Advance until every car is idle. Bounded so a logic error cannot hang.
    const int maxTicks = 1000;
    for (int tick = 0; tick < maxTicks; ++tick) {
        bool allIdle = true;
        for (const auto& [id, car] : elevators) {
            (void)id;
            if (!car->isIdle()) {
                allIdle = false;
                break;
            }
        }
        if (allIdle) {
            return;
        }
        step();
    }
}

const Elevator& ElevatorSystem::getElevator(int elevatorId) const {
    return elevatorById(elevatorId);
}

void ElevatorSystem::printStatus() const {
    for (int id = 1; id <= NUM_ELEVATORS; ++id) {  // stable, id-sorted order
        const Elevator& car = *elevators.at(id);

        std::cout << "  E" << id << "  " << stateName(car.getCurrentState())
                  << "  floor " << car.getCurrentFloor() << "   up{";
        for (int floor : car.getUpwardRequests()) {
            std::cout << ' ' << floor;
        }
        std::cout << " }  down{";
        for (int floor : car.getDownwardRequests()) {
            std::cout << ' ' << floor;
        }
        std::cout << " }\n";
    }
}
