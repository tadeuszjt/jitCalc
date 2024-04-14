A simple imperative language with indentation-specific syntax, functions, control-flow and jit-based compilation using C++, LLVM, Bison and CMake.

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
