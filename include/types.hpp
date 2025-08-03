#pragma once
#include "common_include.hpp"
#define DEBUG 0

enum CMDS{
    CMD_ADD,
    CMD_QUERY_STATUS,
    CMD_PAUSE_ALL,
    CMD_PAUSE_IDX,
    CMD_RESUME_ALL,
    CMD_RESUME_IDX,
    CMD_REMOVE_TORRENT,
    CMD_QUERY_STATS,
    CMD_QUERY_STATS_ALLTIME
};

typedef struct ServerContext{
    const char* socket_path;
    struct sockaddr_un server_addr;
    lt::settings_pack settings;
    lt::session* session;
    int srv_fd;
    bool done;
}ServerContext;

typedef struct ResponseContext{
    std::string message;
}ResponseContext;
