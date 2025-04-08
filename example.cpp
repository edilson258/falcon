#include <cassert>
#include <cstdlib>
#include <format>
#include <iostream>
#include <string>
#include <vector>

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
  nlohmann::json body = req.json();
  User user(body["email"], body["password"]);
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
  nlohmann::json json = nlohmann::json::object();
  for (User &u : users) {
    if (u.is_deleted) continue;
    json["users"].push_back({{"email", u.email}, {"password", u.password}});
  }
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

  users.push_back(User("alicey@email.com", "alice123"));
  users.push_back(User("milkey@test.com", "strongpass"));

  app.get("/users/", find_many);
  app.get("/users/:id", find_by_id);
  app.post("/users", create);
  app.delet("/users/:id", delet);

  app.listen(":8000", []() { std::cout << "Http server running...\n"; });
}
