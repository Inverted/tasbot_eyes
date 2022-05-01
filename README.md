# tasbot_eyes

## Dependencies and credit

* jgarff's rpi_ws281x lib under the BSD 2-Clause "Simplified" License, project found
  here: https://github.com/jgarff/rpi_ws281x
* Eric S. Raymond's GIFLIB under "an X Consortium-like open-source license", project found
  here: http://giflib.sourceforge.net/

## Install

1. Clone, compile and install the rpi_ws281x lib (requires cmake)
2. Download, compile and install GIFLIB (found on SourceForge https://sourceforge.net/projects/giflib/, requires make)
3. Clone and compile this repo (requires cmake)  
:warning: **ATTENTION**: When compiling for real TASBot, you wanna edit `main.c` and change `bool realTASBot = false;` to `bool realTASBot = true;` 

## How it works

The program looks within its current working directory, for a folder called `gifs` and subdirectories within it.

| Path | Explanation                                                                       |
| --- |-----------------------------------------------------------------------------------|
|`./gifs/others`| Here go all the general animations. The is choosing randomly animations from here |
|`./gifs/blinks`| Here are all blinking animations, the program can choose from                     |
|`./gifs/base.gif`| The frame, TASBots expression always returns to between the animations and blinks |

### Abstract program flow

0. Start
1. Show base expression
2. Sleep a random amount of time based on blink pattern
3. Do a blink cycle, meaning  
    1. Determine how many times TASBot blinks (_n_) based on pattern
    2. Blink
    3. Show base expression
    4. Sleep based on pattern
    5. _n = n-1_
    6. If _n>0_, go to ii.
4. Show random animation
    * Check if the animation has its own color. If not, choose a random one and overwrite all set pixels
6. Go to 1.

### About blink pattern

It can be defined, how many times at most TASBot blinks between every random animation, but at least always once (if
pattern is not making skipping them, e.g. `0-x-y`). It can also be defined, how long are the periods are between the
blinks.  
**Example**: `3-4-6`

1. `3`: Maximum amount of blinks between blinks
2. `4`: At least 4 seconds between each blink...
3. `6`: ...but at most 6 seconds.

The actual amount of seconds the program sleeps between the blinks is randomly generated in milliseconds. In this
example, TASBot could wait _4581 ms_, just as _5987 ms_. He could blink at least once, but at most 3 times, before the
next random animation

## Usage

| Argument           | Description                                                                                                                                                                                                                | Example                                                                                           |
|--------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|---------------------------------------------------------------------------------------------------|
| `-h`               | Print a help screen with this information                                                                                                                                                                                 | -                                                                                                 |
| `-v`               | Enable verbose logging                                                                                                                                                                                                     | -                                                                                                 |
| `-r`               | Enable console renderer for frames                                                                                                                                                                                         | -                                                                                                 |
| `-d [GPIO]`        | Change GPIO data pin. Possible options are between 2 to 27. Default is 10                                                                                                                                                  | `-d 18`                                                                                           |
| `-c`               | Use random palette for monochrome animations                                                                                                                                                                               | -                                                                                                 |
| `-b [0-255]`       | Set maximum possible brightness. Default is 24                                                                                                                                                                             | `-b 100`                                                                                          |
| `-s [MULTIPLIER]`  | Playback speed. Needs to be bigger than 0                                                                                                                                                                                  | `-s 1.25`                                                                                         |
| `-B [PATTERN]`     | Controls the blinks. Highest number that can be used within the pattern is 9<br />* 1st: Maximum number of blinks between animations<br />* 2nd: Minimum seconds between blinks<br />* 3rd: Maximum seconds between blinks | `-B 4-4-6`<br />* Maximum of 4 blinks between animations<br />* 4 to 6 seconds between each blink |
| `-p [FOLDER PATH]` | Play animations from a specific folder                                                                                                                                                                                     | `-p "/home/tasbot/display/animations"`                                                         |
| `-z [FOLDER PATH]` | lay blink animation from specific folder                                                                                                                                                                                   | `-z "/home/tasbot/display/animations/blink`                                                    |
| `-i [FILE PATH]`   | Play specific animation as endless loop. "-p" and "-z" become useless with this                                                                                                                                            | `-i "/home/tasbot/display/special_animations/magfest.gif"`                                     |
| `-P [FILE PATH]`              | Use color palette from text file. For formatting of palette file use tool or see example                                                                                                                                   | `-P "/home/pi/tasbot/display/palettes/gameboy.tasbotPalette"`                                     |

## Create color palette

To create a color palette, you can either:

* Use the *tasBot Eye Color Analyzing Uniquified Search Engine (BECAUSE)* to create a color palette based on an
  image: https://github.com/R3tr0BoiDX/because
* Paste the desired hex palette into a plain text document **WITHOUT** the leading '#'. For example, a valid entry for a
  pale red would be `E78587` (no '#' in front). One color each line. BECAUSE creates `.tasbotPalette` files, but that's
  not required.

## Known problems

* Actually use the delay time between frames, that's encoded within the GIF. There is already a method implemented, that
  needs to be tested yet. In theory, `0xF9` is the identifier for the byte from the "Graphics Control Extension"-block,
  where the delay time is encoded. More on that here: http://giflib.sourceforge.net/whatsinagif/bits_and_bytes.html
* When an animation is monochrome, it's pixels can be colored in a randomly chosen color, to make everything more
  colorful (using the `-c` argument). However, the color that is used to overwrite isn't adjusting to the brightness of
  the given color within the frame. Right now it's flat overwriting the same color to all pixels, that just _aren't_
  black. Use the method `getLuminance()` for that.
* It's the first time I used plain C. I hope I got all memory allocations right. I would appreciate, if someone could
  check my `malloc's, calloc's, memset's` and esp. `free's`.
* Some methods got pretty blob like, that could be made :sparkles: prettier :sparkles:
* The entire main.c is a huge blob, that could be resolved into several files (.h and .c)

## Future ideas

* Create a playlist support in two senses:
    * An honest to goodness playlist, in which animations can be queued as desired
    * Replace the current random selection of the animations with a queue. When queue is empty create a new one. This
      would prevent having the same animation chosen multiple times in a row or the impression, that a certain animation
      is favoured (just because it gets picked repeatedly by the random generator)
* Debate on if during a blink cycle (a period between the random animations), TASBot should use the same blink animation
  for all blinks or if that should stay random like it is right now. This could further smooth out the appearance.
* Argument to change the base frame
* Argument if the playback speed, which can be set with `-s xx`, should also affect the delays between the animations or just the animation itself.
