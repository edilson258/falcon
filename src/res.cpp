#include "res.hpp"
#include "include/fc.hpp"
#include "src/templates.hpp"

namespace fc {

const response response::ok() {
  return response(templates::OK_RESPONSE);
}

void response::set_status(status status) {
  m_status = status;
}

void response::set_content_type(const std::string cont_type) {
  m_content_type = cont_type;
}

} // namespace fc
