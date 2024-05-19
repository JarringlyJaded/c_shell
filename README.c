test cases:

readcommand.c
- reads the command given through batch or interactive mode
- when successful, the command is printed/repeated back to the user. 
  It is printed as indevidual tokens.
- This program determines wheather to be in batch mode or interactive mode.
  Once it determines the mode it parses through the command and aquires the tokens
  needed to execute. The program prints the command as a substitute for executing.

executecommand.c
- executes the program according to what was provided on the given line.
- when successful, the program executes and the user gets a response based on what they entered.
- This program executes what the user provides onto the entry line. To be specific, it runs through the
  execute_command function to check for specific commands entered by user and also checks with the 
  *find_executable function for directories.