#include <iostream>

#include "Direction.h"
#include "ElevatorSystem.h"
#include "User.h"

// Sample driver: shows the intended flow only. Class bodies are not implemented
// yet, so this compiles (see `make check`) but does not link/run until the .cpp
// files land.
int main() {
    ElevatorSystem system;  // 3 elevators, floors 1..10, all starting at REST

    // --- Users ------------------------------------------------------------
    User alice(1, "Alice");
    User bob(2, "Bob");
    User carol(3, "Carol");

    std::cout << "== Initial ==\n";
    system.printStatus();

    // --- Alice: floor 3 -> up -> floor 9 --------------------------------
    // Hall call. System weighs nearest REST car vs. nearest car already
    // heading up toward floor 3, assigns the closer one.
    int aliceCar = system.requestPick(3, Direction::UP);
    std::cout << "\nAlice (" << alice.getName() << ") assigned elevator "
              << aliceCar << " for pick-up at floor 3\n";
    // Alice steps in and chooses her destination.
    system.requestDrop(aliceCar, 9);

    // --- Bob: floor 7 -> down -> floor 1 -------------------------------
    int bobCar = system.requestPick(7, Direction::DOWN);
    std::cout << "Bob (" << bob.getName() << ") assigned elevator "
              << bobCar << " for pick-up at floor 7\n";
    system.requestDrop(bobCar, 1);

    // --- Carol: floor 4 -> up -> floor 6 ------------------------------
    // If Alice's car is already climbing past floor 4 in the UP direction,
    // SCAN lets it scoop Carol up on the way instead of waking another car.
    int carolCar = system.requestPick(4, Direction::UP);
    std::cout << "Carol (" << carol.getName() << ") assigned elevator "
              << carolCar << " for pick-up at floor 4\n";
    system.requestDrop(carolCar, 6);

    std::cout << "\n== After requests ==\n";
    system.printStatus();

    // --- Simulate ------------------------------------------------------
    std::cout << "\n== Stepping simulation ==\n";
    for (int tick = 1; tick <= 20; ++tick) {
        system.step();
        std::cout << "-- tick " << tick << " --\n";
        system.printStatus();
    }

    return 0;
}
