#pragma once

#include <iostream>
#include <signal.h>

class signals {
public:
  static void ignore_sigpipe() {
#ifndef _WIN32
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    if (sigemptyset(&sa.sa_mask) != 0 || sigaction(SIGPIPE, &sa, nullptr) != 0) {
      std::cerr << "[FALCON ERROR]: Failed to ignore SIGPIPE,  this may crash the entire application on Unix-like systems when the client closes the connection early" << std::endl;
    }
#endif
  }
};
