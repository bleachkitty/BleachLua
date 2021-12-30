# BleachLua
This is a C++ binding for Lua.

This is the Lua glue used by the Bleach engine for all my internal projects.  It's relatively engine-agnostic, so I've extracted it from the engine and put it up here on GitHub.  Feel free to use it directly in your projects or as a learning tool.

If you find any bugs, you can open up an issue or submit a pull request.

If you use it on a released game, crediting me for the library is appreciated but not required.

This binding has several advantages:
* It's simple.
* It's fast relatively fast.
* It allows you to trivially call Lua functions, and to trivially bind any compatible C++ function to Lua (i.e. any function with parameters and return types that Lua recognizes).
* The interface for dealing with tables with very simple and uses C++-like constructs (range-based for loops, structured bindings, etc.)
* It uses the unmodified Lua core, so you can compile it against any compatible version (see constraints below).

Constraints:
* You must use a C++ 17 compiler (though I might downgrade to C++ 14 just to allow more compatibility).
* It works and is tested on Lua 5.3.x.  I might still work on Lua 5.2.x if you set the appropriate setting in LuaConfig.h, but I haven't tested it.  Lua 5.2 support will be removed in a future iteration.  Earlier versions are not supported, nor is Lua 5.4 supported (yet).

One big note: I am primarily a game developer, not an open-source library creator.  This code is provided as-is.  No effort has been made to get this to work on other platforms or compilers, but it *should* be fine.  I will continue to update it with features and bug fixes, but probably won't create a fancy CMAKE script or anything.  If you want to do that, go for it.  :)  Feel free to submit a pull request if you like and I'll integrate it in.

## Getting Started

The best way to get started is to load up the sample project and see how it works.  It's heavily commented and demonstrates the key features of the library.  There's a lot more of course, but this should get you started.  Feel free to message me if you have any questions at all.
