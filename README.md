# Shawazu GB Dumper

A reader for your GB / GBC / GBA games. It can dump ROM as well as backup and restore savegame.  
**No special software is needed!**   
Just plug into your computer and a mass storage device with your rom and savegame file will show up, similar to a regular USB drive.  

You can even play your cartridge with an emulator (tested mGBA) and savegame is loaded and stored transparently in the cartridge.

Old Gameboy games use battery backed SRAM for savegame storage, but after many yeary the battery can decline causing you to lose your savegame. Better back up those precious Pokémon now!

## Features
* Plug & Play on macOS, Linux and Windows. No special software needed
* Continue playing your physical cartridge on the big screen right where you left off, using an emulator (mGBA recommended)
* Dump Game ROM
* Backup & Restore Savegame
* Save pictures from Gameboy Camera
* Configure RTC
* Flash bootleg cartridges (not implemented)

## Usage
Connect Shawazu GB Dumper to your computer and plug in your cartridge.  
Hotplug is supported, so it doesn't matter whether you first connect the cartridge, or first plug into the computer.  
After *safe eject* Shawazu will not show up until it was manually reconnected.  
**IMPORTANT: GB / GBC cartridges run on 5V, GBA carts run on 3.3v. Set the correct voltage BEFORE plugging in the cartridge!**

### Updating
Drag and drop the .uf2 file to *SHAWAZU* drive will cause the rp2040 to enter bootloader (update) mode. When in bootloader more, drag and drop the .uf2 to the *RPI-RP2* drive.

## Compatible hardware

### Shawazu DIY
* Shawazu GB Dumper DIY board rev2 (limited edition)<br><img src="https://github.com/tihmstar/shawazu-gb-dumper/blob/master/images/DIY_rev2.jpg?raw=true" width="300" />
* Shawazu GB Dumper DIY board rev3 (limited edition)<br><img src="https://github.com/tihmstar/shawazu-gb-dumper/blob/master/images/DIY_rev3.jpg?raw=true" width="300" />
* Shawazu GB Dumper DIY board rev4 <br><img src="https://github.com/tihmstar/shawazu-gb-dumper/blob/master/images/DIY_rev4.jpg?raw=true" width="300" />

Rev2 has a different pinout and needs a slightly different firmware (rev2 firmware). Rev3 and newer use regular firmware (rev3 firmware).

### Shawazu Mini
* Shawazu Mini rev1 (limited edition)<br><img src="https://github.com/tihmstar/shawazu-gb-dumper/blob/master/images/mini_rev1.jpg?raw=true" width="300" />

Shawazu Mini runs same firmware as rev3 & rev4.

# Where to get one?
The DIY variant is open hardware. You can find the KiCad files and the gerber files in the [hardware repo](https://github.com/tihmstar/shawazu-gb-dumper-hardware).  
Alternatively, you can purchase a DIY self-solder kit from a partnered seller.  
The Shawazu mini is still in development and will hopefully be soon available at a partnered seller.

## Official partnered sellers
* Coming soon!

# Demos

Playing in emulator directly from cartridge. Savegame is loaded and stored transparently on the cartridge.
<br><img src="https://github.com/tihmstar/shawazu-gb-dumper/blob/master/images/demo_gba.gif?raw=true" width="600" />

Gameboy Camera Images are automatically converted to bmp files.
<br><img src="https://github.com/tihmstar/shawazu-gb-dumper/blob/master/images/gb_camera.jpg?raw=true" width="400" />

# Discord
One day i will probably make my own discord server for these kind of projects. Until then you can find me on the [Corco Cartridge Dev](https://discord.com/invite/jxGfqw2mdt) server in the **SHAWAZU** subsection.
