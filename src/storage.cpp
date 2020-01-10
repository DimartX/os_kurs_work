#include <iostream>
#include <string>
#include <map>
#include <thread>
#include <zmq.hpp>

#include "functions.hpp"

class Storage {
public:
    Storage() :
        context(1),
        connected(false),
        connector(context, ZMQ_REQ),
        store_pull(context, ZMQ_PULL),
        store_push(context, ZMQ_PUSH),
        update_push(context, ZMQ_PUSH),
        update_pull(context, ZMQ_PULL),
        my_version(0)
    { }

    void User() {
        std::cout << "connect/disconnect with router (connect <port> <keyword>)" << std::endl;
        while (true) {
            std::string command;
            std::cin >> command;
            if (command == "connect") {
                if (connected) {
                    std::cout << "Already connected to " << host_port << std::endl;
                    continue;
                }
                int port;
                std::string key;
                std::cin >> port >> key;
                connected = Connect(port, key);
            }
            else if (command == "disconnect") {
                if (!connected) {
                    std::cout << "Not connected" << std::endl;
                    continue;
                }
                std::string mes = "disconnect " + my_key;
                send_message(connector, mes);
                std::string request = recieve_message(connector);
                std::cout << request << std::endl;
                connected = false;
            }
            
        }
    }
    
    bool Connect(int port, std::string key) {
        host_port = port;
        my_key = key;
        
        connector.connect(get_port_name(port+2));
        //std::cout << "MY KEY " << key << std::endl;
        std::string init_mes = "connect " + key; 
        send_message(connector, init_mes);
        
        std::cout << "connecting..." << std::endl;
        std::string request = recieve_message(connector);
        //std::cout << "recieved " << request << std::endl;
        std::istringstream is(request);
        std::string number;
        is >> number;
        std::string command;
        is >> command;
        if (command == "init" && number == key) {
            int pull_port;
            int push_port;
            int up_port;
            int down_port;
            is >> pull_port >> push_port >> down_port >> up_port;
            store_pull.connect(get_port_name(pull_port));
            store_push.connect(get_port_name(push_port));
            update_pull.connect(get_port_name(down_port));
            update_push.connect(get_port_name(up_port));
            std::cout << "Succesfully connected" << std::endl;
            return true;
        }
        else {
            std::cout << "Error: missing key" << std::endl;
            return false;
        }
        return false;
    }
    
    void Handler() {
        while (true) {
            std::string request = recieve_message(store_pull);
            std::istringstream is(request);
            int version;
            is >> version;

            
            std::string command;
            is >> command;
            if (command == "add") {
                std::pair<std::string, int> pr;
                is >> pr.first >> pr.second;
                store[pr.first] = pr.second;
                my_version++;   
                
                std::string msg = std::to_string(my_version) + " Ok.";
                send_message(store_push, msg);
            }
            else if (command == "remove") {
                std::pair<std::string, int> pr;
                is >> pr.first;
                store.erase(pr.first);
                my_version++;
                
                std::string msg = std::to_string(my_version) + " Ok.";
                send_message(store_push, msg);
            }
            else if (command == "get") {
                std::pair<std::string, int> pr;
                is >> pr.first;
                
                std::string msg = std::to_string(my_version) + " " +
                    std::to_string(store[pr.first]) + " Ok.";
                send_message(store_push, msg);
                std::cout << "sent get" << std::endl;
            }
            else if (command == "version") {
                std::string reply = std::to_string(version) + " Ok.";
                send_message(store_push, reply);
            }

            if (version - my_version > 1) {
                std::string ask_for_update = my_key + " get_upd";
                send_message(update_push, ask_for_update);
                return;
            }
        }
    }

    void Synchronizer() {
        while (true) {
            std::string request = recieve_message(update_pull);
            std::istringstream is(request);
            std::string number;
            is >> number;
            std::string command;
            is >> command;
            if (command == "upload") {
                std::string reply = number + 
                    " upload " + std::to_string(my_version) + " ";
                for (auto& elem : store) {
                    reply += elem.first + " " +
                        std::to_string(elem.second) + " ";
                }
                send_message(update_push, reply);
            }
            else if (command == "download") {
                int version;
                is >> version;
                my_version = version;
                store.clear();
                while (is) {
                    std::string str;
                    int num;
                    is >> str >> num;
                    store[str] = num;
                }
                std::string reply = number +
                    " updated " + std::to_string(my_version);
                send_message(update_push, reply);
            }
        }
        
    }
    
private:
    int host_port;
    std::string my_key;
    zmq::context_t context;
    bool connected;
    zmq::socket_t connector;
    
    zmq::socket_t store_pull;
    zmq::socket_t store_push;
    zmq::socket_t update_push;
    zmq::socket_t update_pull;
    std::map<std::string, int> store;
    int my_version;
};


int main(int argc, char* argv[]) {
    Storage storage;

    std::thread handler(std::bind(&Storage::Handler, &storage));
    std::thread synchronizer(std::bind(&Storage::Synchronizer, &storage));
    std::thread user(std::bind(&Storage::User, &storage));
    
    handler.join();
    synchronizer.join();
    user.join();
    
    return 0;
}
