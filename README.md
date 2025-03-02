<h1 align="center">Conway Game of Life Numworks
    <br>
    <img src="repoIcon.png" alt="App Logo" height="100"/>
    <br>
    <img src="https://img.shields.io/github/license/BenchatonDev/conway-life-numworks"/>
    <img src="https://img.shields.io/github/downloads/BenchatonDev/conway-life-numworks/latest/total"/>
</h1>

# About the project :
This is a simple C++ app for the Numworks Graphing Calculator made in an afternoon for fun, it's based on [Riley0122's C++ template app](https://github.com/riley0122/numworks_template_cpp?tab=MIT-1-ov-file) which is just [the Official template](https://github.com/numworks/epsilon-sample-app-cpp) stripped to only keep what's necessary. After "creating" the [Conway Game of Life Wii U](https://github.com/BenchatonDev/conway-life-sdl2-wii-u) app I really got into cellurar automatons, while my coding skills limit me to Conway's automaton they didn't stop me from implementing it everywhere I thought it would be interresting to see such as Roblox which is not at all design for 2D work or in th Terminal each time finding on my own ways in which I could optimize my code and small features I wanted to add, but on all the previously mentioned implementations, the cell grid was of a fixed size... Well not in that one, this time the grid is infinite, and the actual simulation is as optimized as I can code it whithout multithreading, it will still start slowing down once 100 -> 300 cells are in due to the calculator's slow 200MHz cpu and the number of cells that can exist at one time is only 1024 but that's because I don't want to go to close to using the full 256ko of ram and or overwhelm the cpu. In this inplementation you can place cells, delete cells, change the camera and cursor speed, save a patern into 1 of 10 clip board slots each capable of storing a patern containing 128 cells.

# Controls :
- `EXE`: Switch between camera and cursor controls
  - `Minus`: Zooms out / Shrinks the cursor
  - `Plus`: Zooms in / Grows the cursor
  - `Dpad`: Moves the Camera / Cursor
- `Toolbox`: Toggles the speed control mode (green cursor)
  - `Plus`: Speeds up the Camera / Cursor
  - `Minus`: Slows down the Camera / Cursor
  - `Ans`: Resets the Camera / Cursor Speed
- `Shift`: Toggles Clipboard mode (red cursor)
  - `Var (Copy)` + `[0 -> 9]`: Saves the patern to that slot
  - `Toolbox (Paste)` + `[0 -> 9]`: Pastes the patern saved on that slot
- `Ans`: Sets the camera pos back to (0, 0)
- `Home`: Quits the app
- `OK`: Places cells at the cursor's position
- `Back`: Removes cells at the cursor's position
- `Backspace`: Toggles Pause

# Building
You'll need the embeded arm compiler, nodeJS 16, and a few python libraries, if you are on MacOS, Windows or a Debian based distro, Numworks provides instructions [here](https://www.numworks.com/engineering/software/build/). If like me you use Arch Linux, I'd recomend you use an AUR helper like [yay](https://github.com/Jguer/yay) and run the following command :
```
yay -S base-devel arm-none-eabi-gcc numworks-udev nvm nodejs npm
```
The first two packages allow you to compile the project, numworks-udev allows WebDFU and by extention NWLink to see the calculator as the calculator, and the last three are for the packaging of the app, nvm allows you to switch nodeJS version which is usefull because to my knowledge NWLink doesn't work on versions higher than 16 and npm is just node's package manager.
