CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -g

# Class implementations shared by the driver and the tests.
LIB_SRCS := User.cpp Elevator.cpp ElevatorSystem.cpp
LIB_OBJS := $(LIB_SRCS:.cpp=.o)
HDRS     := $(wildcard *.h)

APP_BIN  := elevator
TEST_BIN := run_tests

.PHONY: all run test check clean

# Build the sample driver.
all: $(APP_BIN)

$(APP_BIN): $(LIB_OBJS) main.o
	$(CXX) $(CXXFLAGS) -o $@ $^

$(TEST_BIN): $(LIB_OBJS) tests.o
	$(CXX) $(CXXFLAGS) -o $@ $^

# Build (if needed) and run the driver.
run: $(APP_BIN)
	./$(APP_BIN)

# Build (if needed) and run the unit tests.
test: $(TEST_BIN)
	./$(TEST_BIN)

%.o: %.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Syntax-only pass over every translation unit.
check:
	@for src in $(LIB_SRCS) main.cpp tests.cpp; do \
		echo "checking $$src"; \
		$(CXX) $(CXXFLAGS) -fsyntax-only $$src || exit 1; \
	done

clean:
	$(RM) *.o $(APP_BIN) $(TEST_BIN)
