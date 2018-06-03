set(LZ4_PATH "${LIBS}/lz4" CACHE FILEPATH "LZ4 path")
set(LZ4_INCLUDES ${LZ4_PATH})
file(GLOB LZ4_SRC "${LZ4_PATH}/*.c")

append(EXTRA_INCLUDES ${LZ4_INCLUDES})
append(EXTRA_SRC ${LZ4_SRC})
add_definitions(-DUSE_LZ4)
