#pragma once

// Movement state of a single elevator car.
//   REST - not moving, no pending requests
//   UP   - currently serving requests in the upward direction
//   DOWN - currently serving requests in the downward direction
enum class ElevatorState {
    REST,
    UP,
    DOWN
};
