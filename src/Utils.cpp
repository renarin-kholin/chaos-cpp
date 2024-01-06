//
// Created by shishu on 1/6/24.
//
#include "Utils.h"

std::string get_random_id(const size_t length) {
    using std::chrono::high_resolution_clock;
    static thread_local std::mt19937 rng(
        static_cast<unsigned int>(high_resolution_clock::now().time_since_epoch().count()));
    static const std::string characters(
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
    std::string id(length, '0');
    std::uniform_int_distribution<int> uniform(0, int(characters.size() - 1));
    std::generate(id.begin(), id.end(), [&]() { return characters.at(uniform(rng)); });
    return id;
}

shared_ptr<rtc::PeerConnection> create_peer_connection(const rtc::Configuration& rtc_config,
                                                       weak_ptr<rtc::WebSocket> weak_websocket, std::string client_id) {
    Chaos* chaos = Chaos::get_instance();

    auto t_peer_connection = std::make_shared<rtc::PeerConnection>(rtc_config);
    t_peer_connection->onStateChange([](rtc::PeerConnection::State const connection_state) {
        std::cout << "Peer Connection State: " << connection_state << std::endl;
    });
    t_peer_connection->onGatheringStateChange([](rtc::PeerConnection::GatheringState const connection_gathering_state) {
        std::cout << "Peer Connection Gathering State: " << connection_gathering_state << std::endl;
    });
    t_peer_connection->onLocalDescription([weak_websocket, client_id](rtc::Description local_description) {
    std::cout << "Got Local Description " << std::string(local_description.typeString()) <<std::endl;
        json  t_message = {
            {"id", client_id},
            {"type", local_description.typeString()},
            {"description", std::string(local_description)}
        };
        std::cout << t_message << std::endl;
        if (auto t_websocket = weak_websocket.lock()) t_websocket->send(t_message.dump());
        //message to send to the websocket
    });
    t_peer_connection->onLocalCandidate([weak_websocket, client_id](rtc::Candidate  local_candidate) {
        std::cout << "Got Local Candidate" << std::string(local_candidate.mid()) << std::endl;
        json  t_message = {
            {"id", client_id},
            {"type", "candidate"},
            {"candidate", std::string(local_candidate)},
            {"mid", local_candidate.mid()}
        };
        std::cout << t_message << std::endl;
        if (auto t_websocket = weak_websocket.lock()) t_websocket->send(t_message.dump());
    });
    t_peer_connection->onDataChannel([client_id, chaos](shared_ptr<rtc::DataChannel> t_data_channel) {
        std::cout << "Data Channel from " << client_id << "received with label \"" << t_data_channel->label() << "\"" <<
                std::endl;
        t_data_channel->onOpen([weak_datachannel = make_weak_ptr(t_data_channel), chaos]() {
            chaos->set_data_channel_open();
        });
        t_data_channel->onClosed([client_id, chaos]() {
            std::cout << "Datachannel from " << client_id << " closed" << std::endl;
            chaos->set_data_channel_open(false);
        });
        t_data_channel->onMessage([client_id, chaos](auto t_data) {
            if (std::holds_alternative<std::string>(t_data)) {
                chaos->add_message(std::get<std::string>(t_data), client_id);
            } else {
                std::cout << "Received a binary message from " << client_id << " ,size=" << std::get<
                    rtc::binary>(t_data).size() << std::endl;
            }
        });
        chaos->data_channel_map.emplace(client_id, t_data_channel);
        // chaos->emplace_data_channel_map(client_id, t_data_channel);
    });
    chaos->peer_connection_map.emplace(client_id, t_peer_connection);
    // chaos->emplace_peer_connection_map(client_id, t_peer_connection);

    return t_peer_connection;
}

void offer_connection(const rtc::Configuration& rtc_config, const shared_ptr<rtc::WebSocket>& web_socket,
                      const std::string& remote_client_id) {
    Chaos* chaos = Chaos::get_instance();
    //TODO: Add id validation in future
    std::cout << "Offering to " << remote_client_id << std::endl;
    auto t_peer_connection = create_peer_connection(rtc_config, web_socket, remote_client_id);
    std::cout << "Creating a data channel with the label \"" << "Test" << "\"" << std::endl;
    auto t_datachannel = t_peer_connection->createDataChannel("Test");
    t_datachannel->onOpen([chaos, remote_client_id]() {
        std::cout << "Data Channel from " << remote_client_id << " has been opened." << std::endl;
        chaos->set_data_channel_open();
    });
    t_datachannel->onClosed([chaos, remote_client_id]() {
        std::cout << "Data Channel from " << remote_client_id << " has closed." << std::endl;
        chaos->set_data_channel_open(false);
    });
    t_datachannel->onMessage([chaos, remote_client_id](auto t_data) {
        if (std::holds_alternative<std::string>(t_data)) {
            chaos->add_message(std::get<std::string>(t_data), remote_client_id);
        } else {
            std::cout << "Received a binary message from " << remote_client_id << " ,size=" << std::get<rtc::binary>(t_data).size() << std::endl;
        }
    });
    chaos->data_channel_map.emplace(remote_client_id, t_datachannel);
}

