// RUN: %clang -fsyntax-only -Xclang -load -Xclang %NacroPlugin %s
// RUN: %clang -O1 -emit-llvm -S -Xclang -load -Xclang %NacroPlugin %s -o - | %FileCheck %s

#pragma nacro rule add
(list:$expr*) -> {
  $loop(i in list) {
    bar(i);
  }
}

void bar(int i);

void foo() {
  // CHECK: call void @bar(i32 1)
  // CHECK: call void @bar(i32 2)
  // CHECK: call void @bar(i32 3)
  // CHECK: call void @bar(i32 4)
  add(1,2,3,4)
}
