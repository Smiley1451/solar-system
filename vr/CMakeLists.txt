cmake_minimum_required(VERSION 3.16)
project(N-Body-VR)
set(CMAKE_CXX_STANDARD 17)
add_compile_options(-w)

set(ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")

# Include directories
include_directories(
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${ROOT_DIR}/include"
    "${ROOT_DIR}/include/Data"
    "${ROOT_DIR}/include/glm"
    "${ROOT_DIR}/external/freeglut/include"
    "${ROOT_DIR}/external/glew/include"
)

# Source files
set(VR_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/vr_main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/vr_tracker.cpp"
    "${ROOT_DIR}/src/Data/SolarSystemDataSet.cpp"
    "${ROOT_DIR}/src/Data/AbstractData.cpp"
)

add_executable(N-Body-VR ${VR_SOURCES})

# Libraries to link
target_link_libraries(N-Body-VR
    "${ROOT_DIR}/external/freeglut/build/lib/libfreeglut.dll.a"
    "${ROOT_DIR}/external/glew/build/lib/libglew-shared.dll.a"
    opengl32
    glu32
    ws2_32
    winmm
    gdi32
)

# Copy runtime DLLs to build directory
add_custom_command(TARGET N-Body-VR POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${ROOT_DIR}/freeglut.dll"
        "$<TARGET_FILE_DIR:N-Body-VR>/freeglut.dll"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${ROOT_DIR}/libfreeglut.dll"
        "$<TARGET_FILE_DIR:N-Body-VR>/libfreeglut.dll"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${ROOT_DIR}/glew32.dll"
        "$<TARGET_FILE_DIR:N-Body-VR>/glew32.dll"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${ROOT_DIR}/libglew-shared.dll"
        "$<TARGET_FILE_DIR:N-Body-VR>/libglew-shared.dll"
)
