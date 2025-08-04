all:
	g++ ./src/main.cpp ./src/cmd.cpp ./src/util.cpp -I./src/include -O2 -ltorrent-rasterbar -Wunused-variable -o mintd
