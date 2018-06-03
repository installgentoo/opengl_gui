Metelisa imgui

This is a small library for gui drawing with OpenGL, inside your OpenGL context. It's real fast,
it's made to be straightforward, readable and easily extendable.

-why not just use dear imgui?
Too much code to read. Also one of the points about this library was allowing the usage of multiple shaders.
It is also optimised, i do caching, unlike imgui. The editor scene from the demo takes 60 microseconds to
translate all elements into draws and another 100 for opengl to rasterize on my ancient 2-core laptop.

Requires c++14.
Build with cmake. You may need to provide a path to glfw libraries on your system, especially on windows. Everything else was small enough to be included as sources.

Distributed under BSD License, licenses for libraries are as stated in libraries.

Refer to demo/proj/tester/tester_main.cxx for detailed comments



![Alt Text](https://raw.githubusercontent.com/installgentoo/opengl_gui/master/record.gif)
