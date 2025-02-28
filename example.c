#include "falcon/router.h"
#include <assert.h>
#include <falcon.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct User
{
  int id;
  char *email;
  char *password;
};

static int last_id = 1;
int get_user_id()
{
  return last_id++;
}

unsigned users_count = 0;
struct User users[1024] = {0};

void on_listen();
void add_user(int id, char *username, char *password);

// Endpoints
void users_find(fc_request_t *req, fc_response_t *res);
void users_find_by_id(fc_request_t *req, fc_response_t *res);
void users_create(fc_request_t *req, fc_response_t *res);

const fc_schema_t users_create_schema = {
    .nfields = 2,
    .fields = {{.name = "email", .type = FC_STRING_T}, {.name = "password", .type = FC_STRING_T}}};

int main(void)
{
  fc_app *app = fc_app_new();
  fc_router *router = fc_router_new();

  /* mock users */
  add_user(get_user_id(), "alice@test.com", "alice123");
  add_user(get_user_id(), "bob@test.com", "bob123");
  add_user(get_user_id(), "john@test.com", "john123");

  fc_get(router, "/users", users_find, NULL);
  fc_get(router, "/users/:id", users_find_by_id, NULL);
  fc_post(router, "/users", users_create, &users_create_schema);

  fc_use_router(app, router);

  return fc_listen(app, "0.0.0.0", 8080, on_listen);
}

void on_listen()
{
  fprintf(stderr, "Server is running...\n");
}

void add_user(int id, char *email, char *password)
{
  users[users_count++] = (struct User){.id = id, .email = email, .password = password};
}

void users_find(fc_request_t *req, fc_response_t *res)
{
  jjson_t json;
  jjson_array arr;

  jjson_init(&json);
  jjson_init_array(&arr);

  for (int i = 0; i < users_count; ++i)
  {
    jjson_t *j = malloc(sizeof(jjson_t));
    jjson_init(j);
    jjson_value value;
    value.type = JSON_OBJECT;
    value.data.object = j;

    jjson_key_value id = {.key = strdup("id"), .value.type = JSON_NUMBER, .value.data.number = users[i].id};
    jjson_key_value email = {.key = strdup("email"), .value.type = JSON_STRING, .value.data.string = strdup(users[i].email)};
    jjson_key_value passwd = {.key = strdup("password"), .value.type = JSON_STRING, .value.data.string = strdup(users[i].password)};

    jjson_add(j, id);
    jjson_add(j, email);
    jjson_add(j, passwd);

    jjson_array_push(&arr, value);
  }

  jjson_key_value kv = {
      .key = strdup("users"),
      .value.type = JSON_ARRAY,
      .value.data.array = arr,
  };

  jjson_add(&json, kv);
  fc_res_json(res, &json);

  jjson_deinit(&json);
}

void users_find_by_id(fc_request_t *req, fc_response_t *res)
{
  char *id_buf = NULL;
  size_t id_buf_len = 0;
  assert(FC_ERR_OK == fc_req_get_param(req, "id", &id_buf, &id_buf_len) && id_buf);
  int id = (int)strtol(id_buf, NULL, 10);
  printf("%s\n", id_buf);

  char *theme_buf;
  size_t theme_buf_len;
  if (FC_ERR_OK == fc_req_get_cookie(req, "Theme", &theme_buf, &theme_buf_len))
  {
    printf("Theme: %.*s\n", (int)theme_buf_len, theme_buf);
  }

  struct User *user = NULL;
  for (int i = 0; i < users_count; ++i)
  {
    if (id == users[i].id)
    {
      user = &users[i];
      break;
    }
  }

  jjson_t json;
  jjson_init(&json);

  if (user)
  {
    jjson_key_value id = {.key = strdup("id"), .value.type = JSON_NUMBER, .value.data.number = user->id};
    jjson_key_value email = {.key = strdup("email"), .value.type = JSON_STRING, .value.data.string = strdup(user->email)};
    jjson_key_value passwd = {.key = strdup("password"), .value.type = JSON_STRING, .value.data.string = strdup(user->password)};

    jjson_add(&json, id);
    jjson_add(&json, email);
    jjson_add(&json, passwd);
  }
  else
  {
    jjson_key_value null_user = {.key = strdup("user"), .value.type = JSON_NULL};
    jjson_add(&json, null_user);
  }

  fc_res_json(res, &json);

  jjson_deinit(&json);
}

void users_create(fc_request_t *req, fc_response_t *res)
{
  jjson_t *json = (jjson_t *)req->body;

  int id = get_user_id();
  JJSON_GET_STRING(*json, "email", email);
  JJSON_GET_STRING(*json, "password", password);
  add_user(id, strdup(email), strdup(password));

  jjson_t res_json;
  jjson_init(&res_json);
  jjson_add_number(&res_json, "id", id);

  fc_res_set_status(res, FC_STATUS_CREATED);
  fc_res_json(res, &res_json);

  jjson_deinit(&res_json);
}
