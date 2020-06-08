// RUN: %clang -fsyntax-only -Wall -Xclang -load -Xclang %NacroPlugin -Xclang -verify %s

#pragma nacro rule foo
(a:$expr) -> {
  int x = 0; // expected-note {{is bind to declaration within a nacro}}
  return a + x;
}

int foo_caller(int x) {
  foo(x) // expected-error{{a potential declaration leak detected}} expected-note{{the reference to 'x' that comes from outside a nacro}}
}
