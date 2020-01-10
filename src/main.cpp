#include <iostream>
#include <string>

#include <zmq.hpp>
#include "functions.hpp"

struct StorageAPI {
    StorageAPI(int port) :
        context(1),
        push_socket(context, ZMQ_PUSH),
        pull_socket(context, ZMQ_PULL)
    {
        push_socket.connect(get_port_name(port));
        pull_socket.connect(get_port_name(port+1));
    }

    void add(std::pair<std::string, int> pr) {
        std::string msg = "add " + pr.first + " " + std::to_string(pr.second);
        if (!send_message(push_socket, msg)) {
            std::cout << "Error sending for adding" << std::endl;
            return;
        }
        std::string ans = recieve_message(pull_socket);
        std::cout << ans << std::endl;
    }

    void remove(std::pair<std::string, int> pr) {
        std::string msg = "remove " + pr.first;
        if (!send_message(push_socket, msg)) {
            std::cout << "Error sending for removing" << std::endl;
            return;
        }
        std::string ans = recieve_message(pull_socket);
        std::cout << ans << std::endl;
    }

    void get(std::pair<std::string, int> pr) {
        std::string msg = "get " + pr.first;
        if (!send_message(push_socket, msg)) {
            std::cout << "Error sending for getting" << std::endl;
            return;
        }
        std::string ans = recieve_message(pull_socket);
        std::cout << ans << std::endl;
    }

    void version() {
        std::string msg = "version";
        if (!send_message(push_socket, msg)) {
            std::cout << "Error sending for version" << std::endl;
            return;
        }
        std::cout << "recieving" << std::endl;
        std::string ans = recieve_message(pull_socket);
        std::cout << "RECIEVED " ;
        std::cout << ans << std::endl;
    }
    
private:
    zmq::context_t context;
    zmq::socket_t push_socket;
    zmq::socket_t pull_socket;
};

int main() {
    int port;
    std::cout << "Enter port to connect: ";
    std::cin >> port;
    StorageAPI store(port);

    while (true) {
        std::cout << "-> ";
        std::string command;
        std::cin >> command;
        if (command == "add") {
            std::pair<std::string, int> pr;
            std::cin >> pr.first >> pr.second;
            store.add(pr);
        }
        else if (command == "remove") {
            std::pair<std::string, int> pr;
            std::cin >> pr.first;
            store.remove(pr);
        }
        else if (command == "get") {
            std::pair<std::string, int> pr;
            std::cin >> pr.first;
            store.get(pr);
        }
        else if (command == "version") {
            store.version();
        }
        else {
            std::cout << "Wrong command" << std::endl;
        }
    }
    return 0;
}
