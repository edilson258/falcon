#include "include/fc.hpp"
#include <iostream>

// clang-format off
int main(int argc, char *argv[])
{
  fc::App app;
  app.Get("/users", [](fc::Req req) { std::cout << "Got users\n"; return fc::Res::Ok(); });
  app.Listen(":8000", [](){ std::cout << "Http server running...\n"; });
}
