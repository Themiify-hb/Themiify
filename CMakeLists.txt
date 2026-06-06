include(/opt/devkitpro/cmake/WiiU.cmake)
cmake_minimum_required(VERSION 3.16)

project(themiify C CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(themiify
    src/main.cpp
    src/App.cpp
    src/NavBar.cpp
    src/ContentPanel.cpp
    src/screens/HomeScreen.cpp
    src/screens/DownloadScreen.cpp
    src/screens/InstalledScreen.cpp
    src/screens/ThemezerScreen.cpp
)
wut_create_rpx(themiify)
wut_create_wuhb(themiify
    CONTENT content
    NAME Themiify
    SHORTNAME Themiify
    AUTHOR Fangal_Airbag, Alpha-Craft, dkosmari & more!
    ICON meta/icon.png
    TVSPLASH meta/tv-splash.png
    DRCSPLASH meta/drc-splash.png
)

# For libmocha
target_include_directories(themiify PRIVATE "$ENV{DEVKITPRO}/wut/usr/include")
target_link_directories(themiify PRIVATE "$ENV{DEVKITPRO}/wut/usr/lib")

find_package(PkgConfig REQUIRED)

pkg_check_modules(sdl2 REQUIRED IMPORTED_TARGET sdl2)
pkg_check_modules(SDL2_mixer REQUIRED IMPORTED_TARGET SDL2_mixer)
pkg_check_modules(SDL2_image REQUIRED IMPORTED_TARGET SDL2_image)
pkg_check_modules(freetype2 REQUIRED IMPORTED_TARGET freetype2)
pkg_check_modules(harfbuzz REQUIRED IMPORTED_TARGET harfbuzz)
pkg_check_modules(libpng REQUIRED IMPORTED_TARGET libpng)
pkg_check_modules(libzip REQUIRED IMPORTED_TARGET libzip)

add_subdirectory(external)

target_link_libraries(themiify PRIVATE
    PkgConfig::sdl2 PkgConfig::SDL2_mixer PkgConfig::SDL2_image PkgConfig::freetype2 
    PkgConfig::harfbuzz PkgConfig::libpng PkgConfig::libzip bz2 mocha imgui glaze
)