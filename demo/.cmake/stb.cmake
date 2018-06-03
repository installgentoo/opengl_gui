set(STB_PATH "${LIBS}/stb")
set(STB_INCLUDES "${STB_PATH}" CACHE FILEPATH "stb headers")

append(EXTRA_INCLUDES ${STB_INCLUDES})
add_definitions(-DUSE_STB)
