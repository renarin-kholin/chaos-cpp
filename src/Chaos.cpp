//
// Created by shishu on 12/30/23.
//
#include "Chaos.h"
#include <cstring>
#include "imgui_impl_glfw.h"
#include <gst/app/app.h>

void Chaos::error_callback(const std::string& error_message) {
    std::cout << "ERROR::" << error_message << std::endl;
}
Chaos* Chaos::chaos_ = 0;
Chaos* Chaos::get_instance() {
    if(chaos_ == nullptr) {
        chaos_ =  new Chaos();
    }
    return chaos_;
}


void Chaos::setup(int width, int height) {
    glfwInit();
    m_window = glfwCreateWindow(width, height, "Chaos", nullptr, nullptr);
    if (m_window == nullptr) {
        error_callback("GLFW could not create a window.");
        exit(1);
    }
    glfwMakeContextCurrent(m_window);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void) io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    m_window_flags |= ImGuiWindowFlags_NoTitleBar;
    m_window_flags |= ImGuiWindowFlags_NoMove;
    m_window_flags |= ImGuiWindowFlags_NoResize;

    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init();

    rtc::InitLogger(rtc::LogLevel::Info);

    // auto* ice_server = new rtc::IceServer("turn:global.relay.metered.ca", 80, "46c967b98ec9994d702767e8",
    //                                       "lcz6Ykhd14hYyKfP");
    // rtc_config.iceServers.emplace_back(*ice_server);
    rtc_config.iceServers.emplace_back("stun1.l.google.com:19302");
    rtc_config.enableIceUdpMux = false;
    local_client_id = get_random_id(4);
    setup_websockets();
}

void Chaos::setup_websockets() {
    m_websocket = std::make_shared<rtc::WebSocket>();
    std::promise<void> ws_promise;
    auto ws_future = ws_promise.get_future();
    m_websocket->onOpen([&ws_promise]() {
        std::cout << "Websocket connected, signaling is ready." << std::endl;
        ws_promise.set_value();
    });
    m_websocket->onClosed([]() {
        std::cout << "Websocket connection closed." << std::endl;
    });
    m_websocket->onError([&ws_promise](const std::string& error_message) {
        std::cout << "Websocket error." << std::endl;
        ws_promise.set_exception(std::make_exception_ptr(std::runtime_error(error_message)));
    });
    m_websocket->onMessage([ this, weak_websocket = make_weak_ptr(m_websocket)](auto message_data) {
        if (!std::holds_alternative<std::string>(message_data)) return;
        json message = json::parse(std::get<std::string>(message_data)); //TODO: Remove this if this line works correctly

        auto const s_id_iterator = message.find("id");
        if (s_id_iterator == message.end()) return;
        auto const t_client_id = s_id_iterator->get<std::string>();

        auto const s_type_iterator = message.find("type");
        if (s_type_iterator == message.end()) return;
        auto const t_type = s_type_iterator->get<std::string>();


        shared_ptr<rtc::PeerConnection> t_peer_connection;
        if (auto const jt = peer_connection_map.find(t_client_id); jt != peer_connection_map.end()) {
            t_peer_connection = jt->second;
        } else if (t_type == "offer") {
            std::cout << "Got an Offer from " << t_client_id << "\nAnswering..." << std::endl;
            t_peer_connection = create_peer_connection(rtc_config, weak_websocket, t_client_id);
        } else {
            return;
        }
        if (t_type == "offer" || t_type == "answer") {
            auto const t_sdp = message["description"].get<std::string>();
            t_peer_connection->setRemoteDescription(rtc::Description(t_sdp, t_type));
        } else if (t_type == "candidate") {
            auto const t_sdp = message["description"].get<std::string>();
            auto const t_mid = message["mid"].get<std::string>();
            t_peer_connection->addRemoteCandidate(rtc::Candidate(t_sdp, t_mid));
        }
    });
    const std::string url = "wss://bfc64876-21c8-4c1d-b730-4f035ddc995f-00-3f103q41zt6ow.worf.replit.dev/" + local_client_id;
    std::cout << "Websocket url is: " << url << std::endl;
    m_websocket->open(url);
    std::cout << "Waiting for signaling to be connected." << std::endl;
    ws_future.get();

}


void Chaos::render() {
    glfwPollEvents();


    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();


    int display_w, display_h;
    glfwGetFramebufferSize(m_window, &display_w, &display_h);

    ImGui::SetNextWindowPos(ImVec2(0, 0), 0);
    ImGui::SetNextWindowSize(ImVec2(static_cast<float>(display_w), static_cast<float>(display_h)));

    ImGui::Begin("Chaos", 0, m_window_flags);
    ImGui::TextWrapped("Messages");

    ImGui::TextWrapped(std::string("Your userid: " + local_client_id).data());

    static char str0[128] = "";
    static char SDP_REMOTE[512];
    ImGui::InputTextWithHint("Enter other client's userid", "SDP", SDP_REMOTE, IM_ARRAYSIZE(SDP_REMOTE));
    if(ImGui::Button("Connect", ImVec2(80.0f, 30.0f))) {
        offer_connection(rtc_config, m_websocket, std::string(SDP_REMOTE));
    };

    if (!data_channel_open) {
        ImGui::BeginDisabled();
    }

    ImGui::InputText("Enter your message", str0, IM_ARRAYSIZE(str0));
    ImGui::Button("Send", ImVec2(80.0f, 30.0f));

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
    data_channel_map.clear();
    peer_connection_map.clear();
    glfwDestroyWindow(m_window);
    glfwTerminate();
}


