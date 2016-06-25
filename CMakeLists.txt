cmake_minimum_required(VERSION 2.8)
project(extract-map-parts)

## Options
include_directories(".")
find_package(X11)

add_executable(extract-map-parts extract-map-partes.c)
target_link_libraries(extract-map-parts ${X11_LIBRARIES} cpgplot m)
install(TARGETS extract-map-parts DESTINATION bin)

