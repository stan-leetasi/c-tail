# c-tail

This program displays the last n lines from a file or the stdin stream. It functions the same as the standard POSIX tail.

## Usage

1. Compile the code with the provided Makefile:

`make tail`

2. Run the program with:

`./tail [-n COUNT] [FILE]`

`[-n COUNT]`	Specifies the number of lines to display (default: 10)
`[FILE]` 	Specifies which file to read from (default: stdin stream)

3. Delete the executable with:

`make clean`

## Examples

`./tail file.txt`	- Displays the last 10 lines from a file

`./tail -n 5`		- Display the last 5 lines from the stdin stream:

`./tail -n 3 file.txt`	- Display the last 3 lines from a file:

## Implementation Details
- The program utilizes a circular buffer data structure to store the last n lines.
- Circular buffer of size n gets created (default: 10)
- The program stores lines in the circular buffer as it reads from a file or stdin
- When it reaches the end of the file/text on stdin, it prints out the last n lines in the correct order and ends
