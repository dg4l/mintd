#pragma once
#include <iostream>
#include "common_include.hpp"
#include "types.hpp"
#include "cmd.hpp"

std::string craft_torrent_status_string_standard(lt::torrent_status& status);
std::string craft_torrent_status_string_extra(lt::torrent_status& status);
bool packet_has_magic(char* packet);
uint32_t extract_idx_from_packet(char* packet);
void handle_packet(ServerContext* ctx, int client_fd, char* packet);
