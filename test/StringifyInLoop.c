// RUN: %clang -fsyntax-only -Xclang -load -Xclang %NacroPlugin %s
// RUN: %clang -o %t -Xclang -load -Xclang %NacroPlugin %s
// RUN: %t | %FileCheck %s
#include <stdio.h>

#pragma nacro rule foo
(a:$ident*) -> {
  $loop(i in a) {
    printf("hello %s\n", $str(i));
  }
}

int main() {
  foo(a,b,c,d)
  // CHECK: hello a
  // CHECK-NEXT: hello b
  // CHECK-NEXT: hello c
  // CHECK-NEXT: hello d
  return 0;
}
