#include "req.hpp"

namespace fc {

const std::string *Req::GetParam(const std::string &key) const {
  auto it = m_Params.find(key);
  if (it != m_Params.end()) {
    return &it->second;
  }
  return nullptr;
}

Req RequestFactory(void *remote, std::string_view sv) { return Req(remote, sv); }

} // namespace fc
