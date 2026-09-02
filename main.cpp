#include <exception>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "Direction.h"
#include "ElevatorSystem.h"
#include "User.h"

// Driver: feeds hall calls into the system over simulated time and logs every
// assignment decision plus a per-tick fleet snapshot. Calls are interleaved
// with stepping so the three cars end up in different places / directions and
// the assignment logic actually has a choice to make.

namespace {

int g_tick = 0;

const char* dirName(Direction d) {
    return d == Direction::UP ? "UP" : "DOWN";
}

const char* stateName(ElevatorState s) {
    switch (s) {
        case ElevatorState::REST: return "REST";
        case ElevatorState::UP:   return "UP";
        case ElevatorState::DOWN: return "DOWN";
    }
    return "?";
}

void banner(const std::string& title) {
    std::cout << "\n========== " << title << " ==========\n";
}

std::string clock() {
    std::ostringstream os;
    os << "[t=" << std::setw(2) << std::setfill('0') << g_tick << "] ";
    return os.str();
}

void printFleet(const ElevatorSystem& system) {
    for (int id = 1; id <= ElevatorSystem::NUM_ELEVATORS; ++id) {
        const Elevator& car = system.getElevator(id);
        std::cout << "        E" << id << "  "
                  << std::setw(4) << std::left << stateName(car.getCurrentState())
                  << std::right << "  floor " << std::setw(2) << car.getCurrentFloor()
                  << "   up{";
        for (int f : car.getUpwardRequests()) std::cout << ' ' << f;
        std::cout << " } down{";
        for (int f : car.getDownwardRequests()) std::cout << ' ' << f;
        std::cout << " }";
        const auto& parked = car.getPendingDrops();
        if (!parked.empty()) {
            std::cout << " parked{";
            for (const auto& [boardingFloor, destFloor] : parked)
                std::cout << ' ' << boardingFloor << "->" << destFloor;
            std::cout << " }";
        }
        std::cout << '\n';
    }
}

// One rider: hall call, assignment, then the in-car floor selection.
void call(ElevatorSystem& system, const User& rider, int from, Direction dir, int to) {
    std::cout << clock() << rider.getName() << " calls from floor " << from
              << " heading " << dirName(dir) << ", wants floor " << to << '\n';
    try {
        int id = system.requestPick(from, dir);
        const Elevator& car = system.getElevator(id);
        std::cout << "        assigned E" << id << "  (was " << stateName(car.getCurrentState())
                  << " at floor " << car.getCurrentFloor() << ", "
                  << car.distanceTo(from) << " floor(s) from the call)\n";

        system.requestDrop(id, to);
        std::cout << "        " << rider.getName() << " boards E" << id
                  << ", selects floor " << to << '\n';
    } catch (const std::exception& e) {
        std::cout << "        REJECTED: " << e.what() << '\n';
    }
}

void advance(ElevatorSystem& system, int ticks) {
    for (int i = 0; i < ticks; ++i) {
        ++g_tick;
        system.step();
        std::cout << clock() << "fleet:\n";
        printFleet(system);
    }
}

void drain(ElevatorSystem& system) {
    int guard = 0;
    while (!system.allIdle() && guard++ < 500) {
        advance(system, 1);
    }
}

}  // namespace

int main() {
    ElevatorSystem system;

    User alice(1, "Alice");
    User bob(2, "Bob");
    User carol(3, "Carol");
    User dave(4, "Dave");
    User erin(5, "Erin");

    banner("Fleet start (3 cars, floors 1-10)");
    printFleet(system);

    banner("Wave 1: idle fleet, one UP call");
    call(system, alice, 2, Direction::UP, 7);
    advance(system, 2);

    banner("Wave 2: E1 climbing, a far DOWN call cannot use it");
    call(system, bob, 9, Direction::DOWN, 3);
    advance(system, 2);

    banner("Wave 3: an UP call with two cars already in motion");
    call(system, carol, 5, Direction::UP, 10);
    advance(system, 2);

    banner("Wave 4: burst of calls plus one invalid request");
    call(system, dave, 3, Direction::UP, 8);
    call(system, erin, 10, Direction::DOWN, 1);
    call(system, alice, 1, Direction::DOWN, 5);  // invalid: DOWN from the bottom
    advance(system, 2);

    banner("Drain: step until every car is REST");
    drain(system);

    banner("Fleet end");
    printFleet(system);
    return 0;
}
