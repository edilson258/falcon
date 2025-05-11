#include <gtest/gtest.h>
#include <string_view>
#include <vector>

#include "src/router.hpp"

TEST(RouterTest, SplitPath) {
  char path[] = "/store/home/clients/:id";
  std::vector<std::string_view> expected = {"store", "home", "clients", ":id"};

  std::vector<std::string_view> frags = fc::root_router::split_path(path);

  ASSERT_EQ(frags.size(), expected.size());

  for (int i = 0; i < frags.size(); ++i) {
    ASSERT_EQ(frags[i], expected[i]);
  }
}
