cmake_minimum_required(VERSION 3.20)
project(Bomberman C)

# set(CMAKE_GENERATOR "Visual Studio 17 2022")

message("-- Opening CMakeLists for '${PROJECT_NAME}'!")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "-- Libraries will be built statically..." FORCE)

add_subdirectory(${CMAKE_SOURCE_DIR}/deps/SDL)
add_subdirectory(${CMAKE_SOURCE_DIR}/deps/SDL_image)
add_subdirectory(${CMAKE_SOURCE_DIR}/deps/SDL_mixer)
add_subdirectory(${CMAKE_SOURCE_DIR}/deps/SDL_net)
add_subdirectory(${CMAKE_SOURCE_DIR}/deps/SDL_ttf)
add_subdirectory(${CMAKE_SOURCE_DIR}/deps/flecs)

# Prepare the include headers.
set(SDL3_INCLUDE
        ${CMAKE_SOURCE_DIR}/deps/SDL/include
        ${CMAKE_SOURCE_DIR}/deps/SDL_image/include
        ${CMAKE_SOURCE_DIR}/deps/SDL_mixer/include
        ${CMAKE_SOURCE_DIR}/deps/SDL_net/include
        ${CMAKE_SOURCE_DIR}/deps/SDL_ttf/include
)

set(GAME_MODULES_INCLUDE ${CMAKE_SOURCE_DIR}/deps/game_modules)

set(PLUTO_INCLUDE ${CMAKE_SOURCE_DIR}/deps/pluto/src)

set(BINN_INCLUDE ${CMAKE_SOURCE_DIR}/deps/binn/src)
set(FLECS_INCLUDE ${CMAKE_SOURCE_DIR}/deps/flecs/include)
set(FNL_INCLUDE ${CMAKE_SOURCE_DIR}/deps/fnl/C)
set(MLIB_INCLUDE ${CMAKE_SOURCE_DIR}/deps/mlib)
set(RANDOMBYTES_INCLUDE ${CMAKE_SOURCE_DIR}/deps/randombytes)

# Copying /dat to the build directory management.
message("-- Packaging data folder!")
file(REMOVE_RECURSE ${CMAKE_BINARY_DIR}/dat)
file(COPY ${CMAKE_SOURCE_DIR}/dat DESTINATION ${CMAKE_BINARY_DIR})

# Flags
set(CMAKE_C_FLAGS_RELEASE "-std=c99 -O1 -Wall -Wextra -pedantic -Wshadow")
set(CMAKE_C_FLAGS_DEBUG "-std=c99 -g -Wall -Wextra -pedantic -Wfloat-equal -Wundef \
     -Wshadow -Wpointer-arith -Wwrite-strings -Waggregate-return -Wunreachable-code \
     -Wstrict-prototypes -Wstrict-overflow=2 -Wcast-align -Wcast-qual -Wconversion \
     -Wswitch-default -Wswitch-enum -fno-omit-frame-pointer -fno-optimize-sibling-calls"
)

# Build external code I made which can be re-used through different projects separately.
if (EXISTS ${GAME_MODULES_INCLUDE})
    message("-- Building modules statically.")
    add_library(game_modules STATIC
            deps/game_modules/animation.c
            deps/game_modules/log.c
            deps/game_modules/sprite_atlas.c
            deps/game_modules/render_target.c
            deps/game_modules/text_man.c
            deps/game_modules/input_man.c
            deps/game_modules/profiling.c
            deps/game_modules/util.c)
    target_compile_options(game_modules PRIVATE -std=c99)
    target_include_directories(game_modules PRIVATE ${SDL3_INCLUDE} ${FLECS_INCLUDE} ${MLIB_INCLUDE})
    target_link_libraries(game_modules PRIVATE
            SDL3::SDL3
            SDL3_image::SDL3_image
            SDL3_ttf::SDL3_ttf
            flecs::flecs_static)
    set_target_properties(game_modules PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/deps/game_modules
    )
else ()
    message(FATAL_ERROR "-- Couldn't find any module for static build!")
endif ()

if (EXISTS ${PLUTO_INCLUDE})
    message("-- Building Pluto Framework statically.")
    add_library(pluto STATIC
            deps/pluto/src/pluto.c)
    target_compile_options(pluto PRIVATE -std=c99)
    target_include_directories(pluto PRIVATE ${SDL3_INCLUDE} ${FLECS_INCLUDE} ${MLIB_INCLUDE} ${GAME_MODULES_INCLUDE})
    target_link_libraries(pluto PRIVATE
            SDL3::SDL3
            SDL3_image::SDL3_image
            SDL3_ttf::SDL3_ttf
            flecs::flecs_static
            game_modules)
    set_target_properties(pluto PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/deps/pluto
    )
else ()
    message(FATAL_ERROR "-- Couldn't find Pluto Framework for static build!")
endif ()

# Build smaller libs who don't provide a CMakeList directly.

if (EXISTS ${BINN_INCLUDE})
    message("-- Building 'binn' library statically.")
    add_library(binn STATIC deps/binn/src/binn.c)
    target_compile_options(binn PRIVATE -std=c99)
    set_target_properties(binn PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/deps/binn
    )
else ()
    message(FATAL_ERROR "-- Couldn't find 'binn' library for static build!")
endif ()

# Build smaller libs who don't provide a CMakeList directly.
if (EXISTS ${RANDOMBYTES_INCLUDE})
    message("-- Building 'randombytes' library statically.")
    add_library(randombytes STATIC deps/randombytes/randombytes.c)
    target_compile_options(randombytes PRIVATE -std=c99)
    set_target_properties(randombytes PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/deps/randombytes
    )
else ()
    message(FATAL_ERROR "-- Couldn't find 'randombytes' library for static build!")
endif ()

# Set the source files and execute the build.
message("-- Executable compilation...")
set(SOURCES
        src/main.c
)

add_executable(${PROJECT_NAME} ${SOURCES})

# Finalize the include headers.
target_include_directories(${PROJECT_NAME} PRIVATE
        src
        ${GAME_MODULES_INCLUDE}
        ${SDL3_INCLUDE}
        ${BINN_INCLUDE}
        ${FLECS_INCLUDE}
        ${FNL_INCLUDE}
        ${MLIB_INCLUDE}
        ${RANDOMBYTES_INCLUDE}
        ${PLUTO_INCLUDE}
)

message("-- Linkage with libraries...")
# Linkage
target_link_libraries(${PROJECT_NAME} PRIVATE
        SDL3::SDL3
        SDL3_image::SDL3_image
        SDL3_mixer::SDL3_mixer
        SDL3_net::SDL3_net
        SDL3_ttf::SDL3_ttf
        binn
        flecs::flecs_static
        randombytes
        game_modules
        pluto
)

set_target_properties(${PROJECT_NAME}
        PROPERTIES
        C_STANDARD 99
        C_STANDARD_REQUIRED YES
        C_EXTENSIONS NO
)