#include <falcon.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct User
{
  int id;
  char *email;
  char *password;
};

unsigned users_count = 0;
struct User users[1024] = {0};

void on_listen();
void add_user(int id, char *username, char *password);

// Endpoints
void users_find(fc_request_t *req, fc_response_t *res);
void users_find_by_id(fc_request_t *req, fc_response_t *res);
void users_create(fc_request_t *req, fc_response_t *res);

int main(void)
{
  fc_t app;
  fc_init(&app);

  add_user(1, "alice@test.com", "alice123");
  add_user(2, "bob@test.com", "bob123");
  add_user(3, "john@test.com", "john123");

  fc_get(&app, "/users", users_find);
  fc_get(&app, "/users/:id", users_find_by_id);
  fc_post(&app, "/users", users_create);

  return fc_listen(&app, "0.0.0.0", 8080, on_listen);
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

    jjson_key_value id = {.key = "id", .value.type = JSON_NUMBER, .value.data.number = users[i].id};
    jjson_key_value email = {.key = "email", .value.type = JSON_STRING, .value.data.string = users[i].email};
    jjson_key_value passwd = {.key = "password", .value.type = JSON_STRING, .value.data.string = users[i].password};

    jjson_add(j, id);
    jjson_add(j, email);
    jjson_add(j, passwd);

    jjson_array_push(&arr, value);
  }

  jjson_key_value kv = {
      .key = "users",
      .value.type = JSON_ARRAY,
      .value.data.array = arr,
  };

  jjson_add(&json, kv);
  fc_res_json(res, &json);
}

void users_find_by_id(fc_request_t *req, fc_response_t *res)
{
  int id;
  fc_req_get_param_as_int(req, "id", &id);

  struct User *user = NULL;
  for (int i = 0; i < users_count; ++i)
  {
    if (id == users[i].id)
    {
      user = &users[i];
      break;
    }
  }

  jjson_t *json = malloc(sizeof(jjson_t));
  jjson_init(json);

  if (user)
  {
    jjson_key_value id = {.key = "id", .value.type = JSON_NUMBER, .value.data.number = user->id};
    jjson_key_value email = {.key = "email", .value.type = JSON_STRING, .value.data.string = user->email};
    jjson_key_value passwd = {.key = "password", .value.type = JSON_STRING, .value.data.string = user->password};

    jjson_add(json, id);
    jjson_add(json, email);
    jjson_add(json, passwd);
  }
  else
  {
    jjson_key_value null_user = {.key = "user", .value.type = JSON_NULL};
    jjson_add(json, null_user);
  }

  fc_res_json(res, json);
}

void users_create(fc_request_t *req, fc_response_t *res)
{
  fc_res_set_status(res, FC_STATUS_CREATED);
  fc_res_ok(res);
}
