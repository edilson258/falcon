#include <unity.h>

void setUp(void)
{
  // set stuff up here
}

void tearDown(void)
{
  // clean stuff up here
}

int add(int a, int b)
{
  return a + b;
}

void test_add_two_numbers()
{
  int result = add(2, 3);
  TEST_ASSERT_EQUAL_INT(5, result);
}

int main()
{
  UNITY_BEGIN();
  RUN_TEST(test_add_two_numbers);
  return UNITY_END();
}
