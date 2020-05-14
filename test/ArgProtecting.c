// RUN: %clang -Xclang -load -Xclang %NacroPlugin %s -o %t
// RUN: %t | %FileCheck %s
#include <stdio.h>

#pragma nacro rule bar
(a:$expr, b:$expr) -> {
  puts(a);
  printf("%d\n", b * 5);
}

// CHECK: hello
// CHECK: 15
int main() {
  bar("hello", 1 + 2);
  return 0;
}
