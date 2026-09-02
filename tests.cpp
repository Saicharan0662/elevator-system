// Minimal dependency-free unit tests for the elevator system.
//
// Build/run:  make test
//
// Harness: CHECK / CHECK_EQ / CHECK_VEC / CHECK_THROWS increment counters and
// print the failing file:line. main() prints a summary and returns non-zero if
// anything failed.

#include <exception>
#include <iostream>
#include <string>
#include <vector>

#include "Direction.h"
#include "Elevator.h"
#include "ElevatorSystem.h"
#include "User.h"

namespace {

using D = Direction;

int g_checks = 0;
int g_failures = 0;
const char* g_current = "";

#define CHECK(cond)                                                            \
    do {                                                                      \
        ++g_checks;                                                           \
        if (!(cond)) {                                                        \
            ++g_failures;                                                     \
            std::cout << "  FAIL [" << g_current << "] " << __FILE__ << ":"   \
                      << __LINE__ << "  CHECK(" #cond ")\n";                  \
        }                                                                     \
    } while (0)

#define CHECK_EQ(a, b)                                                         \
    do {                                                                      \
        ++g_checks;                                                           \
        auto _va = (a);                                                       \
        auto _vb = (b);                                                       \
        if (!(_va == _vb)) {                                                  \
            ++g_failures;                                                     \
            std::cout << "  FAIL [" << g_current << "] " << __FILE__ << ":"   \
                      << __LINE__ << "  CHECK_EQ(" #a ", " #b ")  got "       \
                      << _va << " vs " << _vb << "\n";                        \
        }                                                                     \
    } while (0)

#define CHECK_THROWS(expr)                                                     \
    do {                                                                      \
        ++g_checks;                                                           \
        bool _threw = false;                                                  \
        try {                                                                 \
            (void)(expr);                                                     \
        } catch (...) {                                                       \
            _threw = true;                                                    \
        }                                                                     \
        if (!_threw) {                                                        \
            ++g_failures;                                                     \
            std::cout << "  FAIL [" << g_current << "] " << __FILE__ << ":"   \
                      << __LINE__ << "  CHECK_THROWS(" #expr ")\n";           \
        }                                                                     \
    } while (0)

std::string vecStr(const std::vector<int>& v) {
    std::string s = "[";
    for (size_t i = 0; i < v.size(); ++i) {
        if (i) s += ' ';
        s += std::to_string(v[i]);
    }
    return s + "]";
}

#define CHECK_VEC(actual, expected)                                            \
    do {                                                                      \
        ++g_checks;                                                           \
        std::vector<int> _a = (actual);                                       \
        std::vector<int> _e = (expected);                                     \
        if (_a != _e) {                                                       \
            ++g_failures;                                                     \
            std::cout << "  FAIL [" << g_current << "] " << __FILE__ << ":"   \
                      << __LINE__ << "  CHECK_VEC(" #actual ", " #expected ")\n" \
                      << "    actual   = " << vecStr(_a) << "\n"              \
                      << "    expected = " << vecStr(_e) << "\n";            \
        }                                                                     \
    } while (0)

// ------------------------------------------------------------------ helpers

void advance(ElevatorSystem& s, int ticks) {
    for (int i = 0; i < ticks; ++i) s.step();
}

std::vector<int> traceSystemFloors(ElevatorSystem& s, int id, int maxTicks = 300) {
    std::vector<int> t{s.getElevator(id).getCurrentFloor()};
    for (int i = 0; i < maxTicks && !s.allIdle(); ++i) {
        s.step();
        t.push_back(s.getElevator(id).getCurrentFloor());
    }
    return t;
}

std::vector<int> traceElevatorFloors(Elevator& e, int maxTicks = 300) {
    std::vector<int> t{e.getCurrentFloor()};
    for (int i = 0; i < maxTicks && !e.isIdle(); ++i) {
        e.step();
        t.push_back(e.getCurrentFloor());
    }
    return t;
}

int firstIndex(const std::vector<int>& v, int x) {
    for (size_t i = 0; i < v.size(); ++i)
        if (v[i] == x) return static_cast<int>(i);
    return -1;
}

int lastIndex(const std::vector<int>& v, int x) {
    for (int i = static_cast<int>(v.size()) - 1; i >= 0; --i)
        if (v[i] == x) return i;
    return -1;
}

bool isMonotonicAsc(const std::vector<int>& v) {
    for (size_t i = 1; i < v.size(); ++i)
        if (v[i] < v[i - 1]) return false;
    return true;
}

int has(const std::set<int>& s, int x) { return static_cast<int>(s.count(x)); }

// ------------------------------------------------------------------- tests

void test_user() {
    User u(7, "Ada");
    CHECK_EQ(u.getUserId(), 7);
    CHECK_EQ(u.getName(), std::string("Ada"));

    u.setUserId(42);
    u.setName("Grace");
    CHECK_EQ(u.getUserId(), 42);
    CHECK_EQ(u.getName(), std::string("Grace"));
}

void test_elevator_basics() {
    Elevator e(2, 4);
    CHECK_EQ(e.getElevatorId(), 2);
    CHECK_EQ(e.getCurrentFloor(), 4);
    CHECK(e.getCurrentState() == ElevatorState::REST);
    CHECK(e.isIdle());
    CHECK(e.getUpwardRequests().empty());
    CHECK(e.getDownwardRequests().empty());
    CHECK(e.getPendingDrops().empty());

    CHECK_EQ(e.distanceTo(4), 0);
    CHECK_EQ(e.distanceTo(9), 5);
    CHECK_EQ(e.distanceTo(1), 3);

    e.addUpwardRequest(8);
    e.addDownwardRequest(2);
    CHECK(!e.isIdle());
    CHECK_EQ(has(e.getUpwardRequests(), 8), 1);
    CHECK_EQ(has(e.getDownwardRequests(), 2), 1);

    e.setCurrentFloor(6);
    e.setCurrentState(ElevatorState::DOWN);
    CHECK_EQ(e.getCurrentFloor(), 6);
    CHECK(e.getCurrentState() == ElevatorState::DOWN);
}

void test_elevator_canServe() {
    Elevator idle(1, 5);
    CHECK(idle.canServe(1, D::UP));    // REST serves any call
    CHECK(idle.canServe(10, D::DOWN));

    Elevator up(2, 3);
    up.addUpwardRequest(9);
    up.setCurrentState(ElevatorState::UP);
    CHECK(up.canServe(3, D::UP));      // at current floor, same direction
    CHECK(up.canServe(6, D::UP));      // ahead, same direction
    CHECK(!up.canServe(2, D::UP));     // behind
    CHECK(!up.canServe(6, D::DOWN));   // opposite direction

    Elevator dn(3, 8);
    dn.addDownwardRequest(2);
    dn.setCurrentState(ElevatorState::DOWN);
    CHECK(dn.canServe(8, D::DOWN));
    CHECK(dn.canServe(5, D::DOWN));
    CHECK(!dn.canServe(9, D::DOWN));   // behind (above)
    CHECK(!dn.canServe(5, D::UP));     // opposite direction
}

void test_elevator_scan_up_order() {
    Elevator e(1, 1);
    e.addUpwardRequest(5);
    e.addUpwardRequest(3);
    CHECK_EQ(e.nextStop(), 3);  // nearest ahead first

    CHECK_VEC(traceElevatorFloors(e), (std::vector<int>{1, 2, 3, 4, 5}));
    CHECK(e.isIdle());
    CHECK(e.getCurrentState() == ElevatorState::REST);
}

void test_elevator_scan_down_order() {
    Elevator e(1, 10);
    e.addDownwardRequest(4);
    e.addDownwardRequest(8);
    CHECK_EQ(e.nextStop(), 8);  // nearest below first

    CHECK_VEC(traceElevatorFloors(e), (std::vector<int>{10, 9, 8, 7, 6, 5, 4}));
    CHECK(e.isIdle());
}

void test_elevator_scan_reversal() {
    // From floor 5, a down call at 3 (dist 2) is nearer than an up call at 9
    // (dist 4): serve 3 first, then reverse up to 9.
    Elevator e(1, 5);
    e.addUpwardRequest(9);
    e.addDownwardRequest(3);
    CHECK_EQ(e.nextStop(), 3);

    std::vector<int> t = traceElevatorFloors(e);
    CHECK(firstIndex(t, 3) != -1);
    CHECK(firstIndex(t, 3) < firstIndex(t, 9));
    CHECK_EQ(t.back(), 9);
    CHECK(e.isIdle());
}

void test_elevator_turnaround_skips_opposite_call() {
    // Climbing to 9 with a DOWN call parked at 5: the car must pass 5 on the
    // up-sweep without clearing it, then serve it coming back down.
    Elevator e(1, 1);
    e.addUpwardRequest(9);
    e.addDownwardRequest(5);

    int guard = 0;
    while (e.getCurrentFloor() != 5 && guard++ < 100) e.step();
    CHECK(e.getCurrentState() == ElevatorState::UP);
    CHECK_EQ(has(e.getDownwardRequests(), 5), 1);  // passed, not served

    guard = 0;
    while (!e.isIdle() && guard++ < 100) e.step();
    CHECK(e.isIdle());
    CHECK_EQ(e.getCurrentFloor(), 5);              // ended where the down call was served
    CHECK_EQ(has(e.getDownwardRequests(), 5), 0);
}

void test_elevator_deferred_drop() {
    // Boarding floor 7 (DOWN), destination 1. Destination must stay parked -
    // never an active stop - until the car actually reaches floor 7.
    Elevator e(1, 1);
    e.addDownwardRequest(7);
    e.registerPickup(7);
    CHECK(e.bindDrop(1));
    CHECK_EQ(static_cast<int>(e.getPendingDrops().size()), 1);
    CHECK_EQ(has(e.getDownwardRequests(), 1), 0);
    CHECK_EQ(has(e.getUpwardRequests(), 1), 0);

    bool seenDestBeforePickup = false;
    bool reachedPickup = false;
    int guard = 0;
    while (!e.isIdle() && guard++ < 100) {
        e.step();
        if (e.getCurrentFloor() == 7) reachedPickup = true;
        if (!reachedPickup && has(e.getDownwardRequests(), 1) > 0)
            seenDestBeforePickup = true;
    }
    CHECK(reachedPickup);
    CHECK(!seenDestBeforePickup);          // the whole point of the fix
    CHECK_EQ(e.getCurrentFloor(), 1);      // delivered to destination
    CHECK(e.isIdle());
    CHECK(e.getPendingDrops().empty());
}

void test_elevator_fifo_drop_binding() {
    // Two picks registered before either drop; drops bind oldest-first.
    Elevator e(1, 1);
    e.addUpwardRequest(3);
    e.addUpwardRequest(6);
    e.registerPickup(3);
    e.registerPickup(6);
    CHECK(e.bindDrop(9));  // -> boarding floor 3
    CHECK(e.bindDrop(8));  // -> boarding floor 6

    const auto& pd = e.getPendingDrops();
    CHECK_EQ(static_cast<int>(pd.size()), 2);
    CHECK_EQ(pd[0].first, 3);
    CHECK_EQ(pd[0].second, 9);
    CHECK_EQ(pd[1].first, 6);
    CHECK_EQ(pd[1].second, 8);

    CHECK_VEC(traceElevatorFloors(e),
              (std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8, 9}));
    CHECK(e.getPendingDrops().empty());
    CHECK(e.isIdle());
}

void test_system_construction() {
    ElevatorSystem s;
    CHECK(s.allIdle());
    for (int id = 1; id <= 3; ++id) {
        const Elevator& car = s.getElevator(id);
        CHECK_EQ(car.getElevatorId(), id);
        CHECK_EQ(car.getCurrentFloor(), 1);
        CHECK(car.getCurrentState() == ElevatorState::REST);
    }
    CHECK_THROWS(s.getElevator(0));
    CHECK_THROWS(s.getElevator(4));
}

void test_system_assignment_across_three_cars() {
    ElevatorSystem s;

    // Idle fleet: nearest wins, ties break to the lowest id.
    CHECK_EQ(s.requestPick(5, D::UP), 1);
    s.requestDrop(1, 9);
    advance(s, 2);  // E1 climbing, state UP

    // E1 is UP -> a DOWN call must go to an idle car (E2, lowest of the tie).
    int id2 = s.requestPick(9, D::DOWN);
    CHECK_EQ(id2, 2);
    CHECK_EQ(has(s.getElevator(2).getDownwardRequests(), 9), 1);
    s.requestDrop(2, 2);
    advance(s, 2);  // E2 deadheading UP; E1 higher

    // E1 UP, E2 UP(deadhead) -> only E3 can take another DOWN call.
    int id3 = s.requestPick(4, D::DOWN);
    CHECK_EQ(id3, 3);
    CHECK_EQ(has(s.getElevator(3).getDownwardRequests(), 4), 1);
    s.requestDrop(3, 1);

    CHECK(!s.getElevator(1).isIdle());
    CHECK(!s.getElevator(2).isIdle());
    CHECK(!s.getElevator(3).isIdle());

    s.run();
    CHECK(s.allIdle());
}

void test_system_assignment_prefers_nearest_eligible() {
    ElevatorSystem s;
    CHECK_EQ(s.requestPick(9, D::UP), 1);
    s.requestDrop(1, 10);
    advance(s, 4);  // E1 UP around floor 5

    // Approaching E1 (dist ~1) beats the idle cars (dist ~5).
    int idNear = s.requestPick(6, D::UP);
    CHECK_EQ(idNear, 1);
    s.requestDrop(1, 8);

    // A call below E1's position: E1 cannot serve without reversing -> idle car.
    int idBehind = s.requestPick(3, D::UP);
    CHECK_EQ(idBehind, 2);
}

void test_system_assignment_fallback_when_none_can_serve() {
    ElevatorSystem s;

    CHECK_EQ(s.requestPick(9, D::UP), 1);
    s.requestDrop(1, 10);
    advance(s, 3);  // E1 UP

    int e2 = s.requestPick(3, D::UP);  // below E1 -> idle E2
    CHECK_EQ(e2, 2);
    s.requestDrop(2, 7);
    advance(s, 1);

    int e3 = s.requestPick(1, D::UP);  // below E1 and E2 -> idle E3
    CHECK_EQ(e3, 3);
    s.requestDrop(3, 8);
    advance(s, 2);  // let E3 commit to UP

    // Every car is now heading UP; a DOWN call matches none of them.
    for (int id = 1; id <= 3; ++id)
        CHECK(!s.getElevator(id).canServe(9, D::DOWN));

    int fb = s.requestPick(9, D::DOWN);
    CHECK(fb >= 1 && fb <= 3);
    for (int id = 1; id <= 3; ++id)  // fell back to the globally nearest car
        CHECK(s.getElevator(fb).distanceTo(9) <= s.getElevator(id).distanceTo(9));
}

void test_system_pickup_then_drop_order_up() {
    ElevatorSystem s;
    int id = s.requestPick(3, D::UP);
    s.requestDrop(id, 8);

    std::vector<int> t = traceSystemFloors(s, id);
    CHECK(firstIndex(t, 3) != -1);
    CHECK(firstIndex(t, 3) < firstIndex(t, 8));  // pick up before drop off
    CHECK(isMonotonicAsc(t));                    // one clean up-sweep
    CHECK_EQ(t.back(), 8);
    CHECK(s.allIdle());
}

void test_system_deferred_drop_not_served_early() {
    // The originally reported bug: call DOWN from 7, select floor 1, before the
    // car has moved. Floor 1 must not be served until after floor 7.
    ElevatorSystem s;
    int id = s.requestPick(7, D::DOWN);
    s.requestDrop(id, 1);
    CHECK_EQ(has(s.getElevator(id).getDownwardRequests(), 1), 0);  // parked, not active

    std::vector<int> t = traceSystemFloors(s, id);
    CHECK(firstIndex(t, 7) != -1);
    CHECK(lastIndex(t, 1) > firstIndex(t, 7));  // reached destination after pickup
    CHECK_EQ(t.back(), 1);
    CHECK(s.allIdle());
}

void test_system_run_drains() {
    ElevatorSystem s;
    s.requestPick(5, D::UP);
    s.requestDrop(1, 9);
    int b = s.requestPick(8, D::DOWN);
    s.requestDrop(b, 2);
    advance(s, 2);
    int c = s.requestPick(6, D::DOWN);
    s.requestDrop(c, 1);

    CHECK(!s.allIdle());
    s.run();
    CHECK(s.allIdle());
}

void test_system_step_on_idle_is_noop() {
    ElevatorSystem s;
    CHECK(s.allIdle());
    advance(s, 5);
    CHECK(s.allIdle());
    for (int id = 1; id <= 3; ++id) {
        CHECK_EQ(s.getElevator(id).getCurrentFloor(), 1);
        CHECK(s.getElevator(id).getCurrentState() == ElevatorState::REST);
    }
}

void test_edge_invalid_requests() {
    ElevatorSystem s;
    CHECK_THROWS(s.requestPick(0, D::UP));
    CHECK_THROWS(s.requestPick(11, D::UP));
    CHECK_THROWS(s.requestPick(-2, D::DOWN));
    CHECK_THROWS(s.requestPick(1, D::DOWN));   // DOWN from the bottom floor
    CHECK_THROWS(s.requestPick(10, D::UP));    // UP from the top floor
    CHECK_THROWS(s.requestDrop(1, 0));
    CHECK_THROWS(s.requestDrop(1, 11));

    bool ok = true;
    try {
        s.requestPick(1, D::UP);    // UP from the bottom is fine
        s.requestPick(10, D::DOWN); // DOWN from the top is fine
    } catch (...) {
        ok = false;
    }
    CHECK(ok);
}

void test_edge_unknown_elevator() {
    ElevatorSystem s;
    CHECK_THROWS(s.requestDrop(0, 5));
    CHECK_THROWS(s.requestDrop(4, 5));
    CHECK_THROWS(s.requestDrop(999, 5));
}

void test_edge_drop_without_pickup() {
    // No matching pick: rider is treated as already aboard, routed from the
    // car's current floor.
    ElevatorSystem s;
    s.requestDrop(1, 6);
    CHECK_EQ(has(s.getElevator(1).getUpwardRequests(), 6), 1);
    CHECK(s.getElevator(1).getPendingDrops().empty());

    ElevatorSystem s2;
    s2.requestDrop(1, 1);  // destination == current floor -> no-op
    CHECK(s2.getElevator(1).isIdle());
}

void test_edge_drop_to_boarding_floor() {
    ElevatorSystem s;
    int id = s.requestPick(5, D::UP);
    s.requestDrop(id, 5);  // destination equals boarding floor

    std::vector<int> t = traceSystemFloors(s, id);
    CHECK(firstIndex(t, 5) != -1);
    CHECK_EQ(t.back(), 5);
    CHECK(s.getElevator(id).getPendingDrops().empty());
    CHECK(s.allIdle());
}

void test_edge_inconsistent_direction_drop() {
    // Rider calls UP at 5 but selects floor 2. Still delivered: the car picks
    // up at 5, then reverses and runs down to 2.
    ElevatorSystem s;
    int id = s.requestPick(5, D::UP);
    s.requestDrop(id, 2);

    std::vector<int> t = traceSystemFloors(s, id);
    CHECK(firstIndex(t, 5) != -1);
    CHECK(firstIndex(t, 5) < lastIndex(t, 2));
    CHECK_EQ(t.back(), 2);
    CHECK(s.allIdle());
}

struct NamedTest {
    const char* name;
    void (*fn)();
};

const NamedTest kTests[] = {
    {"user", test_user},
    {"elevator_basics", test_elevator_basics},
    {"elevator_canServe", test_elevator_canServe},
    {"elevator_scan_up_order", test_elevator_scan_up_order},
    {"elevator_scan_down_order", test_elevator_scan_down_order},
    {"elevator_scan_reversal", test_elevator_scan_reversal},
    {"elevator_turnaround_skips_opposite_call", test_elevator_turnaround_skips_opposite_call},
    {"elevator_deferred_drop", test_elevator_deferred_drop},
    {"elevator_fifo_drop_binding", test_elevator_fifo_drop_binding},
    {"system_construction", test_system_construction},
    {"system_assignment_across_three_cars", test_system_assignment_across_three_cars},
    {"system_assignment_prefers_nearest_eligible", test_system_assignment_prefers_nearest_eligible},
    {"system_assignment_fallback_when_none_can_serve", test_system_assignment_fallback_when_none_can_serve},
    {"system_pickup_then_drop_order_up", test_system_pickup_then_drop_order_up},
    {"system_deferred_drop_not_served_early", test_system_deferred_drop_not_served_early},
    {"system_run_drains", test_system_run_drains},
    {"system_step_on_idle_is_noop", test_system_step_on_idle_is_noop},
    {"edge_invalid_requests", test_edge_invalid_requests},
    {"edge_unknown_elevator", test_edge_unknown_elevator},
    {"edge_drop_without_pickup", test_edge_drop_without_pickup},
    {"edge_drop_to_boarding_floor", test_edge_drop_to_boarding_floor},
    {"edge_inconsistent_direction_drop", test_edge_inconsistent_direction_drop},
};

}  // namespace

int main() {
    int failedTests = 0;
    for (const auto& t : kTests) {
        g_current = t.name;
        int before = g_failures;
        t.fn();
        bool passed = (g_failures == before);
        std::cout << (passed ? "PASS  " : "FAIL  ") << t.name << "\n";
        if (!passed) ++failedTests;
    }

    std::cout << "\n"
              << (g_checks - g_failures) << "/" << g_checks << " checks passed, "
              << (sizeof(kTests) / sizeof(kTests[0]) - failedTests) << "/"
              << (sizeof(kTests) / sizeof(kTests[0])) << " tests passed\n";
    return failedTests == 0 ? 0 : 1;
}
