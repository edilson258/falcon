#pragma once

#include <functional>
#include <string>
#include <string_view>

namespace fc
{

enum class Method
{
  GET = 1,
  POST,
  // used internally to know how many routes are, must be the last
  COUNT,
};

struct Req
{
public:
  explicit Req() = delete;

  const Method GetMethod() const { return m_Method; }
  const std::string_view &GetRaw() const { return m_Raw; };
  const std::string_view &GetPath() const { return m_Path; }
  const void *GetRemoteSock() const { return m_UVRemoteSock; }

private:
  Method m_Method;
  void *m_UVRemoteSock;
  std::string_view m_Raw;
  std::string_view m_Path;
  std::string_view m_RawBody;

  Req(void *remote, std::string_view raw) : m_UVRemoteSock(remote), m_Raw(raw) {};
  friend Req RequestFactory(void *remote, std::string_view raw);
  friend struct HttpParser;
};

struct Res
{
public:
  static Res Ok();

  const std::string &GetRaw() const { return m_Raw; }

private:
  std::string m_Raw;

  Res(std::string raw) : m_Raw(std::move(raw)) {}

  friend struct impl;
};

using PathHandler = std::function<Res(Req)>;

struct App
{
public:
  App();
  ~App();

  void Get(const std::string, PathHandler);
  int Listen(const std::string, std::function<void()>);

private:
  struct impl;
  impl *m_PImpl;

  friend void ParseHttpRequest(Req);
  friend struct impl;
};
} // namespace fc
