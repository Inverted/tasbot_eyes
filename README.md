# tasbot_eyes


## Dependencies and credit

* jgarff's rpi_ws281x lib under the BSD 2-Clause "Simplified" License, project found
  [here](https://github.com/jgarff/rpi_ws281x)
* Eric S. Raymond's GIFLIB under "an X Consortium-like open-source license", project found
  [here](http://giflib.sourceforge.net/)


## Install

1. Clone, compile and install the rpi_ws281x lib (requires cmake)
2. Download, compile and install GIFLIB (found [here](https://sourceforge.net/projects/giflib/) on SourceForge, requires make)
3. Clone and compile this repo (requires cmake)  
   :warning: **ATTENTION**: When compiling for real TASBot, make sure `bool realTASBot` ist set to `true` (it is by default)


## How it works

The program looks within its current working directory, for a folder called `gifs` and subdirectories within it.

| Path                 | Explanation                                                                               |
|----------------------|-------------------------------------------------------------------------------------------|
| `./gifs/others`      | Here go all the general animations. The program is choosing randomly animations from here |
| `./gifs/blinks`      | Here are all blinking animations, the program can choose from                             |
| `./gifs/base.gif`    | The frame, TASBots expression always returns to between the animations and blinks         |
| `./gifs/startup.gif` | The animation, that gets played exactly once, when the eye starts                         |


### Abstract program flow

0. Start
1. Play startup animation
2. Show base expression
3. Sleep a random amount of time based on blink pattern
4. Do a blink cycle, meaning
    1. Determine how many times TASBot blinks (_n_) based on pattern
    2. Choose and show random blink
    3. Show base expression
    4. Sleep based on pattern
    5. _n = n-1_
    6. If _n>0_, go to ii.
5. Show random animation
    * Check if the animation has its own color. If not, choose a random one and overwrite all set pixels
6. Go to 1.


### About blink patterns

It can be defined, how many times at most TASBot blinks between every random animation, but at least always once (if pattern is not making skipping them, e.g. `0-x-y`). It can also be defined, how long are the periods are between the blinks.  
**Example**: `3-4-6`

1. `3`: Maximum amount of blinks between blinks
2. `4`: At least 4 seconds between each blink...
3. `6`: ...but at most 6 seconds.

The actual amount of seconds the program sleeps between the blinks is randomly generated in milliseconds. In this example, TASBot could wait _4581 ms_, just as _5987 ms_. He could blink at least once, but at most 3 times, before the next random animation


## Usage

See the build-in `-h` option or [`arguments.c:225`](https://github.com/R3tr0BoiDX/tasbot_eyes/blob/develop-branch/arguments.c#L225) to get an overview of all available arguments. There are some for changing the playback speed, setting the data pin, playing a specific animation and many more!


### Create color palette

Color palettes are the random colors TASBot can choose from when playing a monochrome animation and colorful mode is activated. To create a color palette, you can either:

* Use the [TAS**B**ot **E**ye **C**olor **A**nalyzing **U**niquified **S**earch **E**ngine (BECAUSE)](https://github.com/R3tr0BoiDX/TASBot-Toolkit#because) to create a color palette based on an image
* Paste the desired hex palette into a plain text document **WITHOUT** the leading '#'. For example, a valid entry for a  pale red would be `E78587` (no '#' in front). One color per line. BECAUSE creates `.tbp` files, but those are just text files. File extension not required.


### Inject animation
To inject any animation :warning: **that is already stored on TASBot** :warning: send a UDP datagram to 8080. An abstract payload could look like `T;animation.gif`.
* `T` is the playback type, how the animation gets played. It can either be:
  * `Q` for queueing the animation by adding it on top of the current animation stack. It will be the next animation to be played after the blinks.
  * `I` for immediate playback. Stops the current animation and plays this. :warning: Not implemented yet!
* `animation.gif` is a locally, already on TASBot stored animation! Full or relative paths are supported, as long as the file exists on TASBot!

To make injecting animations easier, you can use the [**An**imation **Inj**ector **A**lpha (aNinja)](https://github.com/R3tr0BoiDX/TASBot-Toolkit#aninja) from the TASBot-Eyes-Toolkit if you want to.


## Known problems

* There's a teeny tiny memory leak.
* Gamma correction is behaving extremely really weird. It's breaking the LED indexing even tho it should not.
* When an animation is monochrome, it's pixels can be colored in a randomly chosen color, to make everything more colorful (using the `-c` argument). However, the color that is used to overwrite isn't adjusting to the brightness of the given color within the frame. Right now it's flat overwriting the same color to all pixels, that just _aren't_ black. Use the method `getLuminance()` for that.
* Some methods got pretty blob like, that could be made :sparkles: prettier :sparkles:


## Future ideas

* An honest to goodness playlist support, in which animations can be queued as desired on startup
* Debate on, if during a blink cycle (a period between the random animations), TASBot should use the same blink animation for all blinks or if that should stay random like it is right now. This could further smooth out the appearance.
* Argument to change the base frame
* Loop argument for single frame mode


# Thanks to

* *dwangoAC* letting me debug on real TASBot
* *jakobrs* for the original eye software and the index translation table
* *Inverted* for pointing out gamma correction
* *HeavyPodda* for helping debugging
* *CompuCat* helping with features
* *Adafruit* for the gamma correction table
