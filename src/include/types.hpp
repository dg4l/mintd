#pragma once
#include "common_include.hpp"
#define MAX_PACKET_SIZE 10000
#define MAX_URL_LEN 8000 

enum CMDS {
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

typedef struct ResponseContext {
    std::string message;
} ResponseContext;

typedef struct Server {
    const char* socket_path;
    struct sockaddr_un server_addr;
    std::string outgoing_interface;
    std::string incoming_interface;
    bool strict_bind;
    lt::session* session;
    lt::settings_pack settings;
    int srv_fd;
    bool done;
    // TODO: finish refactor
    bool parse_config_file();
    void handle_packet(int client_fd, char* packet); 
    void handle_client(int client);
    void handle_alerts(void);
    bool cmd_remove_torrent(ResponseContext* response, unsigned int idx);
    bool cmd_resume_all(ResponseContext* response);
    bool cmd_resume_idx(ResponseContext* response, unsigned int idx);
    bool cmd_add_torrent(ResponseContext* response, char* packet);
    bool cmd_invalid(ResponseContext* response);
    bool cmd_pause_all(ResponseContext* response);
    bool cmd_pause_idx(ResponseContext* response, uint16_t idx);
    bool cmd_query_torrent_status(ResponseContext* response, unsigned int idx);
    bool cmd_status(ResponseContext* response);
    bool cmd_query_stats(ResponseContext* response);
    bool handle_command(int client_fd, char* packet);
} Server;

typedef struct ParsedAddPacket {
    std::string url;
    std::string save_path;
} ParsedAddPacket;
