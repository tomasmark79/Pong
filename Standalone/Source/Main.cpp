
#include <Engine/Engine.hpp>
#include <engine/version.h>

#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

int main()
{
    std::unique_ptr<Engine> lib = std::make_unique<Engine>(); // cpp14 +
    // std::unique_ptr<Engine> lib(new Engine()); // cpp11 +

    return 0;
}
