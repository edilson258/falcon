#include <falcon.h>
#include <stdio.h>

void on_listen() { fprintf(stderr, "Server is running...\n"); }

void home_handler(falcon_request *req, falcon_response *res) { falcon_ok(res); }

int main(void)
{
  falcon app;
  falcon_get(&app, "/hello", home_handler);
  falcon_listen(&app, "127.0.0.1", 8080, on_listen);
}
