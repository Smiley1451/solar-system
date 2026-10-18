cmake_minimum_required (VERSION 3.16)
project (N-Body-Simulation)
set(CMAKE_CXX_STANDARD 17)
add_compile_options(-w)

if (UNIX)
    message("Compiling for Linux")
    find_package(GLUT)
    find_package(GLEW)
    find_package(Boost REQUIRED COMPONENTS program_options)
    find_package(OpenMP)
    if(DEFINED ENABLE_OPENMP)
        if(OPENMP_FOUND AND ${ENABLE_OPENMP})
            message(NOTICE "Compiling with OpenMP enabled!")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
        endif()
    endif()
    if(DEFINED ENABLE_SIMD)
        if(${ENABLE_SIMD})
            message(NOTICE "Compiling with SIMD enabled!")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DENABLE_SIMD")
        endif()
    endif()
    set(BOOST_INC "/usr/include/boost")
    set(BOOST_LIB "/usr/lib/x86_64-linux-gnu/")

    find_package(OpenCL REQUIRED)
    find_package(OpenGL REQUIRED)

    file (GLOB CORE_SRC "${CMAKE_CURRENT_SOURCE_DIR}/lib/Core/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/lib/Core/*.c")
    file (GLOB OPENCL_SRC "${CMAKE_CURRENT_SOURCE_DIR}/lib/OpenCL/*.c" "${CMAKE_CURRENT_SOURCE_DIR}/lib/OpenCL/*.cpp")
    set(OPENCL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib/OpenCL")
    set(CORE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib/Core")

    file(GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
    file(GLOB DATA_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/Data/*.cpp")
    file(GLOB RENDER_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/Render/*.cpp")
    file(GLOB SIM_CALC_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/Simulation/*.cpp")
    file(GLOB PERFORMANCE_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/PerformanceMetrics/*.cpp")

    add_executable(N-Body-Simulation ${SOURCES} ${OPENCL_SRC} ${CORE_SRC} ${DATA_SOURCE} ${RENDER_SOURCE} ${SIM_CALC_SOURCE} ${PERFORMANCE_SOURCE})
    target_include_directories(N-Body-Simulation PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/lib" "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/include/Data" "${CMAKE_CURRENT_SOURCE_DIR}/include/Simulation" "${CMAKE_CURRENT_SOURCE_DIR}/include/glm" ${OPENCL_PATH} ${CORE_PATH} ${Boost_INCLUDE_DIR} ${OpenMP_CXX_INCLUDE_DIRS})
    target_link_libraries(N-Body-Simulation ${OpenCL_LIBRARY} ${CMAKE_DL_LIBS} ${Boost_LIBRARIES} ${GLUT_LIBRARIES} ${GLEW_LIBRARIES} ${OPENGL_LIBRARIES} ${OpenMP_CXX_LIBRARIES})

elseif(WIN32)
    message(STATUS "Compiling for Windows")
    add_definitions(-DCL_TARGET_OPENCL_VERSION=120)

    # Use bundled external dependencies if present
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/freeglut/include")
        set(GLUT_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/external/freeglut/include")
        set(GLUT_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/external/freeglut/build/lib/libfreeglut.dll.a")
    else()
        set(GLUT_INCLUDE_DIRS "C:/lib/glut/include/")
        set(GLUT_LIBRARY "C:/lib/glut/lib/freeglut.lib")
    endif()

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/glew/include")
        set(GLEW_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/external/glew/include")
        set(GLEW_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/external/glew/build/lib/libglew-shared.dll.a")
    else()
        set(GLEW_INCLUDE_DIRS "C:/lib/glew-2.1.0/include")
        set(GLEW_LIBRARY "C:/lib/glew-2.1.0/lib/Release/x64/glew32.lib")
    endif()

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/boost-headers")
        set(BOOST_INC "${CMAKE_CURRENT_SOURCE_DIR}/external/boost-headers")
    else()
        set(BOOST_INC "C:/lib/boost_1_79_0/")
    endif()

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/OpenCL-Headers")
        set(OpenCL_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/external/OpenCL-Headers")
        set(OpenCL_LIBRARY "C:/Windows/System32/OpenCL.dll")
    else()
        find_package(OpenCL REQUIRED)
    endif()

    find_package(OpenGL REQUIRED)

    find_package(OpenMP)
    if(DEFINED ENABLE_OPENMP)
        if(OpenMP_FOUND AND ${ENABLE_OPENMP})
            message(NOTICE "Compiling with OpenMP enabled!")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
        endif()
    endif()

    if(DEFINED ENABLE_SIMD)
        if(${ENABLE_SIMD})
            message(NOTICE "Compiling with SIMD enabled!")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DENABLE_SIMD")
        endif()
    endif()

    set(CORE_SRC
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/Core/Assert.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/Core/Error.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/Core/Exception.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/Core/Memory.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/Core/StrError.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/Core/Time.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/Core/TimeSpan.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/Core/WindowsError.cpp"
    )

    file (GLOB OPENCL_SRC "${CMAKE_CURRENT_SOURCE_DIR}/lib/OpenCL/*.c" "${CMAKE_CURRENT_SOURCE_DIR}/lib/OpenCL/*.cpp")
    set(OPENCL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib/OpenCL")
    set(CORE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib/Core")

    file(GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
    file(GLOB DATA_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/Data/*.cpp")
    file(GLOB RENDER_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/Render/*.cpp")
    file(GLOB SIM_CALC_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/Simulation/*.cpp")
    file(GLOB PERFORMANCE_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/PerformanceMetrics/*.cpp")

    add_executable(N-Body-Simulation ${SOURCES} ${OPENCL_SRC} ${CORE_SRC} ${DATA_SOURCE} ${RENDER_SOURCE} ${SIM_CALC_SOURCE} ${PERFORMANCE_SOURCE})

    target_include_directories(N-Body-Simulation PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/Data"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/Simulation"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/glm"
        "${CMAKE_CURRENT_SOURCE_DIR}/lib"
        ${OPENCL_PATH}
        ${CORE_PATH}
        ${BOOST_INC}
        ${OpenCL_INCLUDE_DIRS}
        ${GLUT_INCLUDE_DIRS}
        ${GLEW_INCLUDE_DIRS}
    )

    target_link_libraries(N-Body-Simulation PUBLIC
        ${OpenCL_LIBRARY}
        ${GLUT_LIBRARY}
        ${GLEW_LIBRARY}
        opengl32
        glu32
        gdi32
        winmm
        imagehlp
    )

    if(DEFINED ENABLE_OPENMP AND ${ENABLE_OPENMP})
        target_link_libraries(N-Body-Simulation PUBLIC ${OpenMP_CXX_LIBRARIES} -fopenmp)
    endif()

    # Automatically copy DLLs to build output directory
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/freeglut/build/bin/libfreeglut.dll")
        add_custom_command(TARGET N-Body-Simulation POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/external/freeglut/build/bin/libfreeglut.dll"
                "$<TARGET_FILE_DIR:N-Body-Simulation>/freeglut.dll"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/external/freeglut/build/bin/libfreeglut.dll"
                "$<TARGET_FILE_DIR:N-Body-Simulation>/libfreeglut.dll"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/external/glew/build/bin/libglew-shared.dll"
                "$<TARGET_FILE_DIR:N-Body-Simulation>/glew32.dll"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/external/glew/build/bin/libglew-shared.dll"
                "$<TARGET_FILE_DIR:N-Body-Simulation>/libglew-shared.dll"
        )
    endif()
endif()