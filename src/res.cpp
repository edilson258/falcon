#include "res.hpp"
#include "fc.hpp"
#include "templates.hpp"

namespace fc
{

Res Res::Ok()
{
  return Res(templates::OK_RESPONSE);
}

} // namespace fc
