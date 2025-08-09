//#include <fstream>
#include "cmd.hpp"
#include "common_include.hpp"
#define MAX_URL_LEN 8000

bool cmd_remove_torrent(ServerContext* ctx, ResponseContext* response, unsigned int idx){
    std::vector<lt::torrent_handle> handles = ctx->session->get_torrents();
    if (idx < handles.size()){
        ctx->session->remove_torrent(handles[idx]);
    }
    response->message = "REMOVED";
    return true;
}

// this is real bad.
bool parse_add_packet(char* packet, ParsedAddPacket* parsed_packet){
    uint16_t url_len = (uint8_t)packet[9] | ((uint8_t)packet[8] << 8);
    uint16_t save_path_len = 0;
    if (!url_len) return false;
    else if (url_len > MAX_PACKET_SIZE - 10) return false;
    parsed_packet->url = std::string(packet + 10, packet + 10 + url_len);
    if (url_len == MAX_PACKET_SIZE - 10){
        parsed_packet->save_path = ".";
    }
    else{
        if (12 + url_len > MAX_PACKET_SIZE) return false;
        save_path_len = (uint8_t)packet[11 + url_len] | (uint8_t)(packet[10 + url_len]) << 8;
        if (!save_path_len){
            parsed_packet->save_path = ".";
        }
        else{
            if (12 + url_len + save_path_len > MAX_PACKET_SIZE) return false;
            parsed_packet->save_path = std::string(packet + 12 + url_len, packet + 12 + url_len + save_path_len);
        }
    }
    return true;
}

// parse packet inline for now. if i feel like it,
// maybe factor out and pass a struct returned from the parser into this function
// rather than doing parsing inline
bool cmd_add_torrent(ServerContext* ctx, ResponseContext* response, char* packet){
    ParsedAddPacket parsed_packet;
    if (!parse_add_packet(packet, &parsed_packet)){
        response->message = "INVALID PACKET";
        return false;
    }
    lt::add_torrent_params atp = lt::parse_magnet_uri(parsed_packet.url);
    atp.save_path = parsed_packet.save_path;
    ctx->session->add_torrent(std::move(atp));
    response->message = "ADDED";
    return true;
}

bool cmd_resume_all(ServerContext* ctx, ResponseContext* response){
    std::vector<lt::torrent_handle> handles = ctx->session->get_torrents();
    for (size_t i = 0; i < handles.size(); ++i){
        handles[i].resume();
    }
    response->message = "resumed";
    return true;
}

bool cmd_pause_idx(ServerContext* ctx, ResponseContext* response, uint16_t idx){
    std::vector<lt::torrent_handle> handles = ctx->session->get_torrents();
    if (idx < handles.size()){
        handles[idx].pause();
    }
    response->message = "PAUSED";
    return true;
}

bool cmd_resume_idx(ServerContext* ctx, ResponseContext* response, unsigned int idx){
    std::vector<lt::torrent_handle> handles = ctx->session->get_torrents();
    if (idx < handles.size()){
        handles[idx].resume();
    }
    response->message = "RESUMED";
    return true;
}

bool cmd_invalid(ServerContext* ctx, ResponseContext* response){
    response->message = "ERROR: INVALID COMMAND";
    return false;
}

bool cmd_pause_all(ServerContext* ctx, ResponseContext* response){
    std::vector<lt::torrent_handle> handles = ctx->session->get_torrents();
    for (size_t i = 0; i < handles.size(); ++i){
        handles[i].pause();
    }
    response->message = "PAUSED ALL";
    return true;
}

bool cmd_status(ServerContext* ctx, ResponseContext* response){
    std::vector<lt::torrent_handle> handles = ctx->session->get_torrents();
    response->message = "";
    std::size_t handle_cnt = handles.size();
    if (!handle_cnt){
        response->message += "no torrents added";
    }
    for (std::size_t i = 0; i < handle_cnt; ++i){
        lt::torrent_status status = handles[i].status();
        response->message += "[";
        response->message += std::to_string(i);
        response->message += "]: ";
        response->message += craft_torrent_status_string(status);
        if (i < handle_cnt - 1){
            response->message += '\n';
        }
    }
    return true;
}

// todo: implement
bool cmd_query_stats_alltime(ServerContext* ctx){
    return true;
}

// for current session
bool cmd_query_stats(ServerContext* ctx, ResponseContext* response){
    std::vector<lt::torrent_handle> handles = ctx->session->get_torrents();
    std::size_t handle_cnt = handles.size();
    std::size_t total_upload = 0;
    std::size_t total_download = 0;
    response->message = "";
    for (std::size_t i = 0; i < handle_cnt; ++i){
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

bool handle_command(ServerContext* ctx, int client_fd, char* packet){
    ResponseContext response;
    response.message = "default";
    bool ret = false;
    uint16_t command = packet[3] | (packet[2] << 8);
    switch(command){
        case CMD_ADD:
            ret = cmd_add_torrent(ctx, &response, packet);
            break;
        case CMD_QUERY_STATUS:
            ret = cmd_status(ctx, &response);
            break;
        case CMD_PAUSE_ALL:
            ret = cmd_pause_all(ctx, &response);
            break;
        case CMD_RESUME_ALL:
            ret = cmd_resume_all(ctx, &response);
            break;
        case CMD_PAUSE_IDX:{
            uint32_t idx = extract_idx_from_packet(packet);
            ret = cmd_pause_idx(ctx, &response, idx);
            break;
        }
        case CMD_RESUME_IDX:{
            uint32_t idx = extract_idx_from_packet(packet);
            ret = cmd_resume_idx(ctx, &response, idx);
            break;
        }
        case CMD_REMOVE_TORRENT:{
            uint32_t idx = extract_idx_from_packet(packet);
            ret = cmd_remove_torrent(ctx, &response, idx);
            break;
        }
        case CMD_QUERY_STATS:
            ret = cmd_query_stats(ctx, &response);
            break;
        default:
            ret = cmd_invalid(ctx, &response);
            break;
    }
    send(client_fd, response.message.c_str(), strlen(response.message.c_str()), 0);
    return ret;
}
