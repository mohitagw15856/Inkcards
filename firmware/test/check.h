// Tiny dependency-free test harness. Keeping the host tests free of gtest/Unity
// means CI needs no network fetch and no extra toolchain, just a C++17 compiler.
#pragma once

#include <cstdio>
#include <string>

namespace testing {

inline int& failures() {
  static int f = 0;
  return f;
}
inline int& checks() {
  static int c = 0;
  return c;
}

inline void report(bool ok, const char* expr, const char* file, int line) {
  ++checks();
  if (!ok) {
    ++failures();
    std::printf("  FAIL: %s  (%s:%d)\n", expr, file, line);
  }
}

}  // namespace testing

#define CHECK(cond) ::testing::report((cond), #cond, __FILE__, __LINE__)
#define CHECK_EQ(a, b) ::testing::report((a) == (b), #a " == " #b, __FILE__, __LINE__)

#define RUN(fn)                        \
  do {                                 \
    std::printf("- %s\n", #fn);        \
    fn();                              \
  } while (0)
