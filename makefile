all:
	g++ main.cpp cmd.cpp util.cpp -I./include -O2 -ltorrent-rasterbar -o mintd
