CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -g
TARGET   := elevator

SRCS := $(wildcard *.cpp)
OBJS := $(SRCS:.cpp=.o)
HDRS := $(wildcard *.h)

.PHONY: all check clean

# Full build. Fails at the link stage until the class .cpp files are added.
all: $(TARGET)

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
