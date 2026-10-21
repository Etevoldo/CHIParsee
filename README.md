![screenshot](/screenshot.png)

# What is this

Yet another CHIP-8 emulator.
Written with C with help of [raylib](https://github.com/raysan5/raylib) as window and graphics.

# How to run (Windows)

Download the raylib libraries fit for your system/compiler in [raylib](https://github.com/raysan5/raylib/releases) and drop it in the "lib" and "include" directory.

You might need to add `-Wl,--defsym,stat64i32=_stat64` in the Makefile FLAGS variable if you are using windows. [see more about this error](https://github.com/raysan5/raylib/discussions/5971)

```console
Make

./main rom_name.ch8
```
Enjoy!

Future Goals:
- [x] Sound
- [x] Add quirk configurations
- [ ] Pixel Fade out screen effect to reduce flickering
- [x] Texture manipulation for screen rendering instead of rec spam
- [x] Debug Functionality
    - [x] Stack
    - [x] All Registers
    - [x] Instructions scroller
- [ ] SUPER-CHIP
- [ ] XO-CHIP
- [x] implement [chip-8 database](https://github.com/chip-8/chip-8-database) for automatic plataform configuration
- [ ] Other Keyboard-layout Configs

# Libraries

All libraries licenses are either in the [externals directory](/externals/) or directly in the code as comments.

- [raylib](https://www.raylib.com/) by raysan. awesome and simple graphics and input library.
- [raygui](https://github.com/raysan5/raygui) ditto.
    - [file_dialog_window](https://github.com/raysan5/raygui/blob/master/examples/custom_file_dialog/gui_window_file_dialog.h) With some small modifications listed on the comments at the beggining of the file.
- [teenysha1](https://github.com/tsoding/teenysha1) by tsoding (yes, that tsoding)
- [cJSON](https://github.com/DaveGamble/cJSON) by Dave Gamble and cJSON contributors. Easy and effective to use json parser.
- [chip-8-database](https://github.com/chip-8/chip-8-database) by The CHIP-8 database authors.