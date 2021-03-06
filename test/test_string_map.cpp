// Copyright 2017 Open Source Robotics Foundation, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <gtest/gtest.h>

#include <string>

#include "./allocator_testing_utils.h"
#include "rcutils/allocator.h"
#include "rcutils/error_handling.h"
#include "rcutils/types/string_map.h"

TEST(test_string_map, lifecycle) {
  auto allocator = rcutils_get_default_allocator();
  auto failing_allocator = get_failing_allocator();
  rcutils_ret_t ret;

  // fini a zero initialized string_map
  {
    rcutils_string_map_t string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_fini(&string_map);
    EXPECT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();
  }

  // init and then fini and then fini again
  {
    rcutils_string_map_t string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&string_map, 10, allocator);
    EXPECT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();
    ret = rcutils_string_map_fini(&string_map);
    EXPECT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();
    ret = rcutils_string_map_fini(&string_map);
    EXPECT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();
  }

  // init and then fini with 0 capacity
  {
    rcutils_string_map_t string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&string_map, 0, allocator);
    EXPECT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();
    ret = rcutils_string_map_fini(&string_map);
    EXPECT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();
  }

  // init on non-zero initialized
  {
    rcutils_string_map_t string_map;
    // dirty the memory, otherwise this is flaky (sometimes the junk memory is null)
    memset(&string_map, 0x7, sizeof(rcutils_string_map_t));
    ret = rcutils_string_map_init(&string_map, 10, allocator);
    EXPECT_EQ(RCUTILS_RET_STRING_MAP_ALREADY_INIT, ret) << rcutils_get_error_string_safe();
    rcutils_reset_error();
  }

  // double init
  {
    rcutils_string_map_t string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&string_map, 10, allocator);
    EXPECT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();
    ret = rcutils_string_map_init(&string_map, 10, allocator);
    EXPECT_EQ(RCUTILS_RET_STRING_MAP_ALREADY_INIT, ret) << rcutils_get_error_string_safe();
    rcutils_reset_error();
    ret = rcutils_string_map_fini(&string_map);
    EXPECT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();
  }

  // null for string map pointer to init
  {
    ret = rcutils_string_map_init(NULL, 10, allocator);
    EXPECT_EQ(RCUTILS_RET_INVALID_ARGUMENT, ret) << rcutils_get_error_string_safe();
    rcutils_reset_error();
  }

  // failing allocator to init
  {
    rcutils_string_map_t string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&string_map, 10, failing_allocator);
    EXPECT_EQ(RCUTILS_RET_BAD_ALLOC, ret) << rcutils_get_error_string_safe();
    rcutils_reset_error();
  }

  // null for string map to fini
  {
    ret = rcutils_string_map_fini(NULL);
    EXPECT_EQ(RCUTILS_RET_INVALID_ARGUMENT, ret) << rcutils_get_error_string_safe();
    rcutils_reset_error();
  }
}

TEST(test_string_map, getters) {
  auto allocator = rcutils_get_default_allocator();
  rcutils_ret_t ret;

  // null for string_map
  {
    size_t capacity, size;
    ret = rcutils_string_map_get_capacity(NULL, &capacity);
    EXPECT_EQ(RCUTILS_RET_INVALID_ARGUMENT, ret) << rcutils_get_error_string_safe();
    rcutils_reset_error();
    ret = rcutils_string_map_get_size(NULL, &size);
    EXPECT_EQ(RCUTILS_RET_INVALID_ARGUMENT, ret) << rcutils_get_error_string_safe();
    rcutils_reset_error();
  }

  // null for capacity/size
  {
    rcutils_string_map_t string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&string_map, 0, allocator);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    ret = rcutils_string_map_get_capacity(&string_map, NULL);
    EXPECT_EQ(RCUTILS_RET_INVALID_ARGUMENT, ret) << rcutils_get_error_string_safe();
    rcutils_reset_error();
    ret = rcutils_string_map_get_size(&string_map, NULL);
    EXPECT_EQ(RCUTILS_RET_INVALID_ARGUMENT, ret) << rcutils_get_error_string_safe();
    rcutils_reset_error();
  }

  // initialize to 0
  {
    rcutils_string_map_t string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&string_map, 0, allocator);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    size_t expected = 0;
    size_t capacity = 42;
    ret = rcutils_string_map_get_capacity(&string_map, &capacity);
    EXPECT_EQ(RCUTILS_RET_OK, ret);
    EXPECT_EQ(expected, capacity);

    size_t size = 42;
    ret = rcutils_string_map_get_size(&string_map, &size);
    EXPECT_EQ(RCUTILS_RET_OK, ret);
    EXPECT_EQ(expected, size);

    ret = rcutils_string_map_fini(&string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);
  }
}

TEST(test_string_map, reserve_and_clear) {
  auto allocator = rcutils_get_default_allocator();
  rcutils_ret_t ret;

  // initialize to 10 (implicit reserve)
  {
    rcutils_string_map_t string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&string_map, 10, allocator);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    size_t expected = 10;
    size_t capacity = 42;
    ret = rcutils_string_map_get_capacity(&string_map, &capacity);
    EXPECT_EQ(RCUTILS_RET_OK, ret);
    EXPECT_EQ(expected, capacity);

    expected = 0;
    size_t size = 42;
    ret = rcutils_string_map_get_size(&string_map, &size);
    EXPECT_EQ(RCUTILS_RET_OK, ret);
    EXPECT_EQ(expected, size);

    ret = rcutils_string_map_fini(&string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);
  }

  // initialize to 0, reserve to 10
  {
    rcutils_string_map_t string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&string_map, 0, allocator);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    {
      size_t expected = 0;
      size_t capacity = 42;
      ret = rcutils_string_map_get_capacity(&string_map, &capacity);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, capacity);

      expected = 0;
      size_t size = 42;
      ret = rcutils_string_map_get_size(&string_map, &size);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, size);
    }

    ret = rcutils_string_map_reserve(&string_map, 10);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    {
      size_t expected = 10;
      size_t capacity = 42;
      ret = rcutils_string_map_get_capacity(&string_map, &capacity);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, capacity);

      expected = 0;
      size_t size = 42;
      ret = rcutils_string_map_get_size(&string_map, &size);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, size);
    }

    ret = rcutils_string_map_fini(&string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);
  }

  // initialize to 10, set, set, clear, reserve 0
  {
    rcutils_string_map_t string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&string_map, 10, allocator);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    {
      size_t expected = 10;
      size_t capacity = 42;
      ret = rcutils_string_map_get_capacity(&string_map, &capacity);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, capacity);

      expected = 0;
      size_t size = 42;
      ret = rcutils_string_map_get_size(&string_map, &size);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, size);
    }

    ret = rcutils_string_map_set_no_resize(&string_map, "key1", "value1");
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    {
      size_t expected = 10;
      size_t capacity = 42;
      ret = rcutils_string_map_get_capacity(&string_map, &capacity);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, capacity);

      expected = 1;
      size_t size = 42;
      ret = rcutils_string_map_get_size(&string_map, &size);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, size);
    }

    ret = rcutils_string_map_set_no_resize(&string_map, "key2", "value2");
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    {
      size_t expected = 10;
      size_t capacity = 42;
      ret = rcutils_string_map_get_capacity(&string_map, &capacity);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, capacity);

      expected = 2;
      size_t size = 42;
      ret = rcutils_string_map_get_size(&string_map, &size);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, size);
    }

    ret = rcutils_string_map_clear(&string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    {
      size_t expected = 10;
      size_t capacity = 42;
      ret = rcutils_string_map_get_capacity(&string_map, &capacity);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, capacity);

      expected = 0;
      size_t size = 42;
      ret = rcutils_string_map_get_size(&string_map, &size);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, size);
    }

    ret = rcutils_string_map_reserve(&string_map, 0);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    {
      size_t expected = 0;
      size_t capacity = 42;
      ret = rcutils_string_map_get_capacity(&string_map, &capacity);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, capacity);

      expected = 0;
      size_t size = 42;
      ret = rcutils_string_map_get_size(&string_map, &size);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, size);
    }

    ret = rcutils_string_map_fini(&string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);
  }

  // initialize to 0, clear
  {
    rcutils_string_map_t string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&string_map, 0, allocator);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    {
      size_t expected = 0;
      size_t capacity = 42;
      ret = rcutils_string_map_get_capacity(&string_map, &capacity);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, capacity);

      expected = 0;
      size_t size = 42;
      ret = rcutils_string_map_get_size(&string_map, &size);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, size);
    }

    ret = rcutils_string_map_clear(&string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    {
      size_t expected = 0;
      size_t capacity = 42;
      ret = rcutils_string_map_get_capacity(&string_map, &capacity);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, capacity);

      expected = 0;
      size_t size = 42;
      ret = rcutils_string_map_get_size(&string_map, &size);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, size);
    }

    ret = rcutils_string_map_fini(&string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);
  }

  // initialize to 0, reserve 10, clear
  {
    rcutils_string_map_t string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&string_map, 0, allocator);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    {
      size_t expected = 0;
      size_t capacity = 42;
      ret = rcutils_string_map_get_capacity(&string_map, &capacity);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, capacity);

      expected = 0;
      size_t size = 42;
      ret = rcutils_string_map_get_size(&string_map, &size);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, size);
    }

    ret = rcutils_string_map_reserve(&string_map, 10);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    {
      size_t expected = 10;
      size_t capacity = 42;
      ret = rcutils_string_map_get_capacity(&string_map, &capacity);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, capacity);

      expected = 0;
      size_t size = 42;
      ret = rcutils_string_map_get_size(&string_map, &size);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, size);
    }

    ret = rcutils_string_map_clear(&string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    {
      size_t expected = 10;
      size_t capacity = 42;
      ret = rcutils_string_map_get_capacity(&string_map, &capacity);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, capacity);

      expected = 0;
      size_t size = 42;
      ret = rcutils_string_map_get_size(&string_map, &size);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, size);
    }

    ret = rcutils_string_map_fini(&string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);
  }

  // null for string_map to reserve
  {
    ret = rcutils_string_map_reserve(NULL, 42);
    EXPECT_EQ(RCUTILS_RET_INVALID_ARGUMENT, ret) << rcutils_get_error_string_safe();
    rcutils_reset_error();
  }

  // null for string_map to clear
  {
    ret = rcutils_string_map_clear(NULL);
    EXPECT_EQ(RCUTILS_RET_INVALID_ARGUMENT, ret) << rcutils_get_error_string_safe();
    rcutils_reset_error();
  }
}

TEST(test_string_map, set_no_resize) {
  auto allocator = rcutils_get_default_allocator();
  auto failing_allocator = get_failing_allocator();
  rcutils_ret_t ret;

  // initialize to 1, set key1, set key2 (should fail)
  {
    rcutils_string_map_t string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&string_map, 1, allocator);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    {
      size_t expected = 1;
      size_t capacity = 42;
      ret = rcutils_string_map_get_capacity(&string_map, &capacity);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, capacity);

      expected = 0;
      size_t size = 42;
      ret = rcutils_string_map_get_size(&string_map, &size);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, size);
    }

    ret = rcutils_string_map_set_no_resize(&string_map, "key1", "value1");
    ASSERT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();

    {
      size_t expected = 1;
      size_t capacity = 42;
      ret = rcutils_string_map_get_capacity(&string_map, &capacity);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, capacity);

      expected = 1;
      size_t size = 42;
      ret = rcutils_string_map_get_size(&string_map, &size);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, size);

      EXPECT_STREQ("value1", rcutils_string_map_get(&string_map, "key1"));
    }

    ret = rcutils_string_map_set_no_resize(&string_map, "key2", "value2");
    ASSERT_EQ(RCUTILS_RET_NOT_ENOUGH_SPACE, ret) << rcutils_get_error_string_safe();
    rcutils_reset_error();

    ret = rcutils_string_map_fini(&string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);
  }

  // initialize to 2, set key1, set key1 again, set key2
  {
    rcutils_string_map_t string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&string_map, 2, allocator);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    {
      size_t expected = 2;
      size_t capacity = 42;
      ret = rcutils_string_map_get_capacity(&string_map, &capacity);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, capacity);

      expected = 0;
      size_t size = 42;
      ret = rcutils_string_map_get_size(&string_map, &size);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, size);
    }

    ret = rcutils_string_map_set_no_resize(&string_map, "key1", "value1");
    ASSERT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();

    {
      size_t expected = 2;
      size_t capacity = 42;
      ret = rcutils_string_map_get_capacity(&string_map, &capacity);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, capacity);

      expected = 1;
      size_t size = 42;
      ret = rcutils_string_map_get_size(&string_map, &size);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, size);

      EXPECT_STREQ("value1", rcutils_string_map_get(&string_map, "key1"));
    }

    ret = rcutils_string_map_set_no_resize(&string_map, "key1", "val1");
    ASSERT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();

    {
      size_t expected = 2;
      size_t capacity = 42;
      ret = rcutils_string_map_get_capacity(&string_map, &capacity);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, capacity);

      expected = 1;
      size_t size = 42;
      ret = rcutils_string_map_get_size(&string_map, &size);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, size);

      EXPECT_STREQ("val1", rcutils_string_map_get(&string_map, "key1"));
    }

    ret = rcutils_string_map_set_no_resize(&string_map, "key2", "value2");
    ASSERT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();

    {
      size_t expected = 2;
      size_t capacity = 42;
      ret = rcutils_string_map_get_capacity(&string_map, &capacity);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, capacity);

      expected = 2;
      size_t size = 42;
      ret = rcutils_string_map_get_size(&string_map, &size);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, size);

      EXPECT_STREQ("value2", rcutils_string_map_get(&string_map, "key2"));
    }

    ret = rcutils_string_map_fini(&string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);
  }

  // use failing allocator
  {
    set_failing_allocator_is_failing(failing_allocator, false);
    rcutils_string_map_t string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&string_map, 1, failing_allocator);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    set_failing_allocator_is_failing(failing_allocator, true);
    ret = rcutils_string_map_set_no_resize(&string_map, "key1", "value1");
    EXPECT_EQ(RCUTILS_RET_BAD_ALLOC, ret) << rcutils_get_error_string_safe();
    rcutils_reset_error();

    ret = rcutils_string_map_fini(&string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);
  }

  // pass NULL for string_map
  {
    ret = rcutils_string_map_set_no_resize(NULL, "key1", "value1");
    EXPECT_EQ(RCUTILS_RET_INVALID_ARGUMENT, ret) << rcutils_get_error_string_safe();
    rcutils_reset_error();
  }

  // pass NULL for key
  {
    rcutils_string_map_t string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&string_map, 2, allocator);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    ret = rcutils_string_map_set_no_resize(&string_map, NULL, "value1");
    EXPECT_EQ(RCUTILS_RET_INVALID_ARGUMENT, ret) << rcutils_get_error_string_safe();
    rcutils_reset_error();

    ret = rcutils_string_map_fini(&string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);
  }

  // pass NULL for value
  {
    rcutils_string_map_t string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&string_map, 2, allocator);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    ret = rcutils_string_map_set_no_resize(&string_map, "key1", NULL);
    EXPECT_EQ(RCUTILS_RET_INVALID_ARGUMENT, ret) << rcutils_get_error_string_safe();
    rcutils_reset_error();

    ret = rcutils_string_map_fini(&string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);
  }
}

TEST(test_string_map, set) {
  auto allocator = rcutils_get_default_allocator();
  auto failing_allocator = get_failing_allocator();
  rcutils_ret_t ret;

  // initialize to 0, set key1
  {
    rcutils_string_map_t string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&string_map, 0, allocator);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    {
      size_t expected = 0;
      size_t capacity = 42;
      ret = rcutils_string_map_get_capacity(&string_map, &capacity);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, capacity);

      expected = 0;
      size_t size = 42;
      ret = rcutils_string_map_get_size(&string_map, &size);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, size);
    }

    ret = rcutils_string_map_set(&string_map, "key1", "value1");
    ASSERT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();

    {
      size_t expected = 1;
      size_t capacity = 42;
      ret = rcutils_string_map_get_capacity(&string_map, &capacity);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, capacity);

      expected = 1;
      size_t size = 42;
      ret = rcutils_string_map_get_size(&string_map, &size);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, size);

      EXPECT_STREQ("value1", rcutils_string_map_get(&string_map, "key1"));
    }

    ret = rcutils_string_map_fini(&string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);
  }

  // initialize to 1, set key1, set key2 (capacity -> 2), set key3 (capacity -> 4)
  {
    rcutils_string_map_t string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&string_map, 1, allocator);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    {
      size_t expected = 1;
      size_t capacity = 42;
      ret = rcutils_string_map_get_capacity(&string_map, &capacity);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, capacity);

      expected = 0;
      size_t size = 42;
      ret = rcutils_string_map_get_size(&string_map, &size);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, size);
    }

    ret = rcutils_string_map_set(&string_map, "key1", "value1");
    ASSERT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();

    {
      size_t expected = 1;
      size_t capacity = 42;
      ret = rcutils_string_map_get_capacity(&string_map, &capacity);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, capacity);

      expected = 1;
      size_t size = 42;
      ret = rcutils_string_map_get_size(&string_map, &size);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, size);

      EXPECT_STREQ("value1", rcutils_string_map_get(&string_map, "key1"));
    }

    ret = rcutils_string_map_set(&string_map, "key2", "value2");
    ASSERT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();

    {
      size_t expected = 2;
      size_t capacity = 42;
      ret = rcutils_string_map_get_capacity(&string_map, &capacity);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, capacity);

      expected = 2;
      size_t size = 42;
      ret = rcutils_string_map_get_size(&string_map, &size);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, size);

      EXPECT_STREQ("value2", rcutils_string_map_get(&string_map, "key2"));
    }

    ret = rcutils_string_map_set(&string_map, "key3", "value3");
    ASSERT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();

    {
      size_t expected = 4;
      size_t capacity = 42;
      ret = rcutils_string_map_get_capacity(&string_map, &capacity);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, capacity);

      expected = 3;
      size_t size = 42;
      ret = rcutils_string_map_get_size(&string_map, &size);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, size);

      EXPECT_STREQ("value3", rcutils_string_map_get(&string_map, "key3"));
    }

    ret = rcutils_string_map_fini(&string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);
  }

  // initialize to 2, set key1, set key1 again, set key2
  {
    rcutils_string_map_t string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&string_map, 2, allocator);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    {
      size_t expected = 2;
      size_t capacity = 42;
      ret = rcutils_string_map_get_capacity(&string_map, &capacity);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, capacity);

      expected = 0;
      size_t size = 42;
      ret = rcutils_string_map_get_size(&string_map, &size);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, size);
    }

    ret = rcutils_string_map_set(&string_map, "key1", "value1");
    ASSERT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();

    {
      size_t expected = 2;
      size_t capacity = 42;
      ret = rcutils_string_map_get_capacity(&string_map, &capacity);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, capacity);

      expected = 1;
      size_t size = 42;
      ret = rcutils_string_map_get_size(&string_map, &size);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, size);

      EXPECT_STREQ("value1", rcutils_string_map_get(&string_map, "key1"));
    }

    ret = rcutils_string_map_set(&string_map, "key1", "val1");
    ASSERT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();

    {
      size_t expected = 2;
      size_t capacity = 42;
      ret = rcutils_string_map_get_capacity(&string_map, &capacity);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, capacity);

      expected = 1;
      size_t size = 42;
      ret = rcutils_string_map_get_size(&string_map, &size);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, size);

      EXPECT_STREQ("val1", rcutils_string_map_get(&string_map, "key1"));
    }

    ret = rcutils_string_map_set(&string_map, "key2", "value2");
    ASSERT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();

    {
      size_t expected = 2;
      size_t capacity = 42;
      ret = rcutils_string_map_get_capacity(&string_map, &capacity);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, capacity);

      expected = 2;
      size_t size = 42;
      ret = rcutils_string_map_get_size(&string_map, &size);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, size);

      EXPECT_STREQ("value2", rcutils_string_map_get(&string_map, "key2"));
    }

    ret = rcutils_string_map_fini(&string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);
  }

  // use failing allocator
  {
    set_failing_allocator_is_failing(failing_allocator, false);
    rcutils_string_map_t string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&string_map, 1, failing_allocator);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    set_failing_allocator_is_failing(failing_allocator, true);
    ret = rcutils_string_map_set(&string_map, "key1", "value1");
    EXPECT_EQ(RCUTILS_RET_BAD_ALLOC, ret) << rcutils_get_error_string_safe();
    rcutils_reset_error();

    ret = rcutils_string_map_fini(&string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);
  }

  // pass NULL for string_map
  {
    ret = rcutils_string_map_set(NULL, "key1", "value1");
    EXPECT_EQ(RCUTILS_RET_INVALID_ARGUMENT, ret) << rcutils_get_error_string_safe();
    rcutils_reset_error();
  }

  // pass NULL for key
  {
    rcutils_string_map_t string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&string_map, 2, allocator);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    ret = rcutils_string_map_set(&string_map, NULL, "value1");
    EXPECT_EQ(RCUTILS_RET_INVALID_ARGUMENT, ret) << rcutils_get_error_string_safe();
    rcutils_reset_error();

    ret = rcutils_string_map_fini(&string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);
  }

  // pass NULL for value
  {
    rcutils_string_map_t string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&string_map, 2, allocator);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    ret = rcutils_string_map_set(&string_map, "key1", NULL);
    EXPECT_EQ(RCUTILS_RET_INVALID_ARGUMENT, ret) << rcutils_get_error_string_safe();
    rcutils_reset_error();

    ret = rcutils_string_map_fini(&string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);
  }
}

TEST(test_string_map, key_exists) {
  auto allocator = rcutils_get_default_allocator();
  rcutils_ret_t ret;
  bool key_exists;

  {
    rcutils_string_map_t string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&string_map, 2, allocator);
    ASSERT_EQ(RCUTILS_RET_OK, ret);
    key_exists = rcutils_string_map_key_exists(&string_map, "key1");
    EXPECT_FALSE(key_exists);
    key_exists = rcutils_string_map_key_exists(&string_map, "key2");
    EXPECT_FALSE(key_exists);

    ret = rcutils_string_map_set(&string_map, "key1", "value1");
    ASSERT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();
    key_exists = rcutils_string_map_key_exists(&string_map, "key1");
    EXPECT_TRUE(key_exists);
    key_exists = rcutils_string_map_key_exists(&string_map, "key2");
    EXPECT_FALSE(key_exists);

    ret = rcutils_string_map_unset(&string_map, "key1");
    ASSERT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();
    key_exists = rcutils_string_map_key_exists(&string_map, "key1");
    EXPECT_FALSE(key_exists);
    key_exists = rcutils_string_map_key_exists(&string_map, "key2");
    EXPECT_FALSE(key_exists);

    ret = rcutils_string_map_fini(&string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);
  }

  // key_exists with string_map as null
  {
    key_exists = rcutils_string_map_key_exists(NULL, "key");
    EXPECT_FALSE(key_exists);
  }

  // key_exists with key as null
  {
    rcutils_string_map_t string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&string_map, 2, allocator);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    key_exists = rcutils_string_map_key_exists(&string_map, NULL);
    EXPECT_FALSE(key_exists);

    ret = rcutils_string_map_fini(&string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);
  }

  // key_exists on empty map
  {
    rcutils_string_map_t string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&string_map, 0, allocator);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    key_exists = rcutils_string_map_key_exists(&string_map, "missing");
    EXPECT_FALSE(key_exists);

    ret = rcutils_string_map_fini(&string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);
  }
}

TEST(test_string_map, key_existsn) {
  auto allocator = rcutils_get_default_allocator();
  rcutils_ret_t ret;

  // key_existsn on normal key, which is longer than compared
  {
    rcutils_string_map_t string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&string_map, 2, allocator);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    ret = rcutils_string_map_set(&string_map, "key1", "value1");
    ASSERT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();
    ret = rcutils_string_map_set(&string_map, "key2", "value2");
    ASSERT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();

    EXPECT_TRUE(rcutils_string_map_key_existsn(&string_map, "key1andsome", 4));
    EXPECT_TRUE(rcutils_string_map_key_existsn(&string_map, "key2andsome", 4));
    EXPECT_FALSE(rcutils_string_map_key_existsn(&string_map, "key1andsome", 5));

    ret = rcutils_string_map_fini(&string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);
  }
}

TEST(test_string_map, unset) {
  auto allocator = rcutils_get_default_allocator();
  rcutils_ret_t ret;

  // initialize to 3, set key1, set key2, set key3, unset key2, set key3, set key2
  {
    rcutils_string_map_t string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&string_map, 3, allocator);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    {
      size_t expected = 3;
      size_t capacity = 42;
      ret = rcutils_string_map_get_capacity(&string_map, &capacity);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, capacity);

      expected = 0;
      size_t size = 42;
      ret = rcutils_string_map_get_size(&string_map, &size);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, size);
    }

    ret = rcutils_string_map_set_no_resize(&string_map, "key1", "value1");
    ASSERT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();
    ret = rcutils_string_map_set_no_resize(&string_map, "key2", "value2");
    ASSERT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();
    ret = rcutils_string_map_set_no_resize(&string_map, "key3", "value3");
    ASSERT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();

    {
      size_t expected = 3;
      size_t capacity = 42;
      ret = rcutils_string_map_get_capacity(&string_map, &capacity);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, capacity);

      expected = 3;
      size_t size = 42;
      ret = rcutils_string_map_get_size(&string_map, &size);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, size);

      EXPECT_STREQ("value1", rcutils_string_map_get(&string_map, "key1"));
      EXPECT_STREQ("value2", rcutils_string_map_get(&string_map, "key2"));
      EXPECT_STREQ("value3", rcutils_string_map_get(&string_map, "key3"));
    }

    ret = rcutils_string_map_unset(&string_map, "key2");
    ASSERT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();

    {
      size_t expected = 3;
      size_t capacity = 42;
      ret = rcutils_string_map_get_capacity(&string_map, &capacity);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, capacity);

      expected = 2;
      size_t size = 42;
      ret = rcutils_string_map_get_size(&string_map, &size);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, size);

      EXPECT_STREQ("value1", rcutils_string_map_get(&string_map, "key1"));
      EXPECT_STREQ("value3", rcutils_string_map_get(&string_map, "key3"));
    }

    ret = rcutils_string_map_set(&string_map, "key3", "value3.1");
    ASSERT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();

    {
      size_t expected = 3;
      size_t capacity = 42;
      ret = rcutils_string_map_get_capacity(&string_map, &capacity);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, capacity);

      expected = 2;
      size_t size = 42;
      ret = rcutils_string_map_get_size(&string_map, &size);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, size);

      EXPECT_STREQ("value1", rcutils_string_map_get(&string_map, "key1"));
      EXPECT_STREQ("value3.1", rcutils_string_map_get(&string_map, "key3"));
    }

    ret = rcutils_string_map_set(&string_map, "key2", "value2");
    ASSERT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();

    {
      size_t expected = 3;
      size_t capacity = 42;
      ret = rcutils_string_map_get_capacity(&string_map, &capacity);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, capacity);

      expected = 3;
      size_t size = 42;
      ret = rcutils_string_map_get_size(&string_map, &size);
      EXPECT_EQ(RCUTILS_RET_OK, ret);
      EXPECT_EQ(expected, size);

      EXPECT_STREQ("value1", rcutils_string_map_get(&string_map, "key1"));
      EXPECT_STREQ("value2", rcutils_string_map_get(&string_map, "key2"));
      EXPECT_STREQ("value3.1", rcutils_string_map_get(&string_map, "key3"));
    }

    ret = rcutils_string_map_fini(&string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);
  }

  // unset with string_map as null
  {
    ret = rcutils_string_map_unset(NULL, "key");
    EXPECT_EQ(RCUTILS_RET_INVALID_ARGUMENT, ret) << rcutils_get_error_string_safe();
    rcutils_reset_error();
  }

  // unset with key as null
  {
    rcutils_string_map_t string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&string_map, 10, allocator);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    ret = rcutils_string_map_unset(&string_map, NULL);
    EXPECT_EQ(RCUTILS_RET_INVALID_ARGUMENT, ret) << rcutils_get_error_string_safe();
    rcutils_reset_error();

    ret = rcutils_string_map_fini(&string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);
  }

  // unset on empty map
  {
    rcutils_string_map_t string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&string_map, 0, allocator);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    ret = rcutils_string_map_unset(&string_map, "missing");
    EXPECT_EQ(RCUTILS_RET_STRING_KEY_NOT_FOUND, ret) << rcutils_get_error_string_safe();
    rcutils_reset_error();

    ret = rcutils_string_map_fini(&string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);
  }

  // unset on missing key in non-empty map
  {
    rcutils_string_map_t string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&string_map, 2, allocator);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    ret = rcutils_string_map_set(&string_map, "key1", "value1");
    ASSERT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();
    ret = rcutils_string_map_set(&string_map, "key2", "value2");
    ASSERT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();

    ret = rcutils_string_map_unset(&string_map, "missing");
    EXPECT_EQ(RCUTILS_RET_STRING_KEY_NOT_FOUND, ret) << rcutils_get_error_string_safe();
    rcutils_reset_error();

    ret = rcutils_string_map_fini(&string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);
  }
}

TEST(test_string_map, get) {
  auto allocator = rcutils_get_default_allocator();
  rcutils_ret_t ret;

  // get normal key
  {
    rcutils_string_map_t string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&string_map, 2, allocator);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    ret = rcutils_string_map_set(&string_map, "key1", "value1");
    ASSERT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();
    ret = rcutils_string_map_set(&string_map, "key2", "value2");
    ASSERT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();

    EXPECT_STREQ("value1", rcutils_string_map_get(&string_map, "key1"));
    EXPECT_STREQ("value2", rcutils_string_map_get(&string_map, "key2"));

    ret = rcutils_string_map_fini(&string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);
  }

  // get missing key
  {
    rcutils_string_map_t string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&string_map, 2, allocator);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    ret = rcutils_string_map_set(&string_map, "key1", "value1");
    ASSERT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();

    EXPECT_EQ(NULL, rcutils_string_map_get(&string_map, "some_key"));

    ret = rcutils_string_map_fini(&string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);
  }

  // get on empty map (capacity but no pairs in it)
  {
    rcutils_string_map_t string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&string_map, 2, allocator);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    EXPECT_EQ(NULL, rcutils_string_map_get(&string_map, "some_key"));

    ret = rcutils_string_map_fini(&string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);
  }

  // get on map with no capacity (also empty)
  {
    rcutils_string_map_t string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&string_map, 0, allocator);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    EXPECT_EQ(NULL, rcutils_string_map_get(&string_map, "some_key"));

    ret = rcutils_string_map_fini(&string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);
  }

  // get with string_map null
  {
    EXPECT_EQ(NULL, rcutils_string_map_get(NULL, "some_key"));
  }

  // get with key null
  {
    rcutils_string_map_t string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&string_map, 2, allocator);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    ret = rcutils_string_map_set(&string_map, "key1", "value1");
    ASSERT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();

    EXPECT_EQ(NULL, rcutils_string_map_get(&string_map, NULL));

    ret = rcutils_string_map_fini(&string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);
  }
}

TEST(test_string_map, getn) {
  auto allocator = rcutils_get_default_allocator();
  rcutils_ret_t ret;

  // get normal key, which is longer than compared
  {
    rcutils_string_map_t string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&string_map, 2, allocator);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    ret = rcutils_string_map_set(&string_map, "key1", "value1");
    ASSERT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();
    ret = rcutils_string_map_set(&string_map, "key2", "value2");
    ASSERT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();

    EXPECT_STREQ("value1", rcutils_string_map_getn(&string_map, "key1andsome", 4));
    EXPECT_STREQ("value2", rcutils_string_map_getn(&string_map, "key2andsome", 4));

    ret = rcutils_string_map_fini(&string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);
  }

  // get missing key
  {
    rcutils_string_map_t string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&string_map, 2, allocator);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    ret = rcutils_string_map_set(&string_map, "key1", "value1");
    ASSERT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();

    EXPECT_EQ(NULL, rcutils_string_map_get(&string_map, "some_key"));

    ret = rcutils_string_map_fini(&string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);
  }

  // get on empty map (capacity but no pairs in it)
  {
    rcutils_string_map_t string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&string_map, 2, allocator);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    EXPECT_EQ(NULL, rcutils_string_map_get(&string_map, "some_key"));

    ret = rcutils_string_map_fini(&string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);
  }

  // get on map with no capacity (also empty)
  {
    rcutils_string_map_t string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&string_map, 0, allocator);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    EXPECT_EQ(NULL, rcutils_string_map_get(&string_map, "some_key"));

    ret = rcutils_string_map_fini(&string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);
  }

  // get with string_map null
  {
    EXPECT_EQ(NULL, rcutils_string_map_get(NULL, "some_key"));
  }

  // get with key null
  {
    rcutils_string_map_t string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&string_map, 2, allocator);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    ret = rcutils_string_map_set(&string_map, "key1", "value1");
    ASSERT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();

    EXPECT_EQ(NULL, rcutils_string_map_get(&string_map, NULL));

    ret = rcutils_string_map_fini(&string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);
  }
}

TEST(test_string_map, get_next_key) {
  auto allocator = rcutils_get_default_allocator();
  rcutils_ret_t ret;

  // iterate over a typical map
  {
    rcutils_string_map_t string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&string_map, 4, allocator);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    ret = rcutils_string_map_set(&string_map, "key1", "value1");
    ASSERT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();
    ret = rcutils_string_map_set(&string_map, "key2", "value2");
    ASSERT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();

    const char * first_key = rcutils_string_map_get_next_key(&string_map, NULL);
    EXPECT_STREQ("key1", first_key);
    const char * second_key = rcutils_string_map_get_next_key(&string_map, first_key);
    EXPECT_STREQ("key2", second_key);
    const char * last_key = rcutils_string_map_get_next_key(&string_map, second_key);
    EXPECT_EQ(NULL, last_key);

    ret = rcutils_string_map_fini(&string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);
  }

  // iterate over a full map
  {
    rcutils_string_map_t string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&string_map, 2, allocator);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    ret = rcutils_string_map_set(&string_map, "key1", "value1");
    ASSERT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();
    ret = rcutils_string_map_set(&string_map, "key2", "value2");
    ASSERT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();

    const char * first_key = rcutils_string_map_get_next_key(&string_map, NULL);
    EXPECT_STREQ("key1", first_key);
    const char * second_key = rcutils_string_map_get_next_key(&string_map, first_key);
    EXPECT_STREQ("key2", second_key);
    const char * last_key = rcutils_string_map_get_next_key(&string_map, second_key);
    EXPECT_EQ(NULL, last_key);

    ret = rcutils_string_map_fini(&string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);
  }

  // iterate over an empty map
  {
    rcutils_string_map_t string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&string_map, 0, allocator);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    const char * last_key = rcutils_string_map_get_next_key(&string_map, NULL);
    EXPECT_EQ(NULL, last_key);

    ret = rcutils_string_map_fini(&string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);
  }

  // iterate over a map with a gap in the keys
  {
    rcutils_string_map_t string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&string_map, 4, allocator);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    ret = rcutils_string_map_set(&string_map, "key1", "value1");
    ASSERT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();
    ret = rcutils_string_map_set(&string_map, "key2", "value2");
    ASSERT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();
    ret = rcutils_string_map_set(&string_map, "key3", "value3");
    ASSERT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();

    // removing key2 will create a gap in the map between key1 and key3
    ret = rcutils_string_map_unset(&string_map, "key2");
    ASSERT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();

    const char * first_key = rcutils_string_map_get_next_key(&string_map, NULL);
    EXPECT_STREQ("key1", first_key);
    const char * second_key = rcutils_string_map_get_next_key(&string_map, first_key);
    EXPECT_STREQ("key3", second_key);
    const char * last_key = rcutils_string_map_get_next_key(&string_map, second_key);
    EXPECT_EQ(NULL, last_key);

    ret = rcutils_string_map_fini(&string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);
  }
}

TEST(test_string_map, copy) {
  auto allocator = rcutils_get_default_allocator();
  rcutils_ret_t ret;

  // copy a typical map into an empty one
  {
    rcutils_string_map_t src_string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&src_string_map, 4, allocator);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    ret = rcutils_string_map_set(&src_string_map, "key1", "value1");
    ASSERT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();
    ret = rcutils_string_map_set(&src_string_map, "key2", "value2");
    ASSERT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();

    rcutils_string_map_t dst_string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&dst_string_map, 0, allocator);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    ret = rcutils_string_map_copy(&src_string_map, &dst_string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    EXPECT_STREQ("value1", rcutils_string_map_get(&dst_string_map, "key1"));
    EXPECT_STREQ("value2", rcutils_string_map_get(&dst_string_map, "key2"));

    ret = rcutils_string_map_fini(&src_string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);
    ret = rcutils_string_map_fini(&dst_string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);
  }

  // copy empty map into empty map
  {
    rcutils_string_map_t src_string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&src_string_map, 0, allocator);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    rcutils_string_map_t dst_string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&dst_string_map, 0, allocator);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    ret = rcutils_string_map_copy(&src_string_map, &dst_string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    ret = rcutils_string_map_fini(&src_string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);
    ret = rcutils_string_map_fini(&dst_string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);
  }

  // copy empty map into non-empty map
  {
    rcutils_string_map_t src_string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&src_string_map, 0, allocator);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    rcutils_string_map_t dst_string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&dst_string_map, 4, allocator);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    ret = rcutils_string_map_set(&dst_string_map, "key1", "value1");
    ASSERT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();
    ret = rcutils_string_map_set(&dst_string_map, "key2", "value2");
    ASSERT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();

    ret = rcutils_string_map_copy(&src_string_map, &dst_string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    EXPECT_STREQ("value1", rcutils_string_map_get(&dst_string_map, "key1"));
    EXPECT_STREQ("value2", rcutils_string_map_get(&dst_string_map, "key2"));

    ret = rcutils_string_map_fini(&src_string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);
    ret = rcutils_string_map_fini(&dst_string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);
  }

  // copy map into map with overlapping keys
  {
    rcutils_string_map_t src_string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&src_string_map, 0, allocator);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    ret = rcutils_string_map_set(&src_string_map, "key1", "value1");
    ASSERT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();
    ret = rcutils_string_map_set(&src_string_map, "key2", "value2");
    ASSERT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();

    rcutils_string_map_t dst_string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&dst_string_map, 4, allocator);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    ret = rcutils_string_map_set(&dst_string_map, "key2", "value2.1");
    ASSERT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();
    ret = rcutils_string_map_set(&dst_string_map, "key3", "value3");
    ASSERT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();

    ret = rcutils_string_map_copy(&src_string_map, &dst_string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    EXPECT_STREQ("value1", rcutils_string_map_get(&dst_string_map, "key1"));
    EXPECT_STREQ("value2", rcutils_string_map_get(&dst_string_map, "key2"));
    EXPECT_STREQ("value3", rcutils_string_map_get(&dst_string_map, "key3"));

    ret = rcutils_string_map_fini(&src_string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);
    ret = rcutils_string_map_fini(&dst_string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);
  }
}

TEST(test_string_map, strange_keys) {
  auto allocator = rcutils_get_default_allocator();
  rcutils_ret_t ret;

  // empty string key
  {
    rcutils_string_map_t string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&string_map, 2, allocator);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    ret = rcutils_string_map_set(&string_map, "", "value1");
    ASSERT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();

    EXPECT_STREQ("value1", rcutils_string_map_get(&string_map, ""));

    ret = rcutils_string_map_fini(&string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);
  }

  // key with spaces
  {
    rcutils_string_map_t string_map = rcutils_get_zero_initialized_string_map();
    ret = rcutils_string_map_init(&string_map, 2, allocator);
    ASSERT_EQ(RCUTILS_RET_OK, ret);

    ret = rcutils_string_map_set(&string_map, "key with spaces", "value1");
    ASSERT_EQ(RCUTILS_RET_OK, ret) << rcutils_get_error_string_safe();

    EXPECT_STREQ("value1", rcutils_string_map_get(&string_map, "key with spaces"));

    ret = rcutils_string_map_fini(&string_map);
    ASSERT_EQ(RCUTILS_RET_OK, ret);
  }
}
