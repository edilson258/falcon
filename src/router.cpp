#include <cstring>
#include <stdexcept>

#include "fc.hpp"
#include "router.hpp"

namespace fc
{

std::string Router::NormalizePath(const std::string_view &input)
{
  bool prevSlash = false;
  std::string output;
  for (auto chr : input)
  {
    if (('/' == chr && prevSlash) || std::isspace(chr))
    {
      continue;
    }
    prevSlash = chr == '/' ? true : false;
    output.push_back(chr);
  }
  return output;
}

std::vector<std::string> Router::FragmentPath(const std::string_view &path)
{
  std::vector<std::string> frags;
  auto normalPath = NormalizePath(path);
  char *frag = std::strtok((char *)normalPath.c_str(), "/");
  while (frag)
  {
    frags.push_back(frag);
    frag = std::strtok(nullptr, "/");
  }
  return frags;
}

void Router::AddRoute(Method method, const std::string path, PathHandler handler)
{
  auto pathFragments = FragmentPath(path);
  Frag *current = &m_Root;

  for (const auto &frag : pathFragments)
  {
    FragType type;
    switch (frag.at(0))
    {
    case '*': type = FragType::WILDCARD; break;
    case ':': type = FragType::DYNAMIC; break;
    default: type = FragType::STATIC; break;
    }

    bool found = false;
    Frag *prev = nullptr;
    Frag *child = current->m_Child;
    while (child)
    {
      if ((type == FragType::STATIC && child->m_Label == frag) || (type != FragType::STATIC && child->m_Type == type))
      {
        if (type == FragType::DYNAMIC && child->m_Label != frag)
        {
          throw std::runtime_error("Conflicting dynamic segment names: " + child->m_Label + " vs " + frag);
        }
        found = true;
        break;
      }
      prev = child;
      child = child->m_Next;
    }
    if (!found)
    {
      Frag *newFrag = new Frag(type, FragType::DYNAMIC == type ? frag.substr(1) : frag);
      if (prev)
      {
        prev->m_Next = newFrag;
      }
      else
      {
        current->m_Child = newFrag;
      }
      child = newFrag;
    }
    current = child;
  }
  if (current->m_Handlers[static_cast<int>(method)] != nullptr)
  {
    throw std::runtime_error("Duplicate route for method " + std::to_string(static_cast<int>(method)) + " at path: " + path);
  }
  current->m_Handlers[static_cast<int>(method)] = handler;
}

PathHandler Router::MatchRoute(Method method, const std::string_view &path, Req &req) const
{
  auto fragments = FragmentPath(path);
  const Frag *current = &m_Root;
  for (auto &frag : fragments)
  {
    bool found = false;
    const Frag *child = current->m_Child;
    while (child && !found)
    {
      switch (child->m_Type)
      {
      case FragType::STATIC: found = frag == child->m_Label; break;
      case FragType::DYNAMIC:
        found = true;
        req.m_Params[child->m_Label] = frag;
        break;
      case FragType::WILDCARD: return child->m_Handlers[static_cast<int>(method)];
      }
      if (found)
      {
        current = child;
        break;
      }
      child = child->m_Next;
    }
    if (!found)
    {
      return nullptr;
    }
  }
  return current->m_Handlers.at((int)method);
}

} // namespace fc
