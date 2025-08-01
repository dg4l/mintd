#include <iostream>
#include <thread>
#include <chrono>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdint.h>
#include <libtorrent/session.hpp>
#include <libtorrent/session_params.hpp>
#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/magnet_uri.hpp>
#define DEBUG 0

enum CMDS{
    CMD_ADD,
    CMD_QUERY_STATUS,
    CMD_PAUSE_ALL,
    CMD_PAUSE_IDX,
    CMD_RESUME_ALL,
    CMD_RESUME_IDX
};

typedef struct ServerContext{
    const char* socket_path;
    struct sockaddr_un server_addr;
    lt::settings_pack settings;
    lt::session* session;
    int srv_fd;
}ServerContext;

typedef struct ResponseContext{
    std::string message;
}ResponseContext;

bool cmd_add_torrent(ServerContext* ctx, ResponseContext* response, char* URL){
    lt::add_torrent_params atp = lt::parse_magnet_uri(URL);
    atp.save_path = ".";
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

bool cmd_pause_all(ServerContext* ctx, ResponseContext* response){
    std::vector<lt::torrent_handle> handles = ctx->session->get_torrents();
    for (size_t i = 0; i < handles.size(); ++i){
        handles[i].pause();
    }
    response->message = "PAUSED ALL";
    return true;
}

std::string craft_torrent_status_string(lt::torrent_status& status){
    std::string status_str;
    float percent_done = status.progress * 100;
    status_str += status.name;
    status_str += " ";
    status_str += std::to_string(percent_done);
    status_str += "%";
    if (status.flags & lt::torrent_flags::paused){
        status_str += " PAUSED";
    }
    else{
        status_str += " UNPAUSED";
    }
    if (status.is_seeding){
        status_str += " SEEDING";
    }
    return status_str;
}

bool cmd_status(ServerContext* ctx, ResponseContext* response){
    std::vector<lt::torrent_handle> handles = ctx->session->get_torrents();
    response->message = "";
    std::size_t handle_cnt = handles.size();
    if (!handle_cnt){
        response->message += "no torrents added";
    }
    // appending is a little messy but it's fine for now
    for (std::size_t i = 0; i < handle_cnt; ++i){
        lt::torrent_status status = handles[i].status();
        response->message += std::to_string(i);
        response->message += ": ";
        response->message += craft_torrent_status_string(status);
        if (i < handles.size() - 1){
            response->message += '\n';
        }
    }
    return true;
}

void init_mintd(ServerContext* ctx){
    ctx->socket_path = "/tmp/mintd.socket";
    lt::settings_pack settings_pak;
    settings_pak.set_int(lt::settings_pack::alert_mask, lt::alert_category::status | lt::alert_category::error);
    settings_pak.set_str(lt::settings_pack::user_agent, "mintd/0.1");
    ctx->session = new lt::session(settings_pak);
    ctx->srv_fd = socket(AF_UNIX, SOCK_STREAM, 0); 
    memset(&ctx->server_addr, 0, sizeof(ctx->server_addr));
    ctx->server_addr.sun_family = AF_UNIX;
    strcpy(ctx->server_addr.sun_path, ctx->socket_path);
    unlink(ctx->socket_path);
    bind(ctx->srv_fd, (struct sockaddr*)&ctx->server_addr, sizeof(ctx->server_addr));
}

bool packet_has_magic(char* packet){
    return (packet[0] == 'M' && packet[1] == 'T');
}

uint32_t extract_idx_from_packet(char* packet){
    uint32_t idx = ((uint8_t)packet[4] << 24) | ((uint8_t)packet[5] << 16) | ((uint8_t)packet[6] << 8) | (uint8_t)packet[7];
    return idx;
}

bool handle_command(ServerContext* ctx, int client_fd, char* packet){
    ResponseContext response;
    response.message = "default";
    bool ret;
    uint16_t command = packet[3] | (packet[2] << 8);
    switch(command){
        case CMD_ADD:
            ret = cmd_add_torrent(ctx, &response, packet + 8);
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
        default:
            ret = false;
    }
    send(client_fd, response.message.c_str(), strlen(response.message.c_str()), 0);
    return ret;
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

int main(int argc, char** argv){
    ServerContext ctx;
    init_mintd(&ctx);
    std::vector<lt::torrent_handle> handles;
    listen(ctx.srv_fd, 1);
    char packet[10000];
    bool done = false;
    std::vector<lt::alert*> alerts;
    while (!done){
        int client = accept(ctx.srv_fd, NULL, NULL);
        if (client != -1){
            int bytes = read(client, packet, sizeof(packet));
            packet[bytes] = 0;
            handle_packet(&ctx, client, packet);
            close(client);
            ctx.session->pop_alerts(&alerts);
            for (lt::alert const* alert : alerts){
                if (lt::alert_cast<lt::torrent_error_alert>(alert)){
                    done = true;
                    continue;
                }
            }
        }
    }
}
