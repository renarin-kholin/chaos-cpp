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


shared_ptr<rtc::PeerConnection> create_peer_connection(const rtc::Configuration& rtc_config, const weak_ptr<rtc::WebSocket>& weak_websocket, std::string client_id) {
    Chaos *chaos = Chaos::get_instance();
    auto t_peer_connection = std::make_shared<rtc::PeerConnection>(rtc_config);

    t_peer_connection->onStateChange([](rtc::PeerConnection::State state) {
        std::cout << "State: " << state << std::endl;
    });
    t_peer_connection->onGatheringStateChange([](rtc::PeerConnection::GatheringState state ) {
        std::cout << "Gathering State: " << state << std::endl;
    });
    t_peer_connection->onLocalDescription([weak_websocket, client_id](const rtc::Description& description) {
        json message = {
                {"client_id",          client_id},
                {"type",        description.typeString()},
                {"description", std::string(description)}
        };
        if(auto ws = weak_websocket.lock()) ws->send(message.dump());
    });
    t_peer_connection->onLocalCandidate([weak_websocket, client_id](const rtc::Candidate& candidate) {
        json message = {{"client_id", client_id}, {"type", "candidate"}, {"candidate", std::string(candidate)}, {"mid", candidate.mid()}};
        if(auto ws = weak_websocket.lock()) ws->send(message.dump());
    });
    t_peer_connection->onDataChannel([client_id, chaos](const shared_ptr<rtc::DataChannel>& dc) {
        chaos->m_data_channel = dc;
        std::cout << "DataChannel from " << client_id << "received with label \"" << dc->label() << "\"" << std::endl;
        setup_datachannel_handlers(dc, client_id);
    });
    chaos->peer_connection_map.emplace(client_id, t_peer_connection);
    return t_peer_connection;
}
void setup_datachannel_handlers(const shared_ptr<rtc::DataChannel>& t_datachannel, const std::string& client_id){
    Chaos *chaos = Chaos::get_instance();
    t_datachannel->onOpen([wdc = make_weak_ptr(t_datachannel), chaos]() {

        chaos->set_data_channel_open();
        if(auto dc = wdc.lock()) dc->send("Connection succeeded with " + chaos->local_client_id);
    });
    t_datachannel->onClosed([client_id]() { std::cout << "DataChannel from " << client_id << " closed" << std::endl; });

    t_datachannel->onMessage([client_id, chaos](auto data) {
        // data holds either std::string or rtc::binary
        if (std::holds_alternative<std::string>(data)) {
            std::string message_content = std::get<std::string>(data);
            chaos->add_message(message_content, client_id);
            std::cout << "Message from " << client_id << " received: " << std::get<std::string>(data)
                      << std::endl;
        }
        else
            std::cout << "Binary message from " << client_id
                      << " received, size=" << std::get<rtc::binary>(data).size() << std::endl;
    });

    chaos->data_channel_map.emplace(client_id, t_datachannel);
}

void send_message(const weak_ptr<rtc::DataChannel>& weak_data_channel, std::string message_content){
    Chaos *chaos = Chaos::get_instance();
    if(auto t_datachannel = weak_data_channel.lock()) t_datachannel->send(message_content);
    chaos->add_message(message_content, chaos->local_client_id);
}

