# ZeroToHero

This is the first project for the YouTube channel ZeroToHero where amongst other things we explore the idea of developing a framework from first principles.

In this initial commit, we go from C to an object oriented/event drivent TCP echo server using LibUV, the same asychronous IO library and event loop used in both NodeJS and Rust in < 1000 lines.

We use a fictional language called C--, which is a subset of C++ features with no standard library.

# Build 

You can build using

python3 build.py

A simple universal makefile is provided which is capable of recursively building any C++ project with a src/ and nested folder structure.

The libuv library must be built and its static library symlinked to lib/libuv/libuv.a
