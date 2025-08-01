#include <iostream>
#include <thread>
#include <chrono>
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
