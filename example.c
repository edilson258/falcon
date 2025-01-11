#include <stdio.h>

#include <falcon.h>

void on_listen() { fprintf(stderr, "Server is running...\n"); }

void home_handler(fReq *req, fRes *res) { fResOk(res); }

int main(void)
{
  fApp app;
  fGet(&app, "/hello/", home_handler);
  fListen(&app, "127.0.0.1", 8080, on_listen);
}
