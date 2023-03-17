main: ./main.cpp
	g++ -O3 -Wall -Wconversion -Wall -Wextra -Werror -std=c++2a -fuse-ld=gold ./main.cpp -o jsplit -lstdc++fs
