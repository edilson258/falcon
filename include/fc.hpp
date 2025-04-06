#pragma once

#include <functional>
#include <string>
#include <string_view>

namespace fc
{

enum class Method
{
  GET = 1,
  POST = 2,
  PUT = 3,
  DELETE = 4,
  PATCH = 5,

  // used internally to keep track of methods len
  COUNT = 5,
};

struct Req
{
public:
  explicit Req() = delete;

  const Method GetMethod() const { return m_Method; }
  const std::string_view &GetRaw() const { return m_Raw; };
  const std::string_view &GetPath() const { return m_Path; }
  const void *GetRemoteSock() const { return m_UVRemoteSock; }
  const std::string *GetParam(const std::string &key) const;

private:
  Method m_Method;
  void *m_UVRemoteSock;
  std::string_view m_Raw;
  std::string_view m_Path;
  std::string_view m_RawBody;
  std::unordered_map<std::string, std::string> m_Params;

  Req(void *remote, std::string_view raw) : m_UVRemoteSock(remote), m_Raw(raw) {};

  friend struct Router;
  friend struct HttpParser;
  friend Req RequestFactory(void *remote, std::string_view raw);
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
  void Post(const std::string, PathHandler);
  void Put(const std::string, PathHandler);
  void Delete(const std::string, PathHandler);
  void Patch(const std::string, PathHandler);
  void Options(const std::string, PathHandler);
  void Head(const std::string, PathHandler);
  void Trace(const std::string, PathHandler);
  void Connect(const std::string, PathHandler);

  int Listen(const std::string, std::function<void()>);

private:
  struct impl;
  impl *m_PImpl;

  friend void ParseHttpRequest(Req);
  friend struct impl;
};
} // namespace fc
