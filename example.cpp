#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "external/nlohmann/json.hpp"
#include "include/fc.hpp"

struct user_schema {
public:
  int m_id;
  std::string m_email;
  std::string m_password;
  bool m_is_deleted = false;

  user_schema(std::string email, std::string password) : m_email(email), m_password(password) {};
};

std::vector<user_schema> users;

fc::response create(fc::request req) {
  auto body = req.json();
  user_schema user(body["email"], body["password"]);
  users.push_back(user);
  return fc::response::ok(fc::status::CREATED);
}

fc::response delet(fc::request req) {
  auto id = (std::stoi(req.get_param("id").value())) - 1;
  if (id >= users.size() || users.at(id).m_is_deleted) {
    return fc::response::ok(fc::status::NOT_FOUND);
  }
  users.at(id).m_is_deleted = true;
  return fc::response::ok(fc::status::NO_CONTENT);
}

fc::response find_many(fc::request req) {
  nlohmann::json json = nlohmann::json::object();
  for (user_schema &u : users) {
    if (u.m_is_deleted) continue;
    json["users"].push_back({{"email", u.m_email}, {"password", u.m_password}});
  }
  return fc::response::json(json);
}

fc::response find_by_id(fc::request req) {
  auto id = (std::stoi(req.get_param("id").value())) - 1;
  if (id >= users.size() || users.at(id).m_is_deleted) {
    return fc::response::ok(fc::status::NOT_FOUND);
  }
  return fc::response::json({{"email", users.at(id).m_email}, {"password", users.at(id).m_password}});
}

int main(int argc, char *argv[]) {
  // mocked users
  users.push_back(user_schema("alicey@email.com", "alice123"));
  users.push_back(user_schema("milkey@test.com", "strongpass"));

  fc::app app;
  fc::router router("/users");

  // middleware
  router.use([](fc::request req) { std::cout << "Request on users/" << std::endl; return req; });

  router.post("", create);
  router.get("", find_many);
  router.get("/:id", find_by_id);
  router.delet("/:id", delet);

  app.use(router);

  app.listen(":8000", [](auto &addr) { std::cout << "Listening at " << addr << std::endl; });
}
