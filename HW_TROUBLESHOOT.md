# HW Troubleshoot
After soldering your Shawazu DIY Kit, it is a good idea to perform a hardware check before inserting your Pokemon cartridge.  
When hardware soldering isn't correct it is undefined what signals the cartridge reads. To not lose your precious savegame (in the worst case) it is recommended to use a cartridge without savegame (or with savegame you don't care about losing) for debugging first.

## Requirements
For hardware debugging your will need a GB/GBC (NOT GBA) cartridge. Once you confirmed your soldering is good, you can use Shawazu with GBA cartridges too.  
Download the `shawazu-gb-dumper-hw_troubleshoot` build from the [Releases](https://github.com/tihmstar/shawazu-gb-dumper/releases) section and flash it onto your shawazu.

## Pinout
The GB cartridge pinout looks like this: ![GB cart pinout by there.oughta.be](https://there.oughta.be/assets/images/2021-12-16/pinout.jpg)
Basically from left to right the pins are:  
[5V][CLK][<span style="text-decoration:overline">WR</span>][<span style="text-decoration:overline">RD</span>][<span style="text-decoration:overline">CS</span>][A0-A15][D0-D7][<span style="text-decoration:overline">RST</span>][Audio][GND]



# Testing the soldering
There is no test for the control signals [5V][CLK][<span style="text-decoration:overline">WR</span>][<span style="text-decoration:overline">RD</span>][<span style="text-decoration:overline">CS</span>][<span style="text-decoration:overline">RST</span>][Audio][GND], make sure to check those manually!  
Most of the time these signals are not the issue though.

## Step 0) Testing the HW debug builds
**DO NOT CONNECT A CARTRIDGE YET!**  
Once you flash the hw debug build, the Shawazu should show up as mass storage drive on your PC containing the following two files:
`Info.txt` and `debug.gb`.

The `Info.txt` should look something like this (static content)
```
Title: debug
ROM Size: 32 KiB
RAM Size: 0 KiB
ROM Revision: 255
Cartridge Type: Unknown(255)

Shawazu GB Dumper Version: v1.2.13 (2024-10-18 13:56:44 +0200)
Board pinout (software): hw_toubleshoot rev3
```

The `debug.gb` can be viewed with a hex editor (or hexdump on mac/linux) and should contain `0x8000` bytes of just `FF` when everything is wired correctly.
```
tMBP:Volumes tihmstar$ hexdump -C SHAWAZU/debug.gb 
00000000  ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff  |................|
*
00008000
```
If the content of this file is not all `FF`, your soldering may be bad. This is okay for testing the hw build itself, the important thing is that the file is there in first place.  
Proceed with the next step to test the Data bus.

## Step 1) Testing D0-D7
**DO NOT CONNECT A CARTRIDGE YET!**  
You connect the dumper without any cartridge and take a look at the `debug.gb` file in a hex viewer (for example `hexdump -C` on linux/mac). Since the pins are set to input, the high impedence should make them all eventually go to `1`. Thus, it is expected that the hexdump reads all `FF`.
If you see anything else, some pin must be shorted to ground or somthing.

The next step is to take a little wire or paperclip or sth, and connect one data pin to GND. Best to do this directly on the cartridge connector. For example if you ground the LSB of the data bus, the .gb dump should read all FE.
Do this for each data pin individually and verify if the dump is correctly FD, FB, F7, etc...
Note: the Operating System reads the dump usually only once, then caches it and doesn't actually perform a re-read request. This read can happen at any time between when you connect your dumper, until the moment you actually open the file. So best is to short the pin to ground and hold it, then connect the dumper and copy the file to your PC. Only after the copy finished, you can stop shorting the pin. Then you need to disconnect the dumper from the PC and repeat the procedure for each pin.


The bits are numbered D0-D7 from right to left in the byte.
A byte 0xAB consists of two "nibbles" A and B in this case.
Each nibble is 4 bit.
```
HEX: 0xAB
BIN: 0b10101011 (left nibble 'A' 0b1010 right nibble 'B' 0b1011)
```
take whatever hex value you have and convert it to binary. 
For example using python
```
$ python -c 'print(bin(0xAB))'
0b10101011
```
or some online tool.
then count from right to left starting from 0 and ending at 7. that will give you the index of the data bus.
For example
```
$ python -c 'print(bin(0xF7))'
0b11110111
```
That means you grounded D3

So by grounding each pin one by one starting from D0 and ending by D7, you should see the sequence `FE, FD, FB, F7, EF, DF, BF, 7F`.

## Step 2) Testing A0-14

Next, you need a GB cartridge (not GBA) to verify the address lines.
Insert the cartridge and copy the dump, then analyse it on your PC. This will be a bit more tricky, but i'll try my best to explain as good as i can.
For this step it is recommended, that you have a known good dump of the cartridge. Use a known-to-be-working dumper to create a good dump, or you know "ask a friend if they happen to have a dump" _wink wink_.
Usually the issue is, that one (or multiple) address bits are "stuck" i.e. tied to either 0 or 1 permanently.
When a read request is issued with a stuck address bit, the resulting dump is "folded" at that bit. When multiple bits are stuck, it can be "folded" multiple times.
The address bus is 16bit in size. When dumping the address space, you need to count up from `0x0000` to `0xFFFF`.
Imagine counting:
```
0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 0x0008 
```
to get the first 8 bytes of the dump.
Let's assume (only) your LSB (0x0001) bit of the address bus is stuck, then the cartridge sees either of the following requests:
When the bit is stuck to 0:
```
0x0000, 0x0000, 0x0002, 0x0002, 0x0004, 0x0004, 0x0006, 0x0006, 0x0008 
```
When the bit is stuck to 1:
```
0x0001, 0x0001, 0x0003, 0x0003, 0x0005, 0x0005, 0x0007, 0x0007, 0x0009 
```
When you look at the hexdump, you will see that two consecutive bytes are always the same. For example if a good dump looks like this:
```
41 42 43 44 45 46 47 48
```
Your dump could look like this:
```
41 41 43 43 45 45 47 47
```
So if you see this pattern in the dump, you know your address is folded at the LSB (or address bit 0). Then you need to check that bit in the hardware and try to fix it.

Once you fixed that, keep looking for more repeating patterns.
For example let's assume you see this pattern:
```
41 42 43 44 41 42 43 44
```
Here the data repeats every 4 byte. 4 in binary is 0b0100, that means address bit A3 is stuck.
So the cartridge could see requests like this:
```
0x0000, 0x0001, 0x0002, 0x0003, 0x0000, 0x0001, 0x0002, 0x0003, 0x0008, 0x0009, 0x000a, 0x000b, 0x0008
```



If the data repeats after every 0x4000 bytes, we can convert that to binary:
```
$ python -c 'print(bin(0x4000))'
0b100000000000000
```
Now counting the bits from left to right we get 
```
$ python -c 'print(len("100000000000000"))'
15
```
But since we should be starting to count at 0 and not at 1 we subtract 1 from the number of bits and get 14. Hence bit A14 seems to be stuck.
Now if you see the lower 0x4000 bytes twice, that means that A14 is stuck to 0. If you see the upper 0x4000 bytes of the image twice, that means A14 is stuck to 1

## Additional Pins
Following this procedure should help you check most of the pins, which usually resolves most problems.
There are a couple special pins like CLK, ~WR, ~RD, ~CS, ~RST which i glossed over.
But for the sake of completeness let's handle them too:

- ~RST is not really used on GB cartridges. Don't remember about GBA, but here another method will be needed.
- CLK is really only use by gameboy camera, you can ignore that unless you're actually dealing with GB camera
- if ~CS doesn't work, no control signals will ever reach the chip
- ~RD is "clocked" for reading. If this doesn't work, the cartidge won't give you any data.
- ~WR is used for back switching and writing to savegame. If this is tied high, no backswitching will ever occur and games larger than 32K will repeat after `0x8000` bytes. If this pin is tied low, you will **most likely corrupt your savegame**, keep doing random bankswitches (if the game supports multiple banks) and potentially read (and simultaneously write) garbage.
