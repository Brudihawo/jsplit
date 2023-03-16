main: ./main.cpp
	g++ -O3 -Wall -Wconversion -Wall -Wextra -Werror -std=c++17 -fuse-ld=gold ./main.cpp -o jgroup -lstdc++fs
