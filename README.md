# Report
We broke the project down into the overarching categories of reading input,
parsing, implementing built-in functions, redirection, pipes, and background
processes.

## Overall Program Flow
The program starts off asking the user for input and reads it in using
fgets(). The input is parsed to create spaces separating all the arguments.
This string is then tokenized by the white space delimiter and immediately
checked for built-in commands. If found, the commands are executed and the
program continues to the next iteration for input. Otherwise, the arguments
are parsed for errors where the meta characters are either given illegal
parameters or are present in incorrect positions. If no errors are found, we
check if there is any piping involved. If we find that there is a pipe
present, we run the appropriate method that handles the execution of pipes
and if present, redirection. Otherwise, we check if there is stand-alone
redirection present (no piping) and set the redirection flags. If we find
stand-alone redirection, we pass in a parsed set of arguments, where the
occurrence of the meta characters '>' and '<' is removed and a location flag
is set to document the location of the input or output file argument. We
then pass this parsed set to a regular command executer (does not handle
pipes). Both the pipe handler and the regular command handler use forking to
process the commands. If the argument array has neither redirection or
piping, we pass the array itself to the regular command handler. Once the
regular command handler or the pipe handler returns control to the main, the
sshell continues to ask for the next command. Sshell does this until the
user gives it an exit command.

## Reading Input and Parsing
After reading input using fgets(), we took a two-step approach to parsing.
First, we added white spaces to the original string upon reaching any one of
the meta-characters (<, > , | , &) to create a "spaced string". So, for
example, upon receiving the input 'echo hello>file', we loop through the
input array, and find the index where '>' occurs. We then proceed to insert
a white space before and after this location and keep the character at its
index.  When there are white spaces already present, the we insert an extra
space. Once the white spaces are put in place, this new string is passed
back to main. We then took this "spaced string" and sorted every argument
into a new argument array using the strok() method with a white space
delimiter. Because we use the white space as the delimiter, even when two
spaces are created between meta characters and other arguments, every
character is put into the arguments array as an individual element. When
handling regular commands, this is the only parsing we do.

When redirection is involved, we take the tokenized string and pass to a
method which removes the '<' or '>' from the arguments array to create a new
redirection specific array, sets the redirection flag, and store the index
of the meta character to find the appropriate location of the input or the
output file name within the modified argument array.

With piping, we count the number of pipes and if there pipes present (more
than 0 pipes), we pass control to the pipe handler. Within the pipe handler,
we parse the argument array so to save everything to the left of the pipe in
an array and then everything to the far right in another.

## Built-in Functions
We check the tokenized argument array to see if the command is one of the
three built in commands. If it is 'exit', the shell terminates immediately.
We've implemented 'cd' and 'pwd' using the chdir() and the getcwd() calls
respectively. If the command is not a built in command, the control is then
passed back to main.

## Regular Commands
As explained above we classified regular commands to be those that did not
require piping or redirection. For example: 'cat file', or 'echo hello'. In
terms of control flow, regular commands are passed to the regular command
handler after checking for piping and redirection.

### Regular Command Handler
The regular command executes both regular commands and redirection commands
with the meta-characters '>' and '<'. For regular commands, it accepts the
argument array for which the parsed string is just every element of the
array separated by a white space. It then takes this array and creates a
child process by forking. The child process executes the first argument of
the argument array on the rest of the array elements. While this occurs, the
parent process waits for the child to finish and exit. Once the child
process finishes, the status is updated; if the status is not 0, the parent
prints an error message. Otherwise, we print the command status to console.

## Redirection
To execute commands involving only redirection, we used the regular command
handler. Before calling the regular command handler we parsed through the
arguments array to remove the redirection meta character, store the index,
and set the flag. Once this is done, we check if the flag is set in main,
and if it is we pass this modified array to the regular command handler. The
command handler, depending which flag is set (input or output redirection
flag), opens the file, and saves the file descriptor value. It checks if the
file descriptor value is less than 0, and if it is, handles the error of not
being able to open the file. If it is not, we use dup2() to direct the input
or output appropriately.

## Pipe Handler
For piping, the control is passed to the pipe handler upon checking if the
number of pipes is greater than 0 with the original arguments array. Once in
the pipe handler, we re-parsed the original arguments array to get the
commands to the left of the pipe and the commands to the far right of the
pipe. The control is then passed off to singlePipe() which is looped based
on the number of pipes found in the inputted command. The pipe handler is
always called once.

#### Single Pipe Handler
To implement the case of one pipe, the single pipe handler is only executed
once. In the single pipe handler we use dup() to redirect the output of the
first command to the second command as its input.

#### Multiple Pipes
For the case of multiple pipes, we use the number of pipes to loop and parse
through every pipe instance. And upon the occurrence of a pipe, the single
pipe handler is executed n times where n is equal to the number of
processes, or the number of pipes.

Note: We were able to get both the instances of single pipes and multi-pipes
to execute correctly and perform the proper commands. However, we were
unable to print out the appropriate status completed or error messages.

## Background Processes
For background processes we set a background flag if we encounter the
meta-character '&' while parsing. If the background flag is set, the parent
does not wait for the child and the process runs in the background.

## Testing
Our primary method of testing was to create our own commands that mimicked
those of the test script as well as those provided in the project
description. We made sure to include commands which, if perfectly executed,
would result in each of the error messages to make sure we were handling
errors as well. Our final test was to run the testing script and make sure
we passed every test that our program was fortified to do.

## Sources
Our main sources included Professor Porquet's lecture slides and multiple
Stack Overflow posts and tam 5's GitHub for help on redirection, piping,
parsing, and background processes.


