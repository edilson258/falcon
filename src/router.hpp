#pragma once

#include "fc.hpp"

namespace fc
{

enum class FragType
{
  STATIC = 1,
  DYNAMIC = 2,
  WILDCARD = 3,
};

struct Frag
{
public:
  FragType m_Type;
  std::string m_Label;
  std::array<PathHandler, static_cast<int>(Method::COUNT)> m_Handlers;

  Frag *m_Next;
  Frag *m_Child;

  Frag() = default;
  Frag(FragType type, std::string label) : m_Type(type), m_Label(label), m_Next(nullptr), m_Child(nullptr)
  {
    std::fill(m_Handlers.begin(), m_Handlers.end(), nullptr);
  };
};

struct Router
{
public:
  Frag m_Root;

  Router() = default;

  void AddRoute(Method method, const std::string, PathHandler);
  PathHandler MatchRoute(Method method, const std::string_view &, Req &) const;

  static std::vector<std::string> FragmentPath(const std::string_view &);
  static std::string NormalizePath(const std::string_view &);
};

} // namespace fc
