#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "external/nlohmann/json.hpp"
#include "include/fc.hpp"

struct user_schema {
public:
  int m_id;
  std::string m_email;
  std::string m_password;
  bool m_isdeleted = false;

  user_schema(std::string email, std::string pwd) : m_email(email), m_password(pwd) {};
};

std::vector<user_schema> users_db;

fc::response users_create(fc::request &req);
fc::response users_delete(fc::request &req);
fc::response users_find_many(fc::request &req);
fc::response users_find_by_id(fc::request &req);

fc::response auth_middleware(fc::request &);
fc::response logger_middleware(fc::request &);

int main(int argc, char *argv[]) {
  // mocked users
  users_db.push_back(user_schema("alicey@email.com", "alice123"));
  users_db.push_back(user_schema("milkey@test.com", "strongpass"));

  fc::app app;
  fc::router router("/users");

  // middlewares
  router.use(logger_middleware);
  router.use(auth_middleware);

  router.post("", users_create);
  router.get("", users_find_many);
  router.get("/:id", users_find_by_id);
  router.delet("/:id", users_delete);

  // render html file
  app.get("/", [](fc::request &req) {
    return fc::response::render("index.html");
  });

  app.use(router);
  app.listen(":8000");
}

fc::response users_find_by_id(fc::request &req) {
  auto id = (std::stoi(std::string(req.get_param("id").value())) - 1);
  if (id >= users_db.size() || users_db.at(id).m_isdeleted) {
    return fc::response::ok(fc::status::NOT_FOUND);
  }
  return fc::response::json({{"email", users_db.at(id).m_email}, {"password", users_db.at(id).m_password}});
}

fc::response users_find_many(fc::request &req) {
  nlohmann::json json = nlohmann::json::object();
  for (user_schema &u : users_db) {
    if (u.m_isdeleted) continue;
    json["users"].push_back({{"email", u.m_email}, {"password", u.m_password}});
  }
  return fc::response::json(json);
}

fc::response users_create(fc::request &req) {
  auto body = req.json();
  user_schema user(body["email"], body["password"]);
  users_db.push_back(user);
  return fc::response::ok(fc::status::CREATED);
}

fc::response users_delete(fc::request &req) {
  auto id = (std::stoi(std::string(req.get_param("id").value())) - 1);
  if (id >= users_db.size() || users_db.at(id).m_isdeleted) {
    return fc::response::ok(fc::status::NOT_FOUND);
  }
  users_db.at(id).m_isdeleted = true;
  return fc::response::ok(fc::status::NO_CONTENT);
}

fc::response logger_middleware(fc::request &req) {
  auto res = req.next();
  std::cout << fc::method_to_string(req.get_method()) << " " << req.get_path() << " " << static_cast<int>(res.get_status()) << std::endl;
  return res;
}

bool is_valid_token(std::string_view token) {
  static std::string prefix = "Bearer ";
  static std::string expected_token = "uGhTVjLwDb0R/s4xR3mwX/AdymqNbV9htkcRiulIw3E=";
  if (!token.starts_with(prefix) || token.substr(prefix.length()) != expected_token) return false;
  return true;
}

fc::response auth_middleware(fc::request &req) {
  if (auto auth_header = req.get_header("Authorization"); auth_header.has_value()) {
    if (is_valid_token(auth_header.value())) {
      return req.next();
    }
  }
  return fc::response::ok(fc::status::UNAUTHORIZED);
}
