#pragma once

#include <zmq.hpp>
#include <string>

#define TIMEOUT 200

bool send_message(zmq::socket_t& socket, const std::string& message_string) {
    zmq::message_t message(message_string.size());
    memcpy(message.data(), message_string.c_str(), message_string.size());
    bool ok = false;
    try {
        ok = socket.send(message);
    }catch(...) {ok = false;}
    return ok;
}

std::string msg_to_string(zmq::message_t& message) {
    return std::string(static_cast<char*>(message.data()), message.size());
}

std::string recieve_message(zmq::socket_t& socket) {
    zmq::message_t message;
    bool ok;
    try {
        ok = socket.recv(&message);
    } catch (...) {
        ok = false;
    }
    std::string recieved_message(static_cast<char*>(message.data()), message.size());
    if (recieved_message.empty() || !ok) {
        return "Error: Worker is not available";
    }
    return recieved_message;
}

std::string get_port_name(int port) {
    return "tcp://127.0.0.1:" + std::to_string(port);
}

int bind_socket(zmq::socket_t& socket) {
    int port = 30000;
    while (true) {
        try {
            socket.bind(get_port_name(port));
            break;
        } catch(...) {
            port++;
        }
    }
    int linger = 0;
    socket.setsockopt(ZMQ_RCVTIMEO, TIMEOUT);
    socket.setsockopt(ZMQ_SNDTIMEO, TIMEOUT);
    socket.setsockopt(ZMQ_LINGER, &linger, sizeof(linger));
    return port;
}
