# nacro
![Build Status](https://github.com/mshockwave/nacro/workflows/Release%20with%20LLVM%2010/badge.svg)

A better macro extension for C/C++. Implemented in Clang plugins. Inspired by Rust's macro system.
```cxx
#pragma nacro rule myPrint
(val:$expr) -> $stmt {
    printf("%d\n", val * 2)
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
void printColor(enum Color color) {
    switch(color) genColors(RED, BLUE, YELLOW)
}

int main() {
    myPrint(1 + 3)   // print out '8'
    printColor(RED); // print 'the color is RED'
    return 0;
}
```

**Note: This project is still under heavy development. Please checkout [this wiki page](https://github.com/mshockwave/nacro/wiki/Restrictions-and-WIP-Items) for work-in-progress items.**

## Table of Contents
 - [How to Build and Use](#how-to-build-and-use)
 - [Getting Started](#getting-started)
 - [Motivations](#motivations)
 - [Syntax and Features](#syntax-and-features)
 - [FAQ](#faq)

## How to Build and Use
### Supported Platforms
 - Linux x86_64
 - Mac OSX (partial support [[1]](#notes))

### Prepare
Here are the tool and version requirements:
|   Tool Name  | Version Requirements |                               Notes                               |
|:------------:|:--------------------:|:-----------------------------------------------------------------:|
|     CMake    |        >=3.4.4       |                                                                   |
|      GCC     |        >=5.1.0       |                     Used to compile the plugin                    |
| LLVM + Clang |        10.0.0        | Used to build and run the plugin. **Need to be exactly this version** |
|    Python    |         >=3.0        |                 Only required if end-to-end tests are enabled                |
|  Google Test [[2]](#notes) |          Any         |              Only required if unit tests are enabled              |
|     Ninja    |          Any         |        Optional. Feel free to use any build tool you prefer       |

Since LLVM project neither has a stable Clang plugin interface nor has ABI compatibilities (even among minor version releases!). The LLVM + Clang bundle need to be exactly the version as stated above. We recommend to [download the prebuilt one from offical website](https://releases.llvm.org/download.html). 

If you would like to enable end-to-end tests, several python packages are also required:
```
pip3 install lit filecheck
```

In addition to requirements above, you might also need to install `libtinfo` on Linux. It can be installed by `libtinfo-dev` package in Debian-family distros.

### Build
First, generate build folder using CMake:
```
mkdir .build
cd .build
cmake -G Ninja \
      -DLLVM_DIR=${LLVM_INSTALL_PATH}/lib/cmake/llvm \
      -DClang_DIR=${LLVM_INSTALL_PATH}/lib/cmake/clang \
      ../
```
The `${LLVM_INSTALL_PATH}` shell variable points to the pre-built bundle folder you download or your home-built LLVM build dir.

If you want to enable unit tests and/or end-to-end tests, use the following configurations:
```
mkdir .build
cd .build
cmake -G Ninja \
      -DLLVM_DIR=${LLVM_INSTALL_PATH}/lib/cmake/llvm \
      -DClang_DIR=${LLVM_INSTALL_PATH}/lib/cmake/clang \
      -DNACRO_ENABLE_UNITTESTS=ON \
      -DNACRO_ENABLE_TESTS=ON \
      ../
```

To build it:
```
ninja all
```
To run unit tests and end-to-end tests, respectively.
```
ninja check-units
ninja check
```

## Getting Started
As shown in the snippet at the top of this page, nacro allows you to embed a small DSL that acts like normal C/C++ function macros but with safer and more powerful features.

The DSL always starts with a pragma directive:
```cxx
#pragma nacro rule foo
```
The line above will create a nacro **rule**, which is the only supported nacro kinds, with the name _foo_.

Then we add the body for _foo_:
```cxx
#pragma nacro rule foo
(a:$expr) -> $expr {
    1 + a
}
```
The syntax here should be pretty straight forward: _a_ is the nacro argument, and the **\$expr** directive right after it shows that this argument is an expression. Outside the argument list, another **\$expr** following the arrow tell us that eventually the nacro body (i.e. `1 + a`) will be an expression after it is expanded.

```cxx
#pragma nacro rule foo
(a:$expr) -> $expr {
    1 + a
}

int caller(int x) {
    return foo(x << 2) * 3;
}
```
Using _foo_ is no different than using a normal C/C++ macro, as shown above. It will also got expanded during pre-processing time just like the old-fashion one. The only difference here is that for `caller`, instead of being expanded to this:
```cxx
int caller(int x) {
    return 1 + x << 2 * 3;
}
```
The nacro one will be expanded to this:
```cxx
int caller(int x) {
    return (1 + (x << 2)) * 3;
}
```
This is one of the protections that we will mentioned in the following chapters.

Another cool feature is loop:
```cxx
#pragma nacro rule bar
(items:$expr*) -> {
    $loop(i in items) {
        printf("hello %s\n", $str(i));
    }
}

void the_caller() {
    bar(some, random, stuff)
}
```
(Here we omit the "generated type" after the arrow, this will implicitly use `$block` as the generated type)

As you can guess, the `$str` directive will perform the stringify (just like the '#') and the `$loop` directive here will decompose `items`, which is a list of expression annotated by the '*' suffix on its type, and generate one copy of loop body for each of the element. Just like loop unrolling.
```cxx
// After bar got expanded...
void the_caller() {
    printf("hello %s\n", "some");
    printf("hello %s\n", "random");
    printf("hello %s\n", "stuff");
}
```

Finally, we're gonna compile the code. To use this plugin in an easier way without adding bunch of `-Xclang` options to clang, we've created a simple wrapper script, `clang-nacro`, to do all the heavy liftings for you. Just use it like a normal clang:
```
$ cat input.c
#include <stdio.h>

#pragma nacro rule bar
(items:$expr*) -> {
    $loop(i in items) {
        printf("hello %s\n", $str(i));
    }
}

int main() {
    bar(some, random, stuff)
    return 0;
}
$ /your/build/dir/clang-nacro -o out_exe input.c
$ ./out_exe
hello some
hello random
hello stuff
$
```

Of course, this is not the full story. Other features like [Invalid Capture Detection](https://github.com/mshockwave/nacro/wiki/Invalid-Capture-Detection) are waiting for you to explore in the [wiki](https://github.com/mshockwave/nacro/wiki)!

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

## Syntax and Features
For detail syntax and list of features, please checkout the [wiki pages](https://github.com/mshockwave/nacro/wiki/Nacro-Syntax).

## FAQ
**Q**: What does the name 'Nacro' come from?

**A**: It's a combination of "New Macro" or "Novel Macro" (whatever). And it was intended to make a pun on _'Necromancer'_. Since in LLVM project there has been _DWARF_ (the debug format) and _ORC_ (On-Request-Compilation JIT), why not adding another D&D reference?

## Notes
[1]: Actually most parts of the plugin can work without any problem in Mac OSX. Utilities and testing infrastructures are the parts that hasn't supported yet.

[2]: Unfortunately pre-built google test is not widely available in mainstream Linux distributions. If that's the case, please follow the instructions from [here](https://github.com/google/googletest). And make sure it installed in CMake's default search path. That is, `/usr/lib` or `/usr/local/lib` in most cases.
