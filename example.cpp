#include <cassert>
#include <cstdlib>
#include <format>
#include <iostream>
#include <string>
#include <vector>

#include "external/simdjson/simdjson.h"
#include "include/fc.hpp"

struct User {
public:
  int id;
  std::string email;
  std::string password;

  bool is_deleted = false;

  User(std::string email, std::string password) : email(email), password(password) {};
};

std::vector<User> users;

fc::response create(fc::request req) {
  simdjson::dom::element doc;
  assert(req.bind_to_json(&doc));
  User user(doc["email"].get_string().value().data(), doc["password"].get_string().value().data());
  users.push_back(user);
  return fc::response::ok(fc::status::CREATED);
}

fc::response delet(fc::request req) {
  auto id = (std::stoi(req.get_param("id").value())) - 1;
  if (id >= users.size() || users.at(id).is_deleted) {
    return fc::response::ok(fc::status::NOT_FOUND);
  }
  users.at(id).is_deleted = true;
  return fc::response::ok(fc::status::NO_CONTENT);
}

fc::response find_many(fc::request req) {
  std::string json = R"({ "users": [)";
  // size_t alive_count = std::count_if(users.begin(), users.end(), [](User &u) { return !u.is_deleted; });
  // TODO: filter deleted users
  for (auto i = 0; i < users.size(); ++i) {
    json += std::format(R"({{ "name": "{}", "password": "{}" }})", users.at(i).email, users.at(i).password);
    if (i + 1 < users.size()) json.push_back(',');
  }
  json += "]}";
  return fc::response::json(json);
}

fc::response find_by_id(fc::request req) {
  auto id = (std::stoi(req.get_param("id").value())) - 1;
  if (id >= users.size() || users.at(id).is_deleted) {
    return fc::response::ok(fc::status::NOT_FOUND);
  }
  auto json = std::format(R"({{ "name": "{}", "password": "{}" }})", users.at(id).email, users.at(id).password);
  return fc::response::json(json);
}

int main(int argc, char *argv[]) {
  fc::app app;

  app.get("/users/", find_many);
  app.get("/users/:id", find_by_id);
  app.post("/users", create);
  app.delet("/users/:id", delet);

  app.listen(":8000", []() { std::cout << "Http server running...\n"; });
}
