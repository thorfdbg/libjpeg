
add_executable(jpeg-tool "")

target_include_directories(jpeg-tool PRIVATE "${CMAKE_CURRENT_LIST_DIR}/..")

# Add all .cpp and .hpp sources files. Adding .hpp is not needed for the build but helps IDEs to show the files.
target_sources(jpeg-tool
  PRIVATE
    bitmaphook.hpp
    bitmaphook.cpp
    defaulttmoc.hpp
    defaulttmoc.cpp
    encodea.hpp
    encodea.cpp
    encodeb.hpp
    encodeb.cpp
    encodec.hpp
    encodec.cpp
    filehook.hpp
    filehook.cpp
    iohelpers.hpp
    iohelpers.cpp
    main.hpp
    main.cpp
    reconstruct.hpp
    reconstruct.cpp
    tmo.hpp
    tmo.cpp
)

target_link_libraries(jpeg-tool PRIVATE jpeg)
