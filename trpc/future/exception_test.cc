//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 Tencent.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#include "trpc/future/exception.h"

#include <gtest/gtest.h>

namespace trpc {

// Subclass of ExceptionBase.
class ExceptionTestErrorMessage : public ExceptionBase {
 public:
  const char *what() const noexcept override { return "test_error_message"; }
};

// Subclass of ExceptionBase.
class ExceptionTestErrorMessageDerived : public ExceptionBase {
 public:
  const char *what() const noexcept override { return "test_error_message_derived"; }
};

// Subclass of ExceptionBase.
class ExceptionTestCore : public ExceptionBase {
 public:
  const char *what() const noexcept override { return "test_error_core"; }
};

// Subclass of ExceptionTestCore.
class ExceptionTestCore1 : public ExceptionTestCore {
 public:
  const char *what() const noexcept override { return "test_error_core_1"; }
};

// Subclass of ExceptionTestCore.
class ExceptionTestCore2 : public ExceptionTestCore {
 public:
  const char *what() const noexcept override { return "test_error_core_2"; }
};

// Subclass of ExceptionTestCore2.
class ExceptionTestCore3 : public ExceptionTestCore2 {
 public:
  const char *what() const noexcept override { return "test_error_core_3"; }
};

// Test What to get error message.
TEST(Exception, TestErrorMessage) {
  Exception e = ExceptionTestErrorMessage();
  EXPECT_TRUE(e.what() == std::string("test_error_message"));
  e = ExceptionTestErrorMessageDerived();
  EXPECT_TRUE(e.what() == std::string("test_error_message_derived"));
}

// Test common exception.
TEST(Exception, TestCommonException) {
  Exception e = CommonException("test msg");
  EXPECT_EQ(e.what(), std::string("test msg"));
}

// 1. Test exception itself.
// 2. Test parent exception.
// 3. Test derived exception.
// 4. Test brother exception.
TEST(Exception, TestExceptionTypeCheck) {
  {
    Exception e = ExceptionTestCore1();
    EXPECT_TRUE(e.is(ExceptionTestCore1()));
  }

  {
    Exception e = ExceptionTestCore2();
    EXPECT_TRUE(e.is(ExceptionTestCore()));
    e = ExceptionTestCore3();
    EXPECT_TRUE(e.is(ExceptionTestCore2()));
  }

  {
    Exception e = ExceptionTestCore();
    EXPECT_TRUE(!e.is(ExceptionTestCore3()));
  }

  {
    Exception e = ExceptionTestCore2();
    EXPECT_TRUE(!e.is(ExceptionTestCore1()));
    e = ExceptionTestCore1();
    EXPECT_TRUE(!e.is(ExceptionTestCore2()));
  }
}

}  // namespace trpc
