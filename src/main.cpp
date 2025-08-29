#include <iostream>
#include <thread>
#include <chrono>
#include <poll.h>
#include <filesystem>
#include <signal.h>
#include <execinfo.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdint.h>
#include <libgen.h>
#include <ifaddrs.h>
#include <libtorrent/session.hpp>
#include <libtorrent/session_params.hpp>
#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <toml++/toml.hpp>
#include "common_include.hpp"
#include "cmd.hpp"
#include "util.hpp"
#include "types.hpp"
#include "global.hpp"
using namespace std::literals;

bool DEBUG = false;

std::string ip_from_interface_name(const char* interface_name){
    std::string ret = ""; 
    struct ifaddrs* ifaddr;
    int failed;
    char ip[NI_MAXHOST];
    if (getifaddrs(&ifaddr) == -1){
        return ret;
    }
    struct ifaddrs* ifa;
    for (ifa = ifaddr; ifa; ifa = ifa->ifa_next){
        if (!ifa->ifa_addr){
            continue;
        }
        failed = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), ip, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
        if (!strcmp(ifa->ifa_name, interface_name) && ifa->ifa_addr->sa_family == AF_INET){
            if (failed){
                break;
            }
            ret = ip;
            break;
        }
    }
    freeifaddrs(ifaddr);
    return ret;
}

bool init_mintd(ServerContext* ctx){
    ctx->socket_path = "/tmp/mintd.socket";
    lt::settings_pack settings_pak;
    settings_pak.set_int(lt::settings_pack::alert_mask, lt::alert_category::status | lt::alert_category::error);
    settings_pak.set_str(lt::settings_pack::user_agent, "mintd/0.1");
    if (ctx->strict_bind){
        // might not be null terminated, investigate.
        std::string incoming_interface = ip_from_interface_name(ctx->incoming_interface.c_str());
        std::string outgoing_interface = ip_from_interface_name(ctx->outgoing_interface.c_str());
        if (incoming_interface.empty() || outgoing_interface.empty()){
            fprintf(stderr, "failed getting interface ip\n");
            return false;
        }
    }
    ctx->session = new lt::session(settings_pak);
    ctx->srv_fd = socket(AF_UNIX, SOCK_STREAM, 0); 
    memset(&ctx->server_addr, 0, sizeof(ctx->server_addr));
    ctx->server_addr.sun_family = AF_UNIX;
    strcpy(ctx->server_addr.sun_path, ctx->socket_path);
    unlink(ctx->socket_path);
    bind(ctx->srv_fd, (struct sockaddr*)&ctx->server_addr, sizeof(ctx->server_addr));
    return true;
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

std::string get_config_path(){
    std::string path = getenv("HOME");
    path += "/.config/mintd/config.toml";
    return path;
}

const char* generate_default_config(){
    const char* default_config = "[network]\nstrict = false\noutgoing_interface = \"eth0\"\nincoming_interface = \"eth0\"\n[developer]\ndebug = false\n";
    return default_config;
}

bool create_default_config_file(std::string config_path){
    std::string config_path_copy = config_path;
    fprintf(stderr, "config file not found!\n");
    std::string dir_path = dirname((char*)config_path.c_str());
    std::filesystem::create_directories(dir_path);
    const char* default_config_text = generate_default_config();
    FILE* config_file = fopen(config_path_copy.c_str(), "w");
    if (!config_file){
        fprintf(stderr, "failed to open config file for writing\n");
        return false;
    }
    fprintf(config_file, default_config_text);
    printf("config file written to %s!\n", config_path_copy.c_str());
    fclose(config_file);
    return true;
}

// **maybe** consider factoring some stuff out, i think checking if the config file exists inline
// is the best way for now, but may not be in the future.
bool parse_config_file(ServerContext* ctx){
    std::string config_path = get_config_path();
    if (!config_path.empty()){
        if (!std::filesystem::exists(config_path)){
            if (!create_default_config_file(config_path)){
                return false;
            }
        }
    }
    toml::table config = toml::parse_file(config_path);
    ctx->strict_bind = config["network"]["strict"].value_or(0);
    DEBUG = config["developer"]["debug"].value_or(0);
    if (DEBUG){
        std::cout << "debug enabled in config." << std::endl;
    }
    if (ctx->strict_bind){
        // we might be doing something redundant here.
        ctx->outgoing_interface = std::string(config["network"]["outgoing_interface"].value_or(""sv));
        ctx->incoming_interface = std::string(config["network"]["incoming_interface"].value_or(""sv));
        if (DEBUG){
            std::cout << ctx->incoming_interface << std::endl;
            std::cout << ctx->outgoing_interface << std::endl;
        }
    }
    return true;
}

void dump_stack_trace(int sig){
    void* array[10];
    size_t size;
    size = backtrace(array, 10);
    fprintf(stderr, "signal %d:\n", sig);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    *(int*)0 = 0;
}

int main(int argc, char** argv){
    int c;
    ServerContext ctx;
    struct pollfd poll_fd;
    poll_fd.events = POLLIN;
    if (!parse_config_file(&ctx)){
        return 1;
    }
    // command line arguments should simply be overrides for the config file.
    while ((c = getopt(argc, argv, "d")) != -1){
    //while ((c = getopt(argc, argv, "db:")) != -1){
        switch(c){
            case 'd':
                DEBUG = true;
                break;
            //case 'b':
            //    break;
        }
    }
    if (!init_mintd(&ctx)){
        return 1;
    }
    std::vector<lt::torrent_handle> handles;
    poll_fd.fd = ctx.srv_fd;
    listen(ctx.srv_fd, 1);
    ctx.done = false;
    signal(SIGSEGV, dump_stack_trace);
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
    delete ctx.session;
    return 0;
}
