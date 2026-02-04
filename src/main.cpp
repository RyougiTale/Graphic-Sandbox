#include "Core/Application.h"
#include <iostream>

int main() {
    try {
        // Pass 0, 0 to auto-size window to 80% of screen
        Application app(0, 0, "Graphics Sandbox");
        app.Run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
