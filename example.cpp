#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "external/simdjson/simdjson.h"
#include "include/fc.hpp"

struct User {
public:
  std::string email;
  std::string password;

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

fc::response find_many(fc::request req) {
  return fc::response::ok();
}

fc::response find_by_id(fc::request req) {
  return fc::response::json(R"({"name": "Edilson"})");
}

int main(int argc, char *argv[]) {
  fc::app app;

  app.get("/users", find_many);
  app.get("/users/:id", find_by_id);
  app.post("/users", create);

  app.listen(":8000", []() { std::cout << "Http server running...\n"; });
}
