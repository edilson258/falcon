#include "falcon/http.h"
#include <stdio.h>
#include <string.h>

#include <unity.h>

#include <falcon.h>
#include <falcon/errn.h>
#include <falcon/router.h>

void setUp(void)
{
  // set stuff up here
}

void tearDown(void)
{
  // clean stuff up here
}

void test_add_route_to_router()
{
  fc_router_t router;
  fc__router_init(&router);

  fc__router_add_route(&router, FC_HTTP_GET, strndup("/users", 6), NULL, NULL);

  TEST_ASSERT_EQUAL_STRING("users", router.root->children->label);
}

void test_match_route()
{
  fc_router_t router;
  fc__router_init(&router);

  fc__router_add_route(&router, FC_HTTP_GET, strndup("/profile", 8), NULL, NULL);

  char *path = strndup("/profile", 8);
  fc_request_t req;
  req.method = FC_HTTP_GET;
  fc__route_handler *h = NULL;
  fc_errno err = fc__router_match_req(&router, &req, path, &h);

  TEST_ASSERT_EQUAL_INT(err, FC_ERR_OK);
  TEST_ASSERT_TRUE(h);
}

void test_match_route_not_found()
{
  fc_router_t router;
  fc__router_init(&router);

  fc__router_add_route(&router, FC_HTTP_GET, strndup("/products", 9), NULL, NULL);

  char *path = strndup("/ping", 5);
  fc_request_t req;
  req.method = FC_HTTP_GET;
  fc__route_handler *h = NULL;
  fc_errno err = fc__router_match_req(&router, &req, path, &h);

  TEST_ASSERT_EQUAL_INT(err, FC_ERR_ENTRY_NOT_FOUND);
  TEST_ASSERT_FALSE(h);
}

void test_match_route_not_found_with_different_method()
{
  fc_router_t router;
  fc__router_init(&router);

  // register route with GET method
  fc__router_add_route(&router, FC_HTTP_GET, strndup("/profile", 8), NULL, NULL);

  char *path = strndup("/profile", 8);
  fc_request_t req;

  // match with POST method
  req.method = FC_HTTP_POST;
  fc__route_handler *h = NULL;
  fc_errno err = fc__router_match_req(&router, &req, path, &h);

  TEST_ASSERT_EQUAL_INT(err, FC_ERR_ENTRY_NOT_FOUND);
  TEST_ASSERT_FALSE(h);
}

int main()
{
  UNITY_BEGIN();
  RUN_TEST(test_add_route_to_router);
  RUN_TEST(test_match_route);
  RUN_TEST(test_match_route_not_found);
  RUN_TEST(test_match_route_not_found_with_different_method);
  return UNITY_END();
}
