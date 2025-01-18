#include "jack.h"
#include <falcon.h>
#include <stdio.h>
#include <stdlib.h>

struct User
{
  char *email;
  char *password;
};

fschema_t user_schema = {
    .nfields = 2,
    .fields = {
        {.name = "email", .type = JSON_STRING},
        {.name = "password", .type = JSON_STRING},
    }};

unsigned users_count = 0;
struct User users[1024];

void on_listen();
void add_user(char *username, char *password);

// Endpoints
void users_find(frequest_t *req, fresponse_t *res);
void users_create(frequest_t *req, fresponse_t *res);

int main(void)
{
  falcon_t app;
  fget(&app, "/users", users_find);
  fpost(&app, "/users", users_create, &user_schema);
  flisten(&app, "0.0.0.0", 8080, on_listen);
}

void on_listen()
{
  fprintf(stderr, "Server is running...\n");
}

void add_user(char *email, char *password)
{
  users[users_count++] = (struct User){.email = email, .password = password};
}

void users_find(frequest_t *req, fresponse_t *res)
{
  jjson_t json;
  jjson_array arr;

  jjson_init(&json);
  jjson_init_array(&arr);

  for (int i = 0; i < users_count; ++i)
  {
    jjson_key_value email = {
        .key = "email",
        .value.type = JSON_STRING,
        .value.data.string = users[i].email};
    jjson_key_value passwd = {
        .key = "password",
        .value.type = JSON_STRING,
        .value.data.string = users[i].password};

    jjson_t *j = malloc(sizeof(jjson_t));
    jjson_init(j);

    jjson_add(j, email);
    jjson_add(j, passwd);

    jjson_value value;
    value.type = JSON_OBJECT;
    value.data.object = j;

    jjson_array_push(&arr, value);
  }

  jjson_key_value kv = {
      .key = "users",
      .value.type = JSON_ARRAY,
      .value.data.array = arr,
  };

  jjson_add(&json, kv);
  fres_json(res, &json);
}

void users_create(frequest_t *req, fresponse_t *res)
{
  jjson_t json = *((jjson_t *)req->body);
  JJSON_GET_STRING(json, "email", email);
  JJSON_GET_STRING(json, "password", password);
  add_user(email, password);

  fres_set_status(res, FHTTP_STATUS_CREATED);
  fres_ok(res);
}
