# Zero

Welcome to Zero, a sample project for Hero.

This project allows you to bootstrap your own C-- project with a convenient CMake build setup which will download required dependencies from Github.

# Building

This project depends on Hero, an object oriented C++ framework which entirely replaces the standard C++ library.

Hero will be built from source, its output will be placed in the CMake dependencies under the build folder.


```
git clone git@github.com:reasoning/zero.git ./
cd build
cmake ../ && make -j6
```

Hero builds from scratch in about 15 seconds on a Ryzen 9 using `-j6`, you may wish to tweak this to your optimum core count.

# No C++ Standard Library

By default this project will build without referencing any headers or linking with libstdc++.

The C++ standard library will be completely inaccessible.

It uses its own set of exports for new/delete and other symbols required for basic C++ error handling and behavior (like pure virtual calls, exception handlers, threadsafe statics, and some basic maths intrinsics)

The `ldd` tool should report the following after a build without libstdc++.

```
┌──(emerson㉿Flow)-[/mnt/c/Code/ZeroToHero/Source/git/gitlab/zero/build]
└─$ ldd zero
        linux-vdso.so.1 (0x00007ffce2df8000)
        libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00007ff5e4ad1000)
        /lib64/ld-linux-x86-64.so.2 (0x00007ff5e4de6000)
```
### Memory Management

When the standard library is disabled you have full control over new/delete for all downstream code.  You may wish to perform memory debugging or insert a custom allocator (for example a simple arena).  To do so just modify the export definitions in `hero/library.cpp`

## With C++ Standary Library

To build with standard library support comment out the 
following lines in CMakeLists.txt (you may optionally leave
exceptions and rtti disabled as standard library code will
work without these)

```
#add_compile_options(-fno-exceptions -fno-rtti)
add_link_options(-nodefaultlibs -lc)
```

You will also need to define `HERO_USING_STD` in `main.cpp` before other includes, or in `hero/hero.h`, or as an environment variable so that the exports are disabled.

If you enable the default libs but don't define `HERO_USING_STD` this result in duplicate definitions for new/delete and std::bad_alloc etc from `hero/library.h`.


After these changes `ldd` should report the following.

```
┌──(emerson㉿Flow)-[/mnt/c/Code/ZeroToHero/Source/git/gitlab/zero/build]
└─$ ldd zero
        linux-vdso.so.1 (0x00007ffeed7ef000)
        libstdc++.so.6 => /lib/x86_64-linux-gnu/libstdc++.so.6 (0x00007f86bec40000)
        libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00007f86bea3d000)
        libm.so.6 => /lib/x86_64-linux-gnu/libm.so.6 (0x00007f86be95e000)
        /lib64/ld-linux-x86-64.so.2 (0x00007f86bef6f000)
        libgcc_s.so.1 => /lib/x86_64-linux-gnu/libgcc_s.so.1 (0x00007f86be93e000)
```


## Running

Build output will produce a binary @ `build/zero`