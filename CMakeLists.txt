include(/opt/devkitpro/cmake/WiiU.cmake)
set(PKG_CONFIG_EXECUTABLE
    "$ENV{DEVKITPRO}/portlibs/wiiu/bin/powerpc-eabi-pkg-config"
    CACHE FILEPATH "pkg-config executable" FORCE
)
cmake_minimum_required(VERSION 3.16)

project(themiify C CXX)

add_executable(themiify
    src/main.cpp
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
pkg_check_modules(SDL2_ttf REQUIRED IMPORTED_TARGET SDL2_ttf)
pkg_check_modules(SDL2_image REQUIRED IMPORTED_TARGET SDL2_image)
pkg_check_modules(freetype2 REQUIRED IMPORTED_TARGET freetype2)
pkg_check_modules(harfbuzz REQUIRED IMPORTED_TARGET harfbuzz)
pkg_check_modules(libpng REQUIRED IMPORTED_TARGET libpng)
pkg_check_modules(libzip REQUIRED IMPORTED_TARGET libzip)

target_link_libraries(themiify PRIVATE
    PkgConfig::sdl2 PkgConfig::SDL2_mixer PkgConfig::SDL2_ttf PkgConfig::freetype2 
    PkgConfig::harfbuzz PkgConfig::libpng PkgConfig::libzip bz2 mocha
)