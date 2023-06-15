# Video Demo

https://www.youtube.com/watch?v=vD2dPFSQ668

# CS-344-Small-Shell

- This project is programmed in C, and is the portfolio project for the Oregon State University Class CS 344 - Operating Systems.  It is allowed to be posted to a public GitHub repo after the class has ended.
  
- Demonstrates an understanding of memory allocation, forking of child processes, signal handlers, and other C language concepts.

  ![image](https://github.com/Mike11199/CS-344-Small-Shell/assets/91037796/075d1fac-f170-492c-a5a6-1b1a650f1690)




  # Code Section - Replacing Child Process With EXECVP

  ```C  
  // in the child process
      case 0:
        sigaction(SIGINT, &old_SIGINT, NULL);    
        sigaction(SIGTSTP, &old_SIGTSTP, NULL);

        for (size_t i=0; i < (nwords+1); i++) {
            argv_for_execvp[i] = NULL;
         }

        //Construct array of arguments for the non-built in command and remove any redirection operators + filename args
        for (size_t i=0; i < nwords; i++) {

          // If a word is "<", then redirect STDIN to instead read from that file
          if (strcmp(words[i], "<") == 0){
            if (redirect_input) errx(1,"multiple redirects of same type\n");
            redirect_input = true;
            if ( (i+1) >= nwords ) errx(1,"redirection with no file name after!\n"); 
            redirect_file_name = words[i+1];
            int in_target = open(redirect_file_name, O_RDONLY);  //https://linux.die.net/man/3/open
            if (in_target == -1){
              perror("target failed to open"); 
              exit(1);
            }
            //redirect output stdout to target
            int result = dup2(in_target, 0);
            if (result == -1){
              perror("error with dup2 redirecting stdin!\n");
              exit(2);
            }      
            i++;
          }
          
          // If a word is ">", then redirect STDOUT to instead write to that file in TRUNCATE mode
          else if (strcmp(words[i], ">") == 0){
            if ( (i+1) >= nwords ) errx(1,"redirection with no file name after!\n");
            redirect_file_name = words[i+1];
            int out_target = open(redirect_file_name, O_WRONLY | O_CREAT | O_TRUNC, 0777);  //https://linux.die.net/man/3/open
            if (out_target == -1){
              perror("target failed to open");
              exit(1);
            }
            //redirect output stdout to target
            int result = dup2(out_target, 1); 
            if (result == -1){
              perror("error with dup2!\n");
              exit(2);
            }                                       

            i++;
 
          }

          // If a word is ">>", then redirect STDOUT to instead write to that file in APPEND mode
          else if (strcmp(words[i], ">>") == 0){
            if ( ( i+1) >= nwords ) errx(1,"redirection with no file name after!\n");
            redirect_file_name = words[i+1];
            int out_target = open(redirect_file_name, O_WRONLY | O_CREAT | O_APPEND, 0777);  //https://linux.die.net/man/3/open
            if (out_target == -1){
              perror("target failed to open"); 
              exit(1);
            }
            //redirect output stdout to target
            int result = dup2(out_target, 1); 
            if (result == -1){
              perror("error with dup2!\n");
              exit(2);

            }     
            i++; 
          }
          else if ((strcmp(words[i], "&") == 0) && (i == (nwords-1))){
          }
          else {   
            argv_for_execvp[i] = words[i];     
          }
         }

        execvp(argv_for_execvp[0], argv_for_execvp); //Run built-in command, searching in path variable if needed      
        //BELOW WILL ONLY RUN IF AN ERROR OCCURS, AS THE EXECVP PROGRAM REPLACES THE CHILD PROCESS
        char exit_msg[100];
        sprintf(exit_msg, "smallsh: %s", argv_for_execvp[0]);  //doing this to match os1 output for sample program
        perror(exit_msg);
        exit(2);
        break;
  ```
