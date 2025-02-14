#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <unity.h>

#include <falcon.h>
#include <falcon/errn.h>
#include <falcon/http.h>
#include <falcon/router.h>

void setUp(void)
{
  // set stuff up here
}

void tearDown(void)
{
  // clean stuff up here
}

void test_normalize_path()
{
  char *path = strdup("////products///:id/////");
  fc_errno err = fc__normalize_path_inplace(&path);
  TEST_ASSERT_EQUAL_INT(FC_ERR_OK, err);
  TEST_ASSERT_EQUAL_STRING("/products/:id", path);
}

void test_split_path()
{
  char *path = strdup("/users/:id");
  size_t nfragments = 0;
  char *fragments[PATH_MAX_FRAGS];
  fc_errno err = fc__split_path(path, &nfragments, fragments);

  TEST_ASSERT_EQUAL_INT(2, nfragments);
  TEST_ASSERT_EQUAL_STRING("users", fragments[0]);
  TEST_ASSERT_EQUAL_STRING(":id", fragments[1]);
}

void test_add_route_to_router()
{
  fc_router_t router;
  fc__router_init(&router);

  fc__router_add_route(&router, FC_HTTP_GET, strdup("/users"), NULL, NULL);

  TEST_ASSERT_EQUAL_STRING("users", router.root->children->label);
}

void test_add_route_to_router_with_collection()
{
  fc_router_t router;
  fc__router_init(&router);

  fc_errno err = fc__router_add_route(&router, FC_HTTP_GET, strdup("/users"), NULL, NULL);
  TEST_ASSERT_EQUAL_INT(FC_ERR_OK, err);
  TEST_ASSERT_EQUAL_STRING("users", router.root->children->label);

  err = fc__router_add_route(&router, FC_HTTP_GET, strdup("/users"), NULL, NULL);
  TEST_ASSERT_EQUAL_INT(FC_ERR_ROUTE_CONFLIT, err);
}

void test_match_route()
{
  fc_router_t router;
  fc__router_init(&router);

  fc__router_add_route(&router, FC_HTTP_GET, strdup("/profile"), NULL, NULL);

  char *path = strdup("/profile");
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

  fc__router_add_route(&router, FC_HTTP_GET, strdup("/products"), NULL, NULL);

  char *path = strdup("/ping");
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
  fc__router_add_route(&router, FC_HTTP_GET, strdup("/profile"), NULL, NULL);

  char *path = strdup("/profile");
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
  RUN_TEST(test_normalize_path);
  RUN_TEST(test_split_path);
  RUN_TEST(test_add_route_to_router);
  RUN_TEST(test_add_route_to_router_with_collection);
  RUN_TEST(test_match_route);
  RUN_TEST(test_match_route_not_found);
  RUN_TEST(test_match_route_not_found_with_different_method);
  return UNITY_END();
}
