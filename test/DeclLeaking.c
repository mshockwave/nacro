#pragma nacro rule foo
(a:$expr) -> {
  int x = 0;
  return a + x;
}

int foo_caller(int x) {
  foo(x)
}
