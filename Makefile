CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -g
TARGET   := elevator

SRCS := $(wildcard *.cpp)
OBJS := $(SRCS:.cpp=.o)
HDRS := $(wildcard *.h)

.PHONY: all run check clean

# Full build.
all: $(TARGET)

# Build (if needed) and run the driver.
run: $(TARGET)
	./$(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Syntax-only pass over every translation unit. Works even with no
# implementations, so use this to validate the declarations.
check:
	@for src in $(SRCS); do \
		echo "checking $$src"; \
		$(CXX) $(CXXFLAGS) -fsyntax-only $$src || exit 1; \
	done

clean:
	$(RM) $(OBJS) $(TARGET)
