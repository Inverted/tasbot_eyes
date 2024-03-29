# tasbot_eyes

* [Dependencies and credit](#dependencies-and-credit)
* [Install](#install)
* [How it works](#how-it-works)
  + [Abstract program flow](#abstract-program-flow)
  + [About blink patterns](#about-blink-patterns)
* [Usage](#usage)
  + [Create color palette](#create-color-palette)
  + [Inject animation](#inject-animation)
  + [WLED UDP realtime control](#wled-udp-realtime-control)
    - [Colorchord](#colorchord)
* [Known problems](#known-problems)
* [Future ideas](#future-ideas)
* [Thanks to](#thanks-to)

## Dependencies and credit

* jgarff's rpi_ws281x lib under the BSD 2-Clause "Simplified" License, project found
  [here](https://github.com/jgarff/rpi_ws281x)
* Eric S. Raymond's GIFLIB under [a MIT like license](https://sourceforge.net/p/giflib/code/ci/master/tree/COPYING), project found
  [here](http://giflib.sourceforge.net/)


## Install

1. Clone, compile and install the rpi_ws281x lib (requires cmake)
2. Download, compile and install GIFLIB (found [here](https://sourceforge.net/projects/giflib/) on SourceForge, requires make)
3. Clone and compile this repo (requires cmake)  
   :warning: **ATTENTION**: When compiling for real TASBot, make sure `bool realTASBot` ist set to `true` (it is by default)

If you experience an error, that a shared library can't be loaded, try to execute `ldconfig` to update your library chache (and maybe reboot).

## How it works

The program looks within its current working directory, for a folder called `gifs` and subdirectories within it.

| Path                 | Explanation                                                                               |
|----------------------|-------------------------------------------------------------------------------------------|
| `./gifs/others`      | Here go all the general animations. The program is choosing randomly animations from here |
| `./gifs/blinks`      | Here are all blinking animations, the program can choose from                             |
| `./gifs/base.gif`    | The animation, TASBots eye returns to between the animations and blinks                   |
| `./gifs/startup.gif` | The animation, that gets played exactly once, when the program starts                     |


### Abstract program flow

1. Startup
2. Play startup animation
3. Show base animation
4. Sleep a random amount of time
5. Do a blink cycle, meaning
   1. Determine how many times TASBot blinks (n)
   2. Play random blink animation
   3. Show base expression
   4. Sleep
   5. n = n-1
   6. If n>0, go to b.
6. Check if animation stack is empty
    * if not:
      1. Pop top animation from stack
      2. Check if the animation has its own color. If not, choose a random one and overwrite all set pixels if wished
      3. Play animation
    * else:
      1. Generate a new stack and go to 6.
7. Go to 3.



### About blink patterns

It can be defined, how many times at most TASBot blinks between every random animation, but at least always once (if pattern is not making skipping them, e.g. `0-x-y`). It can also be defined, how long are the periods are between the blinks.  
**Example**: `3-4-6`

1. `3`: Maximum amount of blinks between blinks
2. `4`: At least 4 seconds between each blink...
3. `6`: ...but at most 6 seconds.

The actual amount of seconds the program sleeps between the blinks is randomly generated in milliseconds. In this example, TASBot could wait _4581 ms_, just as _5987 ms_. He could blink at least once, but at most 3 times, before the next random animation


## Usage

See the build-in `-h` option or [`arguments.c:28`](https://github.com/R3tr0BoiDX/tasbot_eyes/blob/main/arguments.c#L28) to get an overview of all available arguments. There are some for changing the playback speed, setting the data pin, playing a specific animation and many more!


### Create color palette

Color palettes are the random colors TASBot can choose from when playing a monochrome animation and colorful mode is activated. To create a color palette, you can either:

* Use the [TAS**B**ot **E**ye **C**olor **A**nalyzing **U**niquified **S**earch **E**ngine (BECAUSE)](https://github.com/R3tr0BoiDX/TASBot-Toolkit#because) to create a color palette based on an image
* Paste the desired hex palette into a plain text document **WITHOUT** the leading '#'. For example, a valid entry for a  pale red would be `E78587` (no '#' in front). One color per line. BECAUSE creates `.tbp` files, but those are just text files. File extension not required.


### Inject animation
To inject any animation :warning: **that is already stored on TASBot** :warning: send a UDP datagram to 8080. An abstract payload could look like `T;./path/to/animation.gif`.
* `T` is the playback type, how the animation gets played. It can either be:
  * `Q` for queueing the animation by adding it on top of the current animation stack. It will be the next animation to be played after the blinks.
  * `I` for immediate playback. Stops the current animation and plays this. :warning: Not implemented yet!
* `animation.gif` is a locally, already on TASBot stored animation! Full or relative paths are supported, as long as the file exists on TASBot!

To make injecting animations easier, you can use the [**An**imation **Inj**ecto**a**r (aNinja)](https://github.com/R3tr0BoiDX/TASBot-Toolkit#aninja) from the TASBot-Eyes-Toolkit if you want to.

### WLED UDP realtime control
The centered "nose" LEDs can be controlled in real time with the `-U` argument. It's based on the WLED UDP realtime control protocol, see [here](https://github.com/Aircoookie/WLED/wiki/UDP-Realtime-Control). Right now, only the `DRGB` mode is supported, also known as `mode 2`.
It covers an area of 48 LEDs in total in a 8x6 matrix.

#### Colorchord
[ColorChord](https://github.com/cnlohr/colorchord) 2 by CNLohr supports WLED instances. In order to be able to use the realtime control with ColorChord, make sure to add something like this section to your ColorChord config file:
```python
leds = 48
lightx = 8
lighty = 6

wled_realtime = 1
port = 19446  # Default port for UDP realtime
address = x.x.x.x  # TASBot IP
wled_timeout = 2 
```
There's also an example ColorChord config file (`colorchord.conf`) in the `misc` folder of this repository.

## Known problems

* Gamma correction is behaving extremely really weird. It's breaking the LED indexing even tho it should not.
* When an animation is monochrome, it's pixels can be colored in a randomly chosen color, to make everything more colorful (using the `-c` argument). However, the color that is used to overwrite isn't adjusting to the brightness of the given color within the frame. Right now it's flat overwriting the same color to all pixels, that just _aren't_ black. Use the method `getLuminance()` for that.
* Some methods got pretty blob like, that could be made :sparkles: prettier :sparkles:


## Future ideas

* Add a mutex to the stack
* An honest to goodness playlist support, in which animations can be queued as desired on startup
* Debate on, if during a blink cycle (a period between the random animations), TASBot should use the same blink animation for all blinks or if that should stay random like it is right now. This could further smooth out the appearance.
* Argument to change the base frame
* Loop argument for single frame mode


## Thanks to

* *dwangoAC* letting me debug on real TASBot and everything he's doing
* *jakobrs* for the original eye software and the index translation table and helping
* *Inverted* for pointing out gamma correction and helping to fix bugs
* *HeavyPodda* for helping debugging
* *CompuCat* helping with features
* *Adafruit* for the gamma correction table
