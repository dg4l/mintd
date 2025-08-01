all:
	g++ main.cpp cmd.cpp util.cpp -I./include -O2 -ltorrent-rasterbar -Wunused-variable -o mintd
