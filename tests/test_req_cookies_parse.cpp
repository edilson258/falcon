#include <gtest/gtest.h>
#include <string_view>

#include "src/req.hpp"

TEST(RequestTest, CookiesParser) {
  char raw_cookies[] = "Theme = Dark Mode; Language=Pt-Br";
  size_t expected_cookies_len = 2;

  fc::cookies cookies;
  cookies.parse(raw_cookies);

  ASSERT_TRUE(cookies.m_parsed);
  ASSERT_EQ(cookies.m_cookies.size(), expected_cookies_len);
  ASSERT_EQ("Dark Mode", cookies.get("Theme"));
  ASSERT_EQ("Pt-Br", cookies.get("Language"));
}

TEST(RequestTest, CookiesParser_MalFormedInput) {
  char raw_cookies[] = "name=edilson258  ; Language=";
  size_t expected_cookies_len = 1; // 1 because the last is malformed and should be ignored

  fc::cookies cookies;
  cookies.parse(raw_cookies);

  ASSERT_TRUE(cookies.m_parsed);
  ASSERT_EQ(cookies.m_cookies.size(), expected_cookies_len);
  ASSERT_EQ("edilson258", cookies.get("name"));
}
