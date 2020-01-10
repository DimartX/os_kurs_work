#include <iostream>
#include <string>
#include <map>
#include <thread>
#include <zmq.hpp>

#include "functions.hpp"

class Router {
public:
    Router(int port); 

    void Connector();
    void Handler();
    void Synchronizer();
    
    void add(const std::string& key, const int& value, std::string request); 
    void remove(const std::string& key, std::string request); 
    void get(const std::string& key, std::string request); 
    std::pair<bool, std::string> version(const std::string& request);

private:
    void InitializeStorages(int count);
    
private:
    int my_port;
    zmq::context_t context;
    zmq::socket_t connector;
    
    zmq::socket_t frontend_push;
    zmq::socket_t frontend_pull;

    zmq::socket_t up_update;
    std::map<std::string, zmq::socket_t> down_update;
    std::map<std::string, zmq::socket_t> backend_push;
    std::map<std::string, zmq::socket_t> backend_pull;
    std::map<std::string, int> storages_versions;
    
    int my_version;
    std::vector<std::pair<zmq::socket_t, int>> storages;
};

Router::Router(int port) :
    my_port(port),
    context(1),
    connector(context, ZMQ_REP),
    frontend_push(context, ZMQ_PUSH),
    frontend_pull(context, ZMQ_PULL),
    up_update(context, ZMQ_PULL), // приходят сообщения
    my_version(0)
{
    connector.bind(get_port_name(my_port+2));
    frontend_pull.bind(get_port_name(my_port));
    frontend_push.bind(get_port_name(my_port+1));
    up_update.bind(get_port_name(my_port+3));
}

void Router::Connector() {
    while (true) {
        std::istringstream is(recieve_message(connector));
        std::string command;
        is >> command;
        if (command == "connect"){
            std::string ask;
            is >> ask;
            //std::cout << "WHO asked " << ask << std::endl;
            backend_push[ask] = zmq::socket_t(context, ZMQ_PUSH);
            backend_pull[ask] = zmq::socket_t(context, ZMQ_PULL);
            down_update[ask] = zmq::socket_t(context, ZMQ_PUSH);
            storages_versions[ask] = 0;
            int push_port = bind_socket(backend_push[ask]);
            int pull_port = bind_socket(backend_pull[ask]);
            int down_port = bind_socket(down_update[ask]);
            int up_port = my_port + 3;
            
            std::string init = ask + " init " +
                std::to_string(push_port) + " " +
                std::to_string(pull_port) + " " +
                std::to_string(up_port) + " " +
                std::to_string(down_port) + " ";
            send_message(connector, init);
            std::cout << "Permit connection" << std::endl;
            version("version");
        }
        else if (command == "disconnect") {
            std::string ask;
            is >> ask;
            send_message(connector, "disconnected");
            backend_push.erase(ask);
            backend_pull.erase(ask);
            down_update.erase(ask);
        }
    }
}

void Router::Handler() {
    while (true) {
        //send_message(backend_push, "FUCK YOU");
        //std::cout << recieve_message(backend_router) << std::endl;
        std::string request = recieve_message(frontend_pull);
        if (backend_push.empty()) {
            send_message(frontend_push, "There is no storages");
            continue;
        }
        
        std::istringstream is(request);
        std::string command;
        is >> command;
        if (command == "add") {
            std::pair<std::string, int> pr;
            is >> pr.first >> pr.second;
            add(pr.first, pr.second, request);            
        }
        else if (command == "remove") {
            std::pair<std::string, int> pr;
            is >> pr.first;
            remove(pr.first, request);
        }
        else if (command == "get") {
            std::pair<std::string, int> pr;
            is >> pr.first;
            get(pr.first, request);
        }
        else if (command == "version") {
            std::pair<bool, std::string> vers = version(request);;
            if (vers.first)
                send_message(frontend_push, "Newest version, " + vers.second);
            else
                send_message(frontend_push, "Old version, " + vers.second);
        }
    }
}

void Router::Synchronizer() {
    while (true) {
        std::string request = recieve_message(up_update);
        std::istringstream is(request);
        std::string number;
        is >> number;
        std::string command;
        is >> command;
        if (command == "get_upd") {
            std::string key;
            for (auto& versions : storages_versions) {
                if (versions.second == my_version) {
                    key = versions.first;
                    break;
                }
            }
            std::string ask_for_update = number + " upload";
            send_message(down_update[key], ask_for_update);
        }
        else if (command == "updated") {
            int version;
            std::cin >> version;
            storages_versions[number] = version;
        }
        else if (command == "upload") {
            std::string reply = number + " download ";
            while (is) {
                std::string str;
                is >> str;
                reply += str + " ";
            }
            send_message(down_update[number], reply);
        }
    }
}


void Router::add(const std::string& key, const int& value, std::string request) {
    //std::cout << "add" << std::endl;
    std::string msg = std::to_string(my_version) + " " + request;
    
    for (auto& store : backend_push) {
        send_message(store.second, msg);
    }

    bool added = false;
    for (auto& store : backend_pull) {
        std::string ans = recieve_message(store.second);
        std::istringstream is(ans);
        char c = is.peek();
        if (c > '9' || c < '0')
            continue;
        int version;
        is >> version;
        if (version > my_version) {
            added = true;
            my_version = version;
        }
    }

    if (added) 
        send_message(frontend_push, "Ok.");        
    else 
        send_message(frontend_push, "Error: adding");
}

void Router::remove(const std::string& key, std::string request) {
    //std::cout << "remove" << std::endl;
    std::string msg = std::to_string(my_version) + " " + request;

    for (auto& store : backend_push) {
        send_message(store.second, msg);
    }
    
    bool removed = false;
    for (auto& store : backend_pull) {
        std::string ans = recieve_message(store.second);
        std::istringstream is(ans);
        char c = is.peek();
        if (c > '9' || c < '0')
            continue;
        int version;
        is >> version;
        if (version > my_version) {
            removed = true;
            my_version = version;
        }
    }

    if (removed) 
        send_message(frontend_push, "Ok.");        
    else 
        send_message(frontend_push, "Error: removing");
}

void Router::get(const std::string& key, std::string request) {
    //std::cout << "get" << std::endl;
    std::string msg = std::to_string(my_version) + " " + request;
    
    for (auto& store : backend_push) {
        send_message(store.second, msg);
    }

    bool got = false;
    std::string answer;
    for (auto& store : backend_pull) {
        std::string ans = recieve_message(store.second);
        std::istringstream is(ans);
        char c = is.peek();
        if (c > '9' || c < '0')
            continue;
        int version;
        is >> version;
        if (version == my_version) {
            got = true;
            is >> answer;
        }
    }

    if (got) 
        send_message(frontend_push, answer);        
    else 
        send_message(frontend_push, "Error: getting");
}

std::pair<bool, std::string> Router::version(const std::string& request) {
    //std::cout << "version" << std::endl;
    std::string msg = std::to_string(my_version) + " " + request;
    
    for (auto& store : backend_push) {
        send_message(store.second, msg);
    }

    //std::cout << "all sent " << std::endl;
    bool newest = false;
    int avail_version = 0;
    for (auto& store : backend_pull) {
        std::string ans = recieve_message(store.second);
        std::istringstream is(ans);
        char c = is.peek();
        if (c > '9' || c < '0')
            continue;
        int version;
        is >> version;
        if (version == my_version) {
            newest = true;
        }
        if (avail_version < version) {
            avail_version = version;
        }
    }
    
    return std::make_pair(newest, std::to_string(avail_version));
} 


int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <port>" << std::endl;
        return 1;
    }

    int port = std::stoi(argv[1]);

    Router router(port);

    std::thread handler(std::bind(&Router::Handler, &router));
    std::thread synchronizer(std::bind(&Router::Synchronizer, &router));
    std::thread connector(std::bind(&Router::Connector, &router));

    synchronizer.join();
    connector.join();
    handler.join();
    return 0;
}
