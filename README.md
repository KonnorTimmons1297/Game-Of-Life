# Game Of Life

[Conway's Game of Life](https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life) implementation in C using SDL2. 

## Building

You will need a C compiler as well as [SDL2](https://www.libsdl.org/). There is a bash build script for the project that compiles the project with `gcc`.

If you have `gcc` and SDL2 installed, then just invoke the build script:

```bash
./build.sh
```

## Usage

Simply execute the program!

```
./gol
```

### Controls

- Space bar: pause/resume simulation
- Arrow keys: pan around the simulation
- Click on cell: Bring cell to life, or remove life from the cell



## Contributing
Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.

## License
[MIT](https://choosealicense.com/licenses/mit/)
