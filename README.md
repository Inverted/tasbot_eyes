# tasbot_eyes

Uses
* jgarff's rpi_ws281x lib under the BSD 2-Clause "Simplified" License, project found here: https://github.com/jgarff/rpi_ws281x
* Eric S. Raymond's GIFLIB under "an X Consortium-like open-source license", project found here: http://giflib.sourceforge.net/

## Install
1. Clone, compile and install the rpi_ws281x lib (requires cmake)
2. Download, compile and install GIFLIB (found on SourceForge https://sourceforge.net/projects/giflib/, requires make)
3. Clone and compile this repo (requires cmake)

## Create color palette
To create a color palette (not implementet yet), you can either:
* Use the *tasBot Eye Color Analyzing Uniquified Search Engine (BECAUSE)* to create a color palette based on an image: https://github.com/R3tr0BoiDX/because
* Paste the desired hex colors into a plain text document **WITHOUT** the leading '#'. For example, a valid entry for a pale red would be "E78587" (no '#' in front). BECAUSE creates `.tasbotPalette` files, but thats not needed.
