//
// Created by shishu on 12/30/23.
//

#ifndef APPLICATION_H
#define APPLICATION_H
#include "imgui.h"
#include "GLFW/glfw3.h"
#include<iostream>
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <gst/gst.h>
#include "Utils.h"

using std::weak_ptr;
using std::shared_ptr;

struct ChaosMessage {
    std::string content;
    std::string client_id;
    //TODO: Add time representation
};

class Chaos {
protected:
    static Chaos* chaos_;
    Chaos(){}
public:
    Chaos(Chaos &other) = delete;
    void operator=(const Chaos &) = delete;

    void setup(int width, int height, rtc::Configuration *rtc_config);

    void clean();
    void render(const weak_ptr<rtc::WebSocket>& wws, rtc::Configuration *rtc_config);
    static void error_callback(const std::string& error_message);

    [[nodiscard]] bool is_running() const {
        return glfwWindowShouldClose(m_window);
    }
   void set_data_channel_open(bool const is_open = true) {
        data_channel_open = is_open;
    }
    void add_message(std::string const& message_content, std::string const& client_id) {
        m_user_messages.emplace_back<ChaosMessage>({message_content, client_id});
    }
    static Chaos * get_instance();
    static void setup_websockets(const weak_ptr<rtc::WebSocket>& message_data, rtc::Configuration * rtc_config);

    std::unordered_map<std::string, shared_ptr<rtc::PeerConnection>> peer_connection_map;
    std::unordered_map<std::string, shared_ptr<rtc::DataChannel>> data_channel_map;

    std::string local_client_id;
    std::string remote_client_id;

    shared_ptr<rtc::DataChannel> m_data_channel;
private:
    GLFWwindow * m_window = nullptr;
    ImVec4 clear_color = ImVec4(0.45f,0.55f,0.60f,1.0f);
    ImGuiWindowFlags m_window_flags = 0;

    std::string peer_connection_state_i;
    bool data_channel_open = false;
    std::vector<ChaosMessage> m_user_messages;








};

#endif //APPLICATION_H
