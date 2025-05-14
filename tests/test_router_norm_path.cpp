#include <gtest/gtest.h>
#include <string_view>

#include "src/router.hpp"

TEST(RouterTest, NormalizePath) {
  char dirty_path[] = "///path /   // to//   ////some////where//  //  /";
  char expected[] = "/path/to/some/where";
  std::string_view clean_path = fc::root_router::norm_path(dirty_path);
  const_cast<char *>(clean_path.data())[clean_path.length()] = '\0';
  EXPECT_STREQ(expected, clean_path.data());
}
