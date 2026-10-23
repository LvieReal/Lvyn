cmake_minimum_required(VERSION 3.21)
project(game LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(GAME_INCLUDES
    "${PROJECT_SOURCE_DIR}/src"
)

file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/src/*.c"
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui Quick QuickWidgets ShaderTools)
set(GAME_LIBS
    Qt6::Core Qt6::Widgets Qt6::Gui Qt6::Quick Qt6::QuickWidgets
)

add_executable(${PROJECT_NAME} ${GAME_SOURCES})

target_compile_options(${PROJECT_NAME} PRIVATE -O3 -s)
target_link_libraries(${PROJECT_NAME} PRIVATE ${GAME_LIBS})
target_include_directories(${PROJECT_NAME} PRIVATE ${GAME_INCLUDES})

qt_add_shaders(${PROJECT_NAME} "shaders"
    BATCHABLE
    FILES
        assets/shaders/grid.vert
        assets/shaders/grid.frag
)
