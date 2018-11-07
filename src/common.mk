COMMON_CXXFLAGS=-m32 -g -O0 -Wall -Wextra -std=c++17 -g -I/usr/include/jsoncpp -MP -MMD -Werror
COMMON_LDFLAGS=-m32 -ljsoncpp -lSDL2_ttf -lSDL2 -lGL -lboost_program_options -lboost_filesystem -lboost_system
CXX=g++

-include $(SRC:.cpp=.d)

%.o: %.cpp
	$(CXX) -c -o $@ $(CXXFLAGS) $^
