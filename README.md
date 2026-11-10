# FBWMine
This is a (mostly joke) program to load Minecraft chunks into FBW. It presents these chunks nicely in the form of a **World Globe** block which contains an entire minecraft world.

## How to use
*If you don't want to compile the code, download the code then look in the config folder (**I haven't added any releases yet so be aware this is unfinished and unpolished**).*
- If you haven't already done this, move `ANStar_fbwmine` into your `Input/Packages` folder for `FBW`.
- Put `FBWMine.exe` in a folder with a `blocks.txt` (an example of a valid `blocks.txt` is in `config`).
- Open `FBW` and wait until it gets to the main screen, but don't enter a world. (If you need to, add `ANStar_fbwmine` to `XAR` mods). 
- Run `FBWMine.exe`.
- If necessary input your FBW base path (this happens once).
- If necessary input your region folder path for a **modern (1.13+ at least)** Minecraft world. This is a folder for all the regions of a certain dimension in a world. In the latest versions, this folder is located at `<world save path>/dimensions/minecraft/<dimension>/region`. You will know it is a region folder if it contains many `.mca` files.
- If necessary input spawn coords. NOTE: If you want to change your world / spawn coords, just delete `settings.txt` before running the program.

The program should now run! Please message me with any errors.

**To find a World Globe, look in a Yellow Flower...**

### Easy config
If you want to just test the program, then do the first step of **How to use**, open `FBW` then go to `config`. In `config`, rename one of the `settings` files to `settings.txt` and run `FBWMine.exe` from inside `config`. If it says *FBW not found*, wait until you can see the menu and try again.

## How to compile
- Open `FBWMine.slnx` in VS; use most up-to-date tools
- Select `Solution > Properties > Common Properties > Configure Startup Projects > Current Selection`, then `Apply`.
- Select `Release` and `x86` for your build configuration.
- Build `FBW_Minecraft` and `Packer`.
- Navigate to the `Release` folder in file explorer and run `Packer.exe` from there.
- Build `Loader`. Now `FBWMine.exe` should be in `Release`.
- If your Antivirus starts throwing around false accusations, just use `FBW_Minecraft.exe` instead. It does the same thing as `FBWMine.exe`, but is larger and has no icon.

**Unmodified `FBW_Minecraft.exe` SHA512:**
> 9ba17873bca06ffe297ace08daaf6e211759defe6658ea6aa321f85da9a1c3e27dddb7c51a79ba49fff4fffc1f6dc7b1bcb18d33789769fb35a9321b1b649b47

**Unmodified `FBWMine.exe` SHA512:**
> 04deb7f2c0f87a02cec4be7085ee9a1f0b80808a11c339979474ec09afcad7ad212ea1f044be291f9c9086882812c603e0de96e607091e5c603aeb8065988dd3


## What I'm looking to add
- Put World Globe in a sensible place.
- Modify World Globe to make it a bit nicer e.g. add tutorial.
- Separate mod dev tutorial, also in FBW.
- Preload spawn chunks with warning message from World Globe.
- Chunk / Region full error handling.
- Fix some subtle sync issues.
- Fix some memory / resource leaks.
- Clean up code (a bit).

## What I'm NOT looking to add
- Translucent, noclip water.
- New custom textures (though modifying existing ones is fine).
- Skybox, fake sun, daylight / weather cycle, functional portals etc.

This is meant to be a demo, albeit a slightly bloated one. If there's interest in this, I can make a version which autostarts in the background to make it nicer to use, and other mod devs can do the rest.
