#include "util.hpp"

// todo: make this function less ugly.
std::string craft_torrent_status_string(lt::torrent_status& status){
    std::string status_str;
    float percent_done = status.progress * 100;
    // appending is a little messy but it's fine for now
    status_str += "[NAME: ";
    status_str += status.name;
    status_str += "] [DONE: ";
    status_str += std::to_string(percent_done);
    status_str += "%] [STATE:";
    status_str += (status.flags & lt::torrent_flags::paused) ? " PAUSED" : " UNPAUSED";
    if (status.is_seeding){
        status_str += " SEEDING";
    }
    status_str += "]";
    status_str += " [PATH: \"";
    status_str += status.save_path;
    status_str += "\"]";
    return status_str;
}

bool packet_has_magic(char* packet){
    return (packet[0] == 'M' && packet[1] == 'T');
}

uint32_t extract_idx_from_packet(char* packet){
    uint32_t idx = ((uint8_t)packet[4] << 24) | ((uint8_t)packet[5] << 16) | ((uint8_t)packet[6] << 8) | (uint8_t)packet[7];
    return idx;
}

void handle_packet(ServerContext* ctx, int client_fd, char* packet){
    const char* invalid_msg = "packet is invalid!";
    if (!packet_has_magic(packet)){
        send(client_fd, invalid_msg, strlen(invalid_msg), 0);
        return;
    }
    if (DEBUG){
        std::cout << "pkt ok!" << std::endl;
    }
    if (!handle_command(ctx, client_fd, packet)){
        fprintf(stderr, "error while handling command\n"); 
    }
}

