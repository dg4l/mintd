#pragma once
#include "common_include.hpp"
#define MAX_PACKET_SIZE 10000
#define MAX_URL_LEN 8000 

enum CMDS{
    CMD_ADD,
    CMD_QUERY_STATUS,
    CMD_PAUSE_ALL,
    CMD_PAUSE_IDX,
    CMD_RESUME_ALL,
    CMD_RESUME_IDX,
    CMD_REMOVE_TORRENT,
    CMD_QUERY_STATS,
    CMD_QUERY_STATS_ALLTIME,
    CMD_QUERY_TORRENT_STATUS
};

typedef struct ServerContext{
    const char* socket_path;
    struct sockaddr_un server_addr;
    std::string outgoing_interface;
    std::string incoming_interface;
    bool strict_bind;
    lt::settings_pack settings;
    lt::session* session;
    int srv_fd;
    bool done;
}ServerContext;

typedef struct ResponseContext{
    std::string message;
}ResponseContext;

typedef struct ParsedAddPacket{
    std::string url;
    std::string save_path;
}ParsedAddPacket;
