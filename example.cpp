#include "include/fc.hpp"
#include <iostream>

// clang-format off
int main(int argc, char *argv[])
{
  fc::App app;
  app.Get("/users/:id", [](fc::Req req) { std::cout << *req.GetParam("id") << "\n"; return fc::Res::Ok(); });
  app.Listen(":8000", [](){ std::cout << "Http server running...\n"; });
}
