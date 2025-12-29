#include <format>
#include "util.hpp"
#include "global.hpp"

// this function should give more extensive information compared to craft_torrent_status_string_standard 
// also consider changing from taking in a torrent_status to taking in a torrent_handle.
// also should give a progress bar (because it looks nice)
std::string craft_torrent_status_string_extra(lt::torrent_status& status) {
    float percent_done = status.progress * 100;
    std::string status_str = std::format("[NAME]: {}\n[DONE]: {}%\n[DOWN]: {} B/S\n[UP]: {} B/S\n[PEER COUNT]: {}\n[SEED COUNT]: {}",
            status.name, percent_done, status.download_rate, status.upload_rate, status.num_peers, status.num_seeds);
    return status_str;
}

// consider changing from taking in a torrent_status to taking in a torrent_handle.
std::string craft_torrent_status_string_standard(lt::torrent_status& status) {
    float percent_done = status.progress * 100;
    std::string status_str = std::format("[NAME: {}] [DONE: {}%] [STATE: {}] [SEEDING: {}] [PATH: {}]",
            status.name, percent_done, status.flags & lt::torrent_flags::paused ? " PAUSED" : " UNPAUSED",
            status.is_seeding ? "YES" : "NO", status.save_path);
    return status_str;
}

bool packet_has_magic(char* packet) {
    return (packet[0] == 'M' && packet[1] == 'T');
}

// with -O2 on x86, this simply compiles to a single bswap instruction 
uint32_t extract_idx_from_packet(char* packet) {
    uint32_t idx = ((uint8_t)packet[4] << 24) | ((uint8_t)packet[5] << 16) | ((uint8_t)packet[6] << 8) | (uint8_t)packet[7];
    return idx;
}

void handle_packet(ServerContext* ctx, int client_fd, char* packet) {
    const char* invalid_msg = "packet does not contain magic!";
    if (!packet_has_magic(packet)) {
        send(client_fd, invalid_msg, strlen(invalid_msg), 0);
        return;
    }
    if (DEBUG) {
        std::cout << "packet contains magic MT" << std::endl;
    }
    if (!handle_command(ctx, client_fd, packet)) {
        fprintf(stderr, "error while handling command\n"); 
    }
}

