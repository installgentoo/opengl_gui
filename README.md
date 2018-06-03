Metelisa imgui

This is a small library for gui drawing with OpenGL, inside your OpenGL context. It's real fast,
it's made to be straightforward, readable and easily extendable.

-why not just use dear imgui?
Too much code to read. Also one of the points about this library was allowing the usage of multiple shaders.
It is also optimised, i do caching, unlike imgui. The editor scene from the demo takes 60 microseconds to
translate all elements into draws and another 100 for opengl to rasterize on my ancient 2-core laptop.

Requires c++14.
Build with cmake. You need to have glfw library installed on linux, on win it is built automatically(see demo/.cmake/glfw.cmake). Win build tested with mingw 7.3.
There is prebuilt windows binary in ./bin

Running on linux:
cd ./demo/demo/proj
cmake ./
make -j4
cd ./bin
./tester_main

Distributed under BSD License, licenses for libraries are as stated in libraries.

Refer to demo/proj/tester/tester_main.cxx for detailed comments



![Alt Text](https://raw.githubusercontent.com/installgentoo/opengl_gui/master/record.gif)
