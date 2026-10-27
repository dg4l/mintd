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

bool Server::init() {
    const char* runtime_dir = getenv("XDG_RUNTIME_DIR");
    if (!runtime_dir || runtime_dir[0] != '/') {
        fprintf(stderr, "XDG_RUNTIME_DIR is not set\n");
        return false;
    }
    this->socket_path = std::string(runtime_dir) + "/mintd.socket";
    lt::settings_pack settings_pak;
    settings_pak.set_int(lt::settings_pack::alert_mask, lt::alert_category::status | lt::alert_category::error);
    settings_pak.set_str(lt::settings_pack::user_agent, "mintd/0.1");
    settings_pak.set_bool(lt::settings_pack::anonymous_mode, this->anonymous_mode);
    if (this->strict_bind) {
        settings_pak.set_str(lt::settings_pack::outgoing_interfaces, this->outgoing_interface);
        settings_pak.set_str(lt::settings_pack::listen_interfaces, this->incoming_interface);
    }
    if (DEBUG) {
        printf("anonymous mode: %s\n", this->anonymous_mode ? "on" : "off");
        printf("outgoing: %s\nlistening: %s\n", settings_pak.get_str(lt::settings_pack::outgoing_interfaces).c_str(),
                settings_pak.get_str(lt::settings_pack::listen_interfaces).c_str());
    }
    this->session = new lt::session(settings_pak);
    if (this->socket_path.size() >= sizeof(this->server_addr.sun_path)) {
        fprintf(stderr, "socket path too long\n");
        return false;
    }
    this->srv_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (this->srv_fd < 0) {
        perror("socket");
        return false;
    }
    memset(&this->server_addr, 0, sizeof(this->server_addr));
    this->server_addr.sun_family = AF_UNIX;
    strncpy(this->server_addr.sun_path, this->socket_path.c_str(), sizeof(this->server_addr.sun_path) - 1);
    unlink(this->socket_path.c_str());
    mode_t old_umask = umask(0177);
    int bind_result = bind(this->srv_fd, (struct sockaddr*)&this->server_addr, sizeof(this->server_addr));
    umask(old_umask);
    if (bind_result < 0) {
        perror("bind");
        return false;
    }
    if (listen(this->srv_fd, 1) < 0) {
        perror("listen");
        return false;
    }
    return true;
}

bool client_is_same_user(int client) {
    struct ucred cred;
    socklen_t cred_len = sizeof(cred);
    if (getsockopt(client, SOL_SOCKET, SO_PEERCRED, &cred, &cred_len) < 0) return false;
    return cred.uid == getuid();
}

void Server::handle_client(int client) {
    char packet[MAX_PACKET_SIZE + 1];
    struct timeval timeout = {1, 0};
    if (!client_is_same_user(client)) return;
    setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(client, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    ssize_t bytes = read(client, packet, MAX_PACKET_SIZE);
    if (bytes <= 0) return;
    packet[bytes] = 0;
    this->handle_packet(client, packet, bytes);
}

// don't return a bool, just modify the done flag.
void Server::handle_alerts() {
    std::vector<lt::alert*> alerts;
    this->session->pop_alerts(&alerts);
    for (lt::alert const* alert : alerts) {
        if (lt::alert_cast<lt::torrent_error_alert>(alert)) {
            this->done = true;
            continue;
        }
    }
}

std::string get_config_path() {
    const char* home = getenv("HOME");
    if (!home) return "";
    std::string path = home;
    path += "/.config/mintd/config.toml";
    return path;
}

const char* generate_default_config() {
    const char* default_config = "[network]\nstrict = false\nanonymous = false\n#replace with your own network interfaces.\noutgoing_interface = \"eth0\"\nincoming_interface = \"eth0\"\n[developer]\ndebug = false\n";
    return default_config;
}

bool create_default_config_file(std::string config_path) {
    std::string config_path_copy = config_path;
    fprintf(stderr, "config file not found!\n");
    std::filesystem::create_directories(std::filesystem::path(config_path).parent_path());
    const char* default_config_text = generate_default_config();
    FILE* config_file = fopen(config_path_copy.c_str(), "w");
    if (!config_file) {
        fprintf(stderr, "failed to open config file for writing\n");
        return false;
    }
    fputs(default_config_text, config_file);
    printf("config file written to %s!\n", config_path_copy.c_str());
    fclose(config_file);
    return true;
}

bool ensure_config_exists(std::string config_path) {
    if (config_path.empty()) return false;
    if (!std::filesystem::exists(config_path)) return create_default_config_file(config_path);
    return true;
}

// **maybe** consider factoring some stuff out, i think checking if the config file exists inline
// is the best way for now, but may not be in the future.
bool Server::parse_config_file() {
    std::string config_path = get_config_path();
    if (!ensure_config_exists(config_path)) return false;
    toml::table config = toml::parse_file(config_path);
    this->strict_bind = config["network"]["strict"].value_or(0);
    this->anonymous_mode = config["network"]["anonymous"].value_or(false);
    DEBUG = config["developer"]["debug"].value_or(0);
    if (DEBUG) std::cout << "debug enabled in config." << std::endl;
    if (this->strict_bind) {
        // we might be doing something redundant here.
        this->outgoing_interface = std::string(config["network"]["outgoing_interface"].value_or(""sv));
        this->incoming_interface = std::string(config["network"]["incoming_interface"].value_or(""sv));
        if (DEBUG) {
            std::cout << this->incoming_interface << std::endl;
            std::cout << this->outgoing_interface << std::endl;
        }
    }
    return true;
}

void dump_stack_trace(int sig) {
    void* array[10];
    size_t size;
    size = backtrace(array, 10);
    fprintf(stderr, "signal %d:\n", sig);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    signal(sig, SIG_DFL);
    raise(sig);
}

int main(int argc, char** argv) {
    int c;
    Server ctx;
    struct pollfd poll_fd;
    poll_fd.events = POLLIN;
    if (!ctx.parse_config_file()) return 1;
    // command line arguments should simply be overrides for the config file.
    while ((c = getopt(argc, argv, "da")) != -1) {
    //while ((c = getopt(argc, argv, "db:")) != -1) {
        switch(c) {
            case 'd':
                DEBUG = true;
                break;
            case 'a':
                ctx.anonymous_mode = true;
                break;
            //case 'b':
            //    break;
        }
    }
    if (!ctx.init()) return 1;
    std::vector<lt::torrent_handle> handles;
    poll_fd.fd = ctx.srv_fd;
    ctx.done = false;
    signal(SIGSEGV, dump_stack_trace);
    signal(SIGPIPE, SIG_IGN);
    while (!ctx.done) {
        int poll_result = poll(&poll_fd, 1, 200);
        if (poll_result < 0) fprintf(stderr, "poll error\n");
        if (poll_fd.revents & POLLIN) {
            int client = accept(ctx.srv_fd, NULL, NULL);
            if (client != -1) {
                ctx.handle_client(client);
                close(client);
            }
        }
        ctx.handle_alerts();
        if (ctx.done) continue;
    }
    delete ctx.session;
    return 0;
}
