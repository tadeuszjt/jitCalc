A simple imperative language with indentation-specific syntax, functions, control-flow and jit-based compilation using C++, LLVM, Bison and CMake.

Building LLVM for this project requires RTTI for the cl::opt library and -j1 during install to prevent memory running out on WSL.
```
cmake -S llvm -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DLLVM_ENABLE_PROJECTS="clang;lldb;lld" -DLLVM_ENABLE_RTTI=ON
ninja -C build -j1 install
```

Built using LLVM 19
```
mkdir build
cd build
cmake ..
make && ./jitCalc
```

Eg.
```
> fn fib(n)
>   if n < 2
>     return 1
>   return fib(n - 2) + fib(n - 1)
> ;
>
> fib(10)
> ;
> 55
> q
```
