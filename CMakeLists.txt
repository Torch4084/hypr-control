cmake_minimum_required(VERSION 3.16)
project(hypr-control LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED IMPORTED_TARGET gtk4)
pkg_check_modules(LIBADWAITA REQUIRED IMPORTED_TARGET libadwaita-1)

add_executable(hypr-control main.cpp)

target_link_libraries(hypr-control PRIVATE
    PkgConfig::GTK4
    PkgConfig::LIBADWAITA
)

install(TARGETS hypr-control DESTINATION /usr/local/bin)
install(FILES hypr-control.desktop DESTINATION /usr/share/applications)
