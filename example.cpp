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

fc::response auth_middleware(fc::request);
fc::response logger_middleware(fc::request);

int main(int argc, char *argv[]) {
  // mocked users
  users.push_back(user_schema("alicey@email.com", "alice123"));
  users.push_back(user_schema("milkey@test.com", "strongpass"));

  fc::app app;
  fc::router router("/users");

  // middlewares
  router.use(auth_middleware);
  router.use(logger_middleware);

  router.post("", create);
  router.get("", find_many);
  router.get("/:id", find_by_id);
  router.delet("/:id", delet);

  app.use(router);

  app.listen(":8000", [](auto &addr) { std::cout << "Listening at " << addr << std::endl; });
}

fc::response logger_middleware(fc::request req) {
  return req.next();
}

fc::response auth_middleware(fc::request req) {
  static std::string token = "uGhTVjLwDb0R/s4xR3mwX/AdymqNbV9htkcRiulIw3E=";
  if (auto authToken = req.get_header("Authorization"); authToken.has_value()) {
    if (!authToken.value().starts_with("Bearer ")) goto unauthorized;
    if (authToken.value().substr(7) != token) goto forbidden;
    return req.next();
  }
unauthorized:
  return fc::response::ok(fc::status::UNAUTHORIZED);
forbidden:
  return fc::response::ok(fc::status::FORBIDDEN);
}
