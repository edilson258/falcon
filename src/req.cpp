#include "req.hpp"

namespace fc
{

Req RequestFactory(void *remote, std::string_view sv) { return Req(remote, sv); }

} // namespace fc
