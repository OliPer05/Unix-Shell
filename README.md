# Unix-Shell

A small Unix shell written in C. It runs ordinary commands, chains commands together with pipes, and supports a two-way pipe operator that connects two programs to each other. Before every prompt it prints a random quote from `quotes.txt`.

## Features

- **Simple commands** – runs any program on your `PATH` with arguments (`ls -l`, `cat file.txt`) using `fork` + `execvp`.
- **Pipelines** – `cmd1 | cmd2 | cmd3 ...` connects each command's output to the next command's input (up to 100 commands).
- **Two-way pipe** – `cmd1 = cmd2` creates two pipes so that `cmd1`'s output feeds `cmd2`'s input *and* `cmd2`'s output feeds back into `cmd1`'s input.
- **Random quote** – one of the quotes in `quotes.txt` is shown before each `#` prompt.
- **`exit`** (or Ctrl+D) quits the shell.

## Build

```bash
gcc -Wall -o mysh mysh.c
```

## Run

Run it from the folder that contains `quotes.txt` (the shell loads it from the current directory at startup):

```bash
./mysh
```

Example session:

```
The way to happiness: Keep your heart free from hate, your mind from worry. ... ~Norman Vincent Peale
# ls | wc -l
4
Do your little bit of good where you are; ... ~Desmond Tutu
# exit
```

## Files

| File | Description |
|------|-------------|
| `mysh.c` | Shell source code |
| `quotes.txt` | Quotes shown before each prompt (one per line) |

## Notes

- Arguments are split on spaces and tabs only; quoting, redirection (`<`, `>`), and background jobs (`&`) are not supported.
- Built-in commands such as `cd` are not implemented.
