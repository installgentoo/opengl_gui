if(WIN32)
 set(OPENGL_LIBRARIES opengl32 CACHE FILEPATH "OpenGl lib")
else()
 find_package(OpenGL)
endif()

append(EXTRA_LINK_LIBS ${OPENGL_LIBRARIES})
