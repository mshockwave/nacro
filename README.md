# nacro
A better macro extension for C/C++. Implemented in Clang plugins. Inspired by Rust's macro system.
```cxx
#pragma nacro rule myPrint
(val:$expr) -> {
    printf("%d\n", val * 2);
}

#pragma nacro rule genColors
(cases:$expr*) -> {
    $loop(c in cases) {
        case c:
        printf("the color is %s\n", $str(c));
        break;
    }
}

enum Color { RED, BLUE, YELLOW };
void printColor(Color color) {
    switch(color) genColors(RED, BLUE, YELLOW)
}

int main() {
    myPrint(1 + 3) // print out '8'
    return 0;
}
```

## Motivations
The C/C++ macro system adopts a copy-and-paste style text replacement that leads to many problems. For example, values might unintentionally be mixed up with adjacent operands in an expression.
```cxx
#define doubleUp(x) x * 2

// ERROR: foo(2) will print out 8 rather than 10
void foo(int a) {
    printf("%d\n", doubleUp(a + 3));
}
```
Of course this can be solved by adding parentheses around every possible expressions but it will make code less readable and it's still pretty easy for programmers to make mistakes.

Another common problem is 'variable capture':
```cxx
#define foo(arg) {\
    int x = 0;  \
    return arg + x; \
}

// ERROR: caller will always return 0 
// regardless the argument value
int caller(int x) {
    foo(x)
}
```
The root cause is that substitution of `arg`, which is happens to be `x`, will be bind to the declaration _within_ the macro (i.e. `int x = 0;`) rather than the one in `caller`'s argument list.

Last but not the least, C/C++ macro system lacks a clean way to express repeatance text generation or substitution (i.e. loops). Making it less powerful than its counterpart in other languages (e.g. Rust).

## Syntax
_TBA_
