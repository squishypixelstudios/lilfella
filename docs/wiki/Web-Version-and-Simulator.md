# Web Version and Simulator

Lil Fella can also run without a board.

## Web version

There is a browser build of the game. The project builds it as a single web page, using the project's web build script (it needs the WASI SDK). The finished page is included in the project's `web` folder.

The web version runs the same game logic as the device, so the mechanics on this wiki apply to it too.

## Simulator

The project has a PC simulator for testing. It is built with `make` in the `sim` folder. It can take screenshots of the screens, run through the endings, and run long life-cycle tests at speed. It is aimed at the people who make the game, not at players.

If you are a player who just wants to play, you do not need it. Use a real board or the web version.
