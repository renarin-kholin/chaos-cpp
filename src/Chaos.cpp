//
// Created by shishu on 12/30/23.
//
#include "Chaos.h"
#include <cstring>
#include <memory>

#include "imgui_impl_glfw.h"
#include <gst/app/app.h>

void Chaos::error_callback(const std::string& error_message) {
    std::cout << "ERROR::" << error_message << std::endl;
}
Chaos* Chaos::chaos_ = nullptr;
Chaos* Chaos::get_instance() {
    if(chaos_ == nullptr) {
        chaos_ =  new Chaos();
    }
    return chaos_;
}

using std::weak_ptr;
using std::shared_ptr;
using nlohmann::json;

//std::unordered_map<std::string, shared_ptr<rtc::PeerConnection>> peerConnectionMap;
//std::unordered_map<std::string, shared_ptr<rtc::DataChannel>> dataChannelMap;



void Chaos::setup(int width, int height, rtc::Configuration *rtc_config) {
    glfwInit();
    m_window = glfwCreateWindow(width, height, "Chaos", nullptr, nullptr);
    if (m_window == nullptr) {
        error_callback("GLFW could not create a window.");
        exit(1);
    }
    glfwMakeContextCurrent(m_window);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    m_window_flags |= ImGuiWindowFlags_NoTitleBar;
    m_window_flags |= ImGuiWindowFlags_NoMove;
    m_window_flags |= ImGuiWindowFlags_NoResize;

    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init();



    rtc::InitLogger(rtc::LogLevel::Info);

    auto *ice_server = new rtc::IceServer("turn:global.relay.metered.ca", 80, "46c967b98ec9994d702767e8",
                                          "lcz6Ykhd14hYyKfP");
    rtc_config->iceServers.emplace_back(*ice_server);
    rtc_config->enableIceUdpMux = true;
    local_client_id = get_random_id(4);
    std::cout << "Local ID is " << local_client_id << std::endl;





}

void Chaos::setup_websockets(const weak_ptr<rtc::WebSocket>& weak_websocket, rtc::Configuration *rtc_config){
    Chaos *chaos = Chaos::get_instance();
    auto t_websocket = weak_websocket.lock();
    std::promise<void> ws_promise;
    auto ws_future = ws_promise.get_future();
    t_websocket->onOpen([&ws_promise]() {
        std::cout << "Websocket connected, signaling ready" << std::endl;
        ws_promise.set_value();
    });
    t_websocket->onError([&ws_promise](const std::string& s) {
        std::cout << "Websocket error" << std::endl;
        ws_promise.set_exception(std::make_exception_ptr(std::runtime_error(s)));
    });

    t_websocket->onClosed([]() {
        std::cout << "Websocket closed" << std::endl;
    });
    t_websocket->onMessage([&rtc_config, weak_websocket, chaos](auto message_data) {
        if (!std::holds_alternative<std::string>(message_data)) return;
        json message = json::parse(std::get<std::string>(message_data));

        auto t_iterator = message.find("client_id");
        if (t_iterator == message.end())return;

        auto client_id = t_iterator->get<std::string>();

        t_iterator = message.find("type");
        if (t_iterator == message.end())return;

        auto type = t_iterator->get<std::string>();

        shared_ptr<rtc::PeerConnection> t_peer_connection;
        if (auto jt = chaos->peer_connection_map.find(client_id); jt != chaos->peer_connection_map.end()) {
            t_peer_connection = jt->second;
        } else if (type == "offer") {
            std::cout << "Answering to " << client_id << std::endl;
            chaos->remote_client_id = client_id;
            t_peer_connection = create_peer_connection(*rtc_config, weak_websocket, client_id);
        } else {
            return;
        }
        if (type == "offer" || type == "answer") {
            auto sdp = message["description"].get<std::string>();
            t_peer_connection->setRemoteDescription(rtc::Description(sdp, type));
        } else if (type == "candidate") {
            auto sdp = message["candidate"].get<std::string>();
            auto mid = message["mid"].get<std::string>();
            t_peer_connection->addRemoteCandidate(rtc::Candidate(sdp, mid));
        }


    });
    const std::string wsPrefix = "wss://";
    const std::string url =
            wsPrefix + "8d075384-9529-481f-a562-e354bba2f403-00-20ijkdyqcex8u.janeway.replit.dev" + "/" +
            chaos->local_client_id;
    std::cout << "websocket url is " << url << std::endl;

    t_websocket->open(url);
    std::cout << "Waiting for signaling to be connected..." << std::endl;
    ws_future.get();

}




void Chaos::render(const weak_ptr<rtc::WebSocket>& weak_websocket, rtc::Configuration *config) {
    glfwPollEvents();


    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();


    int display_w, display_h;
    glfwGetFramebufferSize(m_window, &display_w, &display_h);

    ImGui::SetNextWindowPos(ImVec2(0, 0), 0);
    ImGui::SetNextWindowSize(ImVec2(static_cast<float>(display_w), static_cast<float>(display_h)));

    ImGui::Begin("Chaos", nullptr, m_window_flags);
    ImGui::TextWrapped("Messages");
    for(const ChaosMessage& message : m_user_messages){
        ImGui::TextWrapped("%s", std::string(message.client_id + ": " + message.content).data());
    }

    ImGui::TextWrapped("%s", std::string("YOUR USERID: " + local_client_id).data());


    static char CLIENT_ID_REMOTE[5];
    ImGui::InputTextWithHint("Enter other client's userid", "client ID", CLIENT_ID_REMOTE, IM_ARRAYSIZE(CLIENT_ID_REMOTE));
    if(ImGui::Button("Connect", ImVec2(80.0f, 30.0f))) {
        auto ws = weak_websocket.lock();

        remote_client_id = std::string (CLIENT_ID_REMOTE);
        std::cout << "Run" << std::endl;
        std::cout << "Offering to " + remote_client_id << std::endl;
        auto pc = create_peer_connection(*config, ws, remote_client_id);
        const std::string label = "test";
        std::cout << "Creating a DataChannel with the label \"" << label << "\"" << std::endl;
        m_data_channel = pc->createDataChannel(label);
        setup_datachannel_handlers(m_data_channel, remote_client_id);
//        offer_connection(rtc_config, m_websocket, std::string(SDP_REMOTE));
    }

    if (!data_channel_open) {
        ImGui::BeginDisabled();
    }
    static char message_input[256] = "";
    ImGui::InputText("Enter your message", message_input, IM_ARRAYSIZE(message_input));
    if(ImGui::Button("Send", ImVec2(80.0f, 30.0f))){
        send_message(make_weak_ptr(m_data_channel), message_input);
        memset(message_input, 0, sizeof(message_input));

    }

    if (!data_channel_open) {
        ImGui::EndDisabled();
    }
    // ImGui::BeginDisabled();
    // ImGui::Button("Voice Call", ImVec2(100.0f, 30.0f));
    // ImGui::EndDisabled();


    ImGui::End();
    ImGui::Render();

    glViewport(0, 0, display_w, display_h);
    glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w,
                 clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(m_window);
}


void Chaos::clean() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(m_window);
    glfwTerminate();
}


