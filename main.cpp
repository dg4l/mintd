#include <iostream>
#include <thread>
#include <chrono>
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdint.h>
#include "common_include.hpp"
#include "cmd.hpp"
#include "util.hpp"
#include "types.hpp"
#include <libtorrent/session.hpp>
#include <libtorrent/session_params.hpp>
#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/magnet_uri.hpp>

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

void handle_client(ServerContext* ctx, int client){
    char packet[10000];
    int bytes = read(client, packet, sizeof(packet));
    packet[bytes] = 0;
    handle_packet(ctx, client, packet);
    close(client);
}

// don't return a bool, just modify the done flag.
void handle_alerts(ServerContext* ctx){
    std::vector<lt::alert*> alerts;
    ctx->session->pop_alerts(&alerts);
    for (lt::alert const* alert : alerts){
        if (lt::alert_cast<lt::torrent_error_alert>(alert)){
            ctx->done = true;
            continue;
        }
    }
}

int main(int argc, char** argv){
    ServerContext ctx;
    struct pollfd poll_fd;
    poll_fd.events = POLLIN;
    init_mintd(&ctx);
    std::vector<lt::torrent_handle> handles;
    poll_fd.fd = ctx.srv_fd;
    listen(ctx.srv_fd, 1);
    ctx.done = false;
    while (!ctx.done){
        int poll_result = poll(&poll_fd, 1, 200);
        if (poll_result < 0){
            fprintf(stderr, "poll error\n");
        }
        if (poll_fd.revents & POLLIN){
            int client = accept(ctx.srv_fd, NULL, NULL);
            if (client != -1){
                handle_client(&ctx, client);
                close(client);
            }
        }
        handle_alerts(&ctx);
        if (ctx.done){
            continue;
        }
    }
}
