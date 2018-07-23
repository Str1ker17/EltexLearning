# EltexLearning 
Whole project status for `trunk`: [![Build Status](https://travis-ci.com/Str1ker17/EltexLearning.svg?branch=trunk)](https://travis-ci.com/Str1ker17/EltexLearning)

This is a personal repository with learning tasks.

Currently done programs:
- `bc` - commander with directory listing, based on ncurses
- `be` - text file editor, works in "replace mode", based on ncurses
- `bterm` - named pipe (FIFO) reader
- `bfcopy` - utility to copy files using different methods, based on `libcopyfile`

Currently done libraries:
- `libcopyfile` - copy file function using different methods (stdio, file descriptors, sendfile)
- `libeditor` - embeds functionality of text editor
- `liblinux_util` - functions to ease developement with linux
- `libncurses_util` - functions to ease developement with ncurses
- `libvector` - implements simple array-like collection with ability to grow, containing 64-bit integers

Currently done training programs:
- `tcpmaxaccept3` - creates a listening TCP socket and establishes as much as possible connections to it, uses epoll

How to build:
- Clone this repo
- `cd` to root directory of this repo
- `make` (is an alias to `make release`) or `make debug` if you want debugging symbols
- Executables are in `bin` subdirectory, libraries are in `lib`
- If you want a single target, just point it, for example: `make bc`

Feel free to send issues and pull requests.
