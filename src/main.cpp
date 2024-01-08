//
// Created by shishu on 12/27/23.
//




#include "Chaos.h"



//bool g_is_running = true;
Chaos * chaos = Chaos::get_instance();



int main() {
    auto ws = std::make_shared<rtc::WebSocket>();

    rtc::Configuration config;

    chaos->setup(640, 480,  &config);
    chaos->setup_websockets(make_weak_ptr(ws), &config);

    while(!chaos->is_running()) {
        chaos->render(make_weak_ptr<rtc::WebSocket>(ws), &config);
    }

    chaos->clean();
    return 0;
}
