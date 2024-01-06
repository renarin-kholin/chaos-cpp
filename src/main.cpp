//
// Created by shishu on 12/27/23.
//

#include "main.h"
#include <iostream>

#include "Chaos.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "GLFW/glfw3.h"


bool g_is_running = true;
Chaos * chaos = Chaos::get_instance();



int main() {
    chaos->setup(640, 480);


    while(!chaos->is_running()) {
        chaos->render();
    }

    chaos->clean();
    return 0;
}
