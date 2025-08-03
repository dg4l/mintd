#include "common_include.hpp"
#include "types.hpp"
#include "util.hpp"

bool cmd_add_torrent(ServerContext* ctx, ResponseContext* response, char* URL);
bool cmd_resume_all(ServerContext* ctx, ResponseContext* response);
bool cmd_remove_torrent(ServerContext* ctx, ResponseContext* response, unsigned int idx);
bool cmd_pause_idx(ServerContext* ctx, ResponseContext* response, uint16_t idx);
bool cmd_resume_idx(ServerContext* ctx, ResponseContext* response, unsigned int idx);
bool cmd_pause_all(ServerContext* ctx, ResponseContext* response);
bool cmd_query_stats(ServerContext* ctx, ResponseContext* response);
bool cmd_status(ServerContext* ctx, ResponseContext* response);
bool handle_command(ServerContext* ctx, int client_fd, char* packet);
