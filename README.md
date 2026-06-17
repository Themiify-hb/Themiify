# Themiify
Themiify is a homebrew application that allows for on the fly installation of Wii U Menu themes via the [Themezer](https://themezer.net/) API or via `.utheme` files placed on an SD Card. 
Once a theme is installed [StyleMiiU](https://github.com/Themiify-hb/StyleMiiU-Plugin) will read the theme and show it the next time the Wii U Menu is loaded.

## Requirements
***Themiify will only work if you have the StyleMiiU aroma plugin installed and if your Wii U Menu's files on your Wii U's NAND are unmodified!***

Themiify will perform checks for these conditions at boot and let you know if they are not met.

## Usage
### Network Tab
You can download themes uploaded to Themezer from this tab. To search for more specific themes you can use different filters, the search bar or QR codes.

### Local Themes Tab
You can install `.utheme` files placed in `sd:/wiiu/themes` from this tab. You can also manage your installed themes here with the ability to either delete them or
set them as your current StyleMiiU theme on the fly.

### Settings Tab
If you are a theme creator, a quick way to dump your menu files from your NAND would be here. Simple select `Dump Wii U Menu Files` and follow the instructions
to dump!

#

For further information on usage and troubleshooting of Themiify, please consult the [Theme Café Docs](https://themecafe.github.io/Docs/install/themiify/).

## Building

To build Themiify please ensure you have the following packages installed via the dkp-pacman package manager: `ppc-bz2`, `ppc-zlib`, `ppc-freetype2`,
`ppc-harfbuzz`, `ppc-libpng`, `ppc-libzip`, `wut`, `wiiu-sdl2`, `wiiu-libcurl`, `wiiu-sdl2_mixer` & `wiiu-sdl2_image`.

Then simply run:

```shell
cmake -S . -B build
cmake --build build
``` 

and place the resulting `.wuhb` file in `sd:/wiiu/apps` and run it from the Wii U Menu.

## Credits

- [AlphaCraft9658](https://github.com/AlphaCraft9658) & [Daniel K. O](https://github.com/dkosmari) 
for all their help on the development of this project!
- Perrohuevo, dewgong and Daniel K. O. for their help designing the UI for this project!
- [Juanen100](https://github.com/Juanen100) for the [StyleMiiU Aroma Plugin](https://github.com/Themiify-hb/StyleMiiU-Plugin)!
- The Theme Café Discord mods, devs and founders!
- Gatto for the amazing [Theme Café Docs](https://themecafe.github.io/Docs/)!
- All the amazing Wii U theme creators!
- And many more!
