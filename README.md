# nacro
![Build](https://github.com/mshockwave/nacro/workflows/Release%20Build%20with%20Prebuilt%20LLVM/Clang%2010.0.0/badge.svg)

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
void printColor(enum Color color) {
    switch(color) genColors(RED, BLUE, YELLOW)
}

int main() {
    myPrint(1 + 3) // print out '8'
    return 0;
}
```

## Table of Contents
 - [How to Build and Use](#how-to-build-and-use)
 - [Motivations](#motivations)
 - [Features and Syntax](#features-and-syntax)

## How to Build and Use
### Prepare
Here are the tool and version requirements:
|   Tool Name  | Version Requirements |                               Notes                               |
|:------------:|:--------------------:|:-----------------------------------------------------------------:|
|     CMake    |        >=3.4.4       |                                                                   |
|      GCC     |        >=5.1.0       |                     Used to compile the plugin                    |
| LLVM + Clang |        10.0.0        | Used to build and run the plugin. **Need to be exactly this version** |
|    Python    |         >=3.0        |                 Only required if end-to-end tests are enabled                |
|  Google Test [[1]](#notes) |          Any         |              Only required if unit tests are enabled              |
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

### Usages
To run syntax-only check with the plugin:
```
clang -fsyntax-only \
      -Xclang -load -Xclang /path/to/NacroPlugin.so \
      input.c
```
To generate executable:
```
clang -Xclang -load -Xclang /path/to/NacroPlugin.so \
      input.c -o exe_out
```
Note that due to ABI compatibility issue, the `clang` above need to be **exactly the one from LLVM/Clang tree you used to build the plugin**.

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

## Features and Syntax
### Macro Argument Types
_TBA_

### Argument Protection
_TBA_

### Loop
_TBA_

### Incorrect Capture Detection
_TBA_

## Notes
[1]: Unfortunately pre-built google test is not widely available in mainstream Linux distributions. If that's the case, please follow the instructions from [here](https://github.com/google/googletest). And make sure it installed in CMake's default search path. That is, `/usr/lib` or `/usr/local/lib` in most cases.