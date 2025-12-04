//#include <fstream>
#include <iostream>
#include <string>
#include "cmd.hpp"
#include "global.hpp"
#include "common_include.hpp"
#define MAX_URL_LEN 8000


// TODO:
// in the future we will likely serialize responses into something like json,
// and it will be the client's job to deserialize and make it human readable.

bool cmd_remove_torrent(ServerContext* ctx, ResponseContext* response, unsigned int idx) {
    if (DEBUG) {
        printf("issued remove torrent command on idx %u\n", idx);
    }
    std::vector<lt::torrent_handle> handles = ctx->session->get_torrents();
    if (idx >= handles.size()) {
        response->message = "ERROR: invalid index";
        return false;
    }
    ctx->session->remove_torrent(handles[idx]);
    response->message = "removed";
    return true;
}

// this is real bad.
// likely rewrite the entire function.
#define URL_OFFSET 10
bool parse_add_packet(char* packet, ParsedAddPacket* parsed_packet) {
    uint16_t url_len = (uint8_t)packet[9] | ((uint8_t)packet[8] << 8);
    uint16_t save_path_len = 0;
    if (!url_len || url_len > MAX_PACKET_SIZE - 10) return false;
    if (12 + url_len > MAX_PACKET_SIZE) return false;
    parsed_packet->url = std::string(packet + URL_OFFSET, packet + URL_OFFSET + url_len);
    if (url_len == MAX_PACKET_SIZE - URL_OFFSET) {
        parsed_packet->save_path = ".";
    }
    else {
        save_path_len = (uint8_t)packet[url_len + URL_OFFSET + 1] | (uint8_t)(packet[url_len + URL_OFFSET]) << 8;
        if (!save_path_len) {
            parsed_packet->save_path = ".";
        }
        else {
            if (save_path_len > MAX_PACKET_SIZE - url_len - 12) return false;
            parsed_packet->save_path = std::string(packet + 12 + url_len, packet + 12 + url_len + save_path_len);
        }
    }
    return true;
}

// parse packet inline for now. if i feel like it,
// maybe factor out and pass a struct returned from the parser into this function
// rather than doing parsing inline
bool cmd_add_torrent(ServerContext* ctx, ResponseContext* response, char* packet) {
    if (DEBUG) {
        printf("issued cmd add torrent\n");
    }
    ParsedAddPacket parsed_packet;
    lt::add_torrent_params atp;
    if (!parse_add_packet(packet, &parsed_packet)) {
        response->message = "INVALID PACKET";
        return false;
    }
    if (DEBUG) {
        printf("torrent parsed, magnet link is: %s\n", parsed_packet.url.c_str());
    }
    try {
        atp = lt::parse_magnet_uri(parsed_packet.url);
    }
    catch (boost::system::system_error& err) {
        response->message = "INVALID URL";
        return false;
    }
    atp.save_path = parsed_packet.save_path;
    ctx->session->add_torrent(std::move(atp));
    response->message = "ADDED";
    return true;
}

bool cmd_resume_all(ServerContext* ctx, ResponseContext* response) {
    if (DEBUG) {
        printf("issued resume all cmd\n");
    }
    std::vector<lt::torrent_handle> handles = ctx->session->get_torrents();
    for (size_t i = 0; i < handles.size(); ++i) {
        handles[i].resume();
    }
    response->message = "resumed";
    return true;
}

bool cmd_pause_idx(ServerContext* ctx, ResponseContext* response, uint16_t idx) {
    if (DEBUG) {
        printf("pause idx cmd, %u\n", idx);
    }
    std::vector<lt::torrent_handle> handles = ctx->session->get_torrents();
    if (idx < handles.size()) {
        handles[idx].pause();
    }
    response->message = "PAUSED";
    return true;
}

bool cmd_resume_idx(ServerContext* ctx, ResponseContext* response, unsigned int idx) {
    if (DEBUG) {
        printf("resume idx cmd, %u\n", idx);
    }
    std::vector<lt::torrent_handle> handles = ctx->session->get_torrents();
    if (idx < handles.size()) {
        handles[idx].resume();
        response->message = "RESUMED";
        return true;
    }
    response->message = "ERROR: invalid idx";
    return false;
}

bool cmd_invalid(ServerContext* ctx, ResponseContext* response) {
    response->message = "ERROR: INVALID COMMAND";
    return false;
}

bool cmd_pause_all(ServerContext* ctx, ResponseContext* response) {
    if (DEBUG) {
        printf("pause all cmd\n");
    }
    std::vector<lt::torrent_handle> handles = ctx->session->get_torrents();
    if (handles.empty()) {
        response->message = "ERROR: NO TORRENTS TO PAUSE!";
        return false;
    }
    for (size_t i = 0; i < handles.size(); ++i) {
        handles[i].pause();
    }
    response->message = "PAUSED ALL";
    return true;
}

bool cmd_query_torrent_status(ServerContext* ctx, ResponseContext* response, unsigned int idx) {
    std::vector<lt::torrent_handle> handles = ctx->session->get_torrents();
    if (idx < handles.size()) {
        lt::torrent_status status = handles[idx].status();
        response->message = craft_torrent_status_string_extra(status);
        return true;
    }
    response->message = "torrent does not exist";
    return false;
}

bool cmd_status(ServerContext* ctx, ResponseContext* response) {
    printf("issued status command\n");
    std::vector<lt::torrent_handle> handles = ctx->session->get_torrents();
    response->message = "";
    std::size_t handle_cnt = handles.size();
    if (!handle_cnt) {
        response->message += "no torrents added";
    }
    for (std::size_t i = 0; i < handle_cnt; ++i) {
        lt::torrent_status status = handles[i].status();
        response->message += "[";
        response->message += std::to_string(i);
        response->message += "]: ";
        response->message += craft_torrent_status_string_standard(status);
        if (i < handle_cnt - 1) {
            response->message += '\n';
        }
    }
    return true;
}

// for current session
bool cmd_query_stats(ServerContext* ctx, ResponseContext* response) {
    if (DEBUG) {
        printf("issued query stats command\n");
    }
    std::vector<lt::torrent_handle> handles = ctx->session->get_torrents();
    std::size_t handle_cnt = handles.size();
    std::size_t total_upload = 0;
    std::size_t total_download = 0;
    response->message = "";
    for (std::size_t i = 0; i < handle_cnt; ++i) {
        lt::torrent_status status = handles[i].status();
        total_upload += status.total_upload;
        total_download += status.total_download;
    }
    response->message += "Total Upload: ";
    response->message += std::to_string(total_upload);
    response->message += "\n";
    response->message += "Total Download: ";
    response->message += std::to_string(total_download);
    return true;
}

// TODO: handle errors in a better way.
bool handle_command(ServerContext* ctx, int client_fd, char* packet) {
    ResponseContext response;
    response.message = "default";
    bool success = false;
    uint16_t command = packet[3] | (packet[2] << 8);
    switch(command) {
        case CMD_ADD:
            success = cmd_add_torrent(ctx, &response, packet);
            break;
        case CMD_QUERY_STATUS:
            success = cmd_status(ctx, &response);
            break;
        case CMD_PAUSE_ALL:
            success = cmd_pause_all(ctx, &response);
            break;
        case CMD_RESUME_ALL:
            success = cmd_resume_all(ctx, &response);
            break;
        case CMD_PAUSE_IDX: {
            uint32_t idx = extract_idx_from_packet(packet);
            success = cmd_pause_idx(ctx, &response, idx);
            break;
        }
        case CMD_RESUME_IDX: {
            uint32_t idx = extract_idx_from_packet(packet);
            success = cmd_resume_idx(ctx, &response, idx);
            break;
        }
        case CMD_REMOVE_TORRENT: {
            uint32_t idx = extract_idx_from_packet(packet);
            success = cmd_remove_torrent(ctx, &response, idx);
            break;
        }
        case CMD_QUERY_TORRENT_STATUS: {
            uint32_t idx = extract_idx_from_packet(packet);
            success = cmd_query_torrent_status(ctx, &response, idx);
            break;
        }
        case CMD_QUERY_STATS:
            success = cmd_query_stats(ctx, &response);
            break;
        default:
            success = cmd_invalid(ctx, &response);
            break;
    }
    send(client_fd, response.message.c_str(), strlen(response.message.c_str()), 0);
    return success;
}
