#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <err.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <stdint.h> // reference https://edstem.org/us/courses/38065/discussion/3028257
#include <sys/wait.h> //reference Exploration - Process API - Monitoring Child Processes - waitpid       
#include <stdbool.h> // include otherwise error on bool type
#include <limits.h>   // ref https://learn.microsoft.com/en-us/cpp/c-language/cpp-integer-limits?view=msvc-170        
#include <fcntl.h> // for O_WRONLY, etc. reference canvas exploration - processes and IO

#ifndef MAX_WORDS
#define MAX_WORDS 512
#endif

char *words[MAX_WORDS];
size_t wordsplit(char const *line);
char * expand(char const *word);
FILE *input;

pid_t PID_most_recent_background_process = INT_MIN; //shell variable for $!
int exit_status_last_foreground_cmd = INT_MIN; //shell variable for $?
bool sig_int_received = false;

void sigint_handler(int sig) {
   
  //THIS DIDN'T WORK - CAUSED CTRL C TO HANG ON GET LINE UNTIL OTHER INPUT
  // printf("\n");
   // errno = 0; //reset error
  //  clearerr(stdin); //clear stdin
   // sig_int_received = true;       
   // clearerr(input);
    //goto prompt;   
}; //reference instructions for smallsh for SIGINT (ctrl-c);

int main(int argc, char *argv[])
{
 
  input = stdin;
 //FILE *input = stdin;
 char *input_fn = "(stdin)";

//restart: 
 
 

  // INTERACTIVE MODE - if one argument, read from STDIN
  // NON-INTERACTIVE MODE - if two arguments, read from a file
  if (argc == 2) {
    input_fn = argv[1];
    input = fopen(input_fn, "re");
    if (!input) err(1, "%s", input_fn);
  } else if (argc > 2) {
    errx(1, "too many arguments");
  }

  char *line = NULL;
  size_t n = 0;



prompt:
  for (;;) {
  
 
    //*********************************************CHECK BACKGROUND PROCESSES ON EACH LOOP HERE******************
    /* TODO: Manage background processes */
    // reference PAGE 544 of Linux Programming Interface michael kerrisk chapter 26 - waitpid()
    pid_t child_process;
    int bg_child_status;

    // ref linux.die.net/man/2/waitpid/
    //WIFSTOPPED(status) returns true if the child process was stopped by delivery of a signal; 
    //this is only possible if the call was done using WUNTRACED or when the child is being traced (see ptrace(2)).

    while ((child_process = waitpid(0, &bg_child_status, WUNTRACED | WNOHANG)) > 0){  //ref pg 551 options can be or'd together
        if(WIFEXITED(bg_child_status)) fprintf(stderr, "Child process %jd done.  Exit status %d. \n", (intmax_t) child_process, WEXITSTATUS(bg_child_status)); //exit status
        else if(WIFSIGNALED(bg_child_status)) fprintf(stderr, "Child process %jd done.  Signaled %d. \n", (intmax_t) child_process, WTERMSIG(bg_child_status)); //signal #
       
        //need WUNTRACEd to detect stopped processes!!!
        else if (WIFSTOPPED(bg_child_status)) {
            //https://man7.org/linux/man-pages/man2/kill.2.htmlC
            fprintf(stderr, "Child process %jd stopped. Continuing. \n", (intmax_t) child_process);
            kill(child_process, SIGCONT); 
        }



    };  //ref programming interface pg 544 - if 0 wait for any child in smae process group as the caller
   // printf("%d\n", child_process);  // this should be -1 if no child processes issame process group reference https://linux.die.net/man/2/waitpid
   //********************END CHECKING BACKGROUND PROCESSES **********************************************************
     
 
    struct sigaction SIGINT_action = {0}, ignore_action = {0}, old_SIGINT ={0}, old_SIGTSTP={0};

    //INTERACTIVE MODE
    if (input == stdin) {
      /* COMPLETED TODO: prompt  */

      // Reference Canvas exploration - Signal Handling API - Example - Custom Handler for SIGINT for next 5 lines of code to ignore SIGINT
      // uses custom sigint_handler from smallsh instructions which does nothing - literally no body of function
      SIGINT_action.sa_handler = sigint_handler; //this is func that does nothing above
      //sigfillset(&SIGINT_action.sa_mask);
      //SIGINT_action.sa_flags = SA_RESTART; //reference signal handling api canvas - signals and interrupted functions section to fix getline error
     
      ignore_action.sa_handler = SIG_IGN; //reference exploration - canvas signal handling api
      sigaction(SIGINT, &SIGINT_action, &old_SIGINT);   // this so ctrl + c goes to the handler which does nothing - then error by getline handled below
      //sigaction(SIGTSTP, &ignore_action, &old_SIGTSTP);  // this so ctrl + z does nothing
      sigaction(SIGTSTP, &SIGINT_action, &old_SIGTSTP);  // this so ctrl + z does nothing; have to do this and not above line or infinite getline loop 

      char *prompt = getenv("PS1");
      if (prompt != NULL) printf("%s", prompt);  // print PS1
      else printf("");                           // expand empty string if NULL - ref 2. expansion in instructions
      //    char *expanded_prompt = expand(prompt);
    }

    
      // while (!sig_int_received && ) {
      ssize_t line_len = getline(&line, &n, input);
      //if (line_len < 0) err(1, "%s", input_fn);
      
      //THIS ALLOWS US TO RESTART IF WE SEND SIGINT WHILE IN INTERACTIVE MODE
      if (input == stdin) {
          
        //printf("stdin\n");
      if (feof(input) && line_len != 1) {
          if (ferror(input)) err(1, "read error"); // reference own work on b64 assignment
          else{
          //printf("end of stdin, exiting!\n");
            return 0;
          }                        
        }
        if (line_len < 0) {
            clearerr(stdin);
            printf("\n");
            goto prompt;
          }
      }
      else {
          if (line_len < 0) err(1, "%s", input_fn);
      }

      //if (line_len == 0) goto prompt;
 //   }

    
    // #2 word splitting - given function by professor - completed
    size_t nwords = wordsplit(line);
    

 // if (sig_int_received) {
//        sig_int_received = false;
//        clearerr(input);
 //      goto prompt;
 //  }



    // ** BLOCK TO EXPAND ALL WORDS - 2**************
    for (size_t i = 0; i < nwords; ++i) {
      //fprintf(stderr, "Word %zu: %s  -->  ", i, words[i]);     
      char *exp_word = expand(words[i]);
      free(words[i]);
      words[i] = exp_word;
     // fprintf(stderr, "%s\n", words[i]);
    }   
    // ** END BLOCK TO EXPAND ALL WORD***********
    



    //printf("print words again test");
  
      

     // fprintf(stderr, "%s\n", words[i]);


      // ***********BLOCK FOR IMPLEMENTING CD FUNCTION***************
      if (nwords > 0 && strcmp(words[0], "cd") == 0){
        //printf("test change directory\n");
        int cd_result;
        if (nwords >2) errx(1,"too many arguments\n");
        if (nwords >1) {
          char *exp_cd_path = expand(words[1]);
          cd_result = chdir(exp_cd_path);
          if (cd_result != 0) errx(1,"error changing folder\n");  
          // char curr_dir[1000];
          // getcwd(curr_dir, sizeof(curr_dir));  //https://linux.die.net/man/3/getcwd
        }
        else {
          cd_result = chdir(getenv("HOME"));
          if (cd_result != 0) errx(1, "error changing folder\n");
        }
        goto prompt;
      }
      //************END BLOCK IMPLEMENTING CD FUNCTION**************
      

      
      //***BLOCK FOR IMPLEMENTING BUILT IN EXIT FUNCTION************
      if (nwords > 0 && strcmp(words[0], "exit") == 0){
        if (nwords > 2) errx(1, "too many arguments\n");
        if (nwords ==1) {
          
          if (exit_status_last_foreground_cmd == INT_MIN) {
             exit(0);
          }
          else {
             exit(exit_status_last_foreground_cmd);
          }
        }
        else {
            for ( size_t i=0; i< strlen(words[1]); i++){
               if (!isdigit(words[1][i])) errx(1,"error gave exit function a non-digit!\n");
            }
            exit(atoi(words[1]));
        }
        
      }
      //***********END BLOCK IMPLEMENTING EXIT FUNCTION*************


      // ***** BLOCK FOR EXECUTING NEW PROCESS THAT IS NOT A BUILD IN PROGRAM ****************
     // reference Canvas Exploration - Process API - Executing a New Program
     
      int childStatus;
      pid_t spawnPid;
     // char *argv_test[] = {"ls", "-al", NULL};
      char *argv_for_execvp[(nwords+1)]; // +1 as last one has to be NULL
      bool redirect_input = false;
      bool redirect_output_append = false;
      bool redirect_output_truncate = false;
      char * redirect_file_name = NULL;
    ///  int out_target;  //output file for redirection file descriptor
     // int in_target; //input file to replace stdin                       
      bool run_in_background = false;
      pid_t spawnPid_fg;
      pid_t spawnPid_bg;

      //these two lines for resetting signals in child
       struct sigaction default_action = {0}, SIGSTP_action={0};
       default_action.sa_handler = SIG_DFL; //this is func that does nothing above 

       //struct sigaction reset_action = {0};
       //reset_action.sa_handler = SIG_DFL;

      if (nwords>0){
        spawnPid = fork();
      }
      else {
       goto prompt;
      }
 
            //char *testargv[] = {words[0], "-al", NULL};

      switch(spawnPid){

      // if process failed
      case -1:
        perror("fork() creating a new child has failed!!\n");
        exit(1);
        break;


      // in the child process
      case 0:
       // printf("in child process\n");
       // printf("CHILD(%d) running command\n", getpid());
       // char *testargv[nwords]; 
            
        //reference exploration canvas - signal handling api - sa handler - reset signals for child pocess - 5 pts
        //signal(SIGINT, SIG_DFL);
       // signal(SIGTSTP, SIG_DFL);
       //reference man7.org/linux/man-pages/man2/sigaction.2.html
   // uses custom sigint_handler from smallsh instructions which does nothing - literally no body of function

        sigaction(SIGINT, &old_SIGINT, NULL);   
        //sigaction(SIGTSTP, &default_action, NULL);  
        //struct sigaction SIGINT_action = {0};
        //SIGINT_action.sa_handler = sigint_handler; 
      //  SIGSTP_action.sa_flags = SA_RESTART;

        //SIGSTP_action.sa_handler = SIG_DFL; //this is func that does nothing above
      //sigfillset(&SIGINT_action.sa_mask);
      //SIGINT_action.sa_flags = SA_RESTART; //reference signal handling api canvas - signals and interrupted functions section to fix getline error          
       // SIGINT_action.sa_handler = sigint_handler;

        sigaction(SIGTSTP, &old_SIGTSTP, NULL);   // this so ctrl + c goes to the handler which does nothing - then error by getline handled below
        //sigaction(SIGTSTP, &SIGINT_action, NULL);        

        for (size_t i=0; i < (nwords+1); i++) {
            argv_for_execvp[i] = NULL;
         }

        //Need to construct array of arguments for the non-built in command and remove any redirection operators and their associated filename args
        for (size_t i=0; i < nwords; i++) {

       //   printf("number of words is %zu\n", nwords);
       //   printf("i is %zu\n", i);
       //   printf("word from word array is %s\n", words[i]);

          if (strcmp(words[i], "<") == 0){
           // printf("%zu", nwords);
            if (redirect_input) errx(1,"multiple redirects of same type\n");
            redirect_input = true;
            //printf("open specified file for reading on STDIN!\n");
            if ( (i+1) >= nwords ) errx(1,"redirection with no file name after!\n"); 
            redirect_file_name = words[i+1];
            int in_target = open(redirect_file_name, O_RDONLY);  //https://linux.die.net/man/3/open
            if (in_target == -1){
              perror("target failed to open"); //ref canvas exploration process and i/o
              exit(1);
            }
            //redirect output stdout to target
            int result = dup2(in_target, 0); // 0 is stdin and reference canvas exploration processes and i/o for code
            if (result == -1){
              perror("error with dup2 redirecting stdin!\n");
              exit(2);
            }      

            i++;

          }
          else if (strcmp(words[i], ">") == 0){
           // printf("open specified file for writing on STDOUT - possibly only in child later! - TRUNCATE MODE\n");
            if (redirect_output_truncate || redirect_output_append) errx(1, "multiple redirects of same type\n");
            redirect_output_truncate = true;
            if ( (i+1) >= nwords ) errx(1,"redirection with no file name after!\n");
            redirect_file_name = words[i+1];
            int out_target = open(redirect_file_name, O_WRONLY | O_CREAT | O_TRUNC, 0777);  //https://linux.die.net/man/3/open
            if (out_target == -1){
              perror("target failed to open"); //ref canvas exploration process and i/o
              exit(1);
            }
            //redirect output stdout to target
            int result = dup2(out_target, 1); // 1 is stdout and reference canvas exploration processes and i/o for code
            if (result == -1){
              perror("error with dup2!\n");
              exit(2);
            }                                       

            i++;
 
          }
          else if (strcmp(words[i], ">>") == 0){
           // printf("open specified file for writing on STDIN - possibly only in child later! - APPEND MODE\n");
            if (redirect_output_truncate || redirect_output_append) errx(1, "multiple redirects of same type\n");
            if ( ( i+1) >= nwords ) errx(1,"redirection with no file name after!\n");
            redirect_file_name = words[i+1];
            int out_target = open(redirect_file_name, O_WRONLY | O_CREAT | O_APPEND, 0777);  //https://linux.die.net/man/3/open
            if (out_target == -1){
              perror("target failed to open"); //ref canvas exploration process and i/o
              exit(1);
            }
            //redirect output stdout to target
            int result = dup2(out_target, 1); // 1 is stdout and reference canvas exploration processes and i/o for code
            if (result == -1){
              perror("error with dup2!\n");
              exit(2);

            }     
            i++; 
          }
          else if ((strcmp(words[i], "&") == 0) && (i == (nwords-1))){
           //   printf("background operator is last word - child!\n");
              //run_in_background = true;
          }
          else {
            
            argv_for_execvp[i] = words[i];  // if not redirection or filename after, put that in the array of arguments for commands and command itself is testargv[0]
         //   printf("word inserted into arguments array is %s\n", argv_for_execvp[i]); 
            
          }
         }

        //***** TEST TO PRINT OUT ARGUMENTS OF THE ARRAY WHICH SHOULD BE NULL TERMINATED************
//         printf("test - printing out arguments array for execvp\n");
         
//         for (size_t i=0; i < (nwords+1); i++) {
//            if (argv_for_execvp[i] == NULL){
//              printf("NULL\n");
//            }
//            else {
//              printf("%s\n", argv_for_execvp[i]);
//            }
//        }
        ///**************END TEST*********************************************************************

        // argv_for_execvp[nwords+1] = NULL;

        // printf("number of words is %zu\n", nwords);
        // testargv[i] = words[i];
        // printf("%s", words[i]);
        // printf("%s", testargv[i]); 
        // printf("printing commands for execvp\n");
        // char *argv_test[] = {"ls", "-al", NULL};
        // execvp(argv_for_execvp[0], argv_for_execvp);  //run program with array for argumnets where we removed redirections and associated files
        
        // printf("%s", argv_test[0]);
        // printf("%s", *argv_test);
        

        // execvp(argv_test[0], argv_test);       //this WORKS TO RUN HARDCODED LS COMMAND!!!!!
        execvp(argv_for_execvp[0], argv_for_execvp);                                       
        
        // execv(argv_test[0], argv_test);
        // printf("did we make it here?\n");   //NEVER WORKS AS CHILD IS REPLACED BY EXECVP - NORMAL

        //perror("error with execvp in child\n!");
        char exit_msg[100];
        sprintf(exit_msg, "smallsh: %s", argv_for_execvp[0]);  //doing this to match os1 output for sample program
       // printf("smallsh: %s", argv_for_execvp[0]);
        perror(exit_msg);

        exit(2);
        break;
      
      //in the parent process
      default:
           //  printf("in parent process: %d\n", getpid());
           //wait for child to TERMINATE with a blocking wait - test
           if ((strcmp(words[nwords-1], "&") == 0)){
              // printf("background operator is last word - parent!\n");
              run_in_background = true;
           }

           if (run_in_background){
          // RUN IN THE BACKGROUND
              spawnPid_bg = waitpid(spawnPid, &childStatus, WNOHANG); //reference Canvas Process - API - monitoring child processes
                                                                      
              //can't use spawnPid_bg as if non-blocking wait won't return right pid until finished, so grab the pid from fork which will be child in parent                                                                     
              PID_most_recent_background_process = spawnPid;  //update shell variable to be updated to PID of the child process; this is $!
           } 
           else{
          
               //PERFORM BLOCKING WAIT TO RUN IN FOREGREOUND
              //do {
              spawnPid_fg = waitpid(spawnPid, &childStatus, WUNTRACED);
            //  } while (!WIFEXITED(childStatus) && !WIFSIGNALED(childStatus));


              //if not in background and stopped - send signal to continue
              if (WIFSTOPPED(childStatus)) {
                //https://man7.org/linux/man-pages/man2/kill.2.htmlC
                //errno = 0;
              //  printf("stopped\n");
               // fprintf(stderr, "signaled stopped!!\n"); 
                //fprintf(stderr, "Child process %jd stopped - parent. Continuing. \n", (intmax_t) spawnPid_fg);
                kill(spawnPid_fg, SIGCONT); 
                PID_most_recent_background_process = spawnPid_fg; // set $! to pid as if was a background command
              }
              
              if(WIFSIGNALED(childStatus)) {
              // fprintf(stderr, "Child process %jd done.  Signaled %d. \n", (intmax_t) spawnPid, WEXITSTATUS(childStatus));
               // fprintf(stderr, "signaled!!%i\n", WTERMSIG(childStatus));
              //printf("signaled!\n");
             // printf("child terminated due to signal: %d\n", WTERMSIG(childStatus));
                exit_status_last_foreground_cmd = (WTERMSIG(childStatus)+128);   // if stopped by signal - set shell variable $? to exit status of waited for command plus value of 128
              }
              if(!WIFSIGNALED(childStatus)){
              //  printf("notsignaled!\n");
                exit_status_last_foreground_cmd = WEXITSTATUS(childStatus); // if not stopped by signal - set shell variable $? to exit status of waited for fg command
              }
             //  printf("child signalled is:%d\n", WTERMSIG(childStatus));                                                
            //   printf("PARENT(%d): child(%d) terminated. Exiting\n", getpid(), spawnPid);  //straight from canvas example
            break;
            }

      
      }
     //***********END BLOCK FOR EXECUTING A NON-BUILT IN PROGRAM*******************************


    



   
  } //end infinite loop
} // end main function
  



char *words[MAX_WORDS] = {0};

/* Splits a string into words delimited by whitespace. Recognizes
 * comments as '#' at the beginning of a word, and backslash escapes.
 *
 * Returns number of words parsed, and updates the words[] array
 * with pointers to the words, each as an allocated string.
 */
size_t wordsplit(char const *line) {
  size_t wlen = 0;
  size_t wind = 0;

  char const *c = line;
  for (;*c && isspace(*c); ++c); /* discard leading space */

  for (; *c;) {
    if (wind == MAX_WORDS) break;
    /* read a word */
    if (*c == '#') break;   // to ignore comments and everything after
    for (;*c && !isspace(*c); ++c) {
      if (*c == '\\') ++c;
      void *tmp = realloc(words[wind], sizeof **words * (wlen + 2));
      if (!tmp) err(1, "realloc");
      words[wind] = tmp;
      words[wind][wlen++] = *c; 
      words[wind][wlen] = '\0';
    }
    ++wind;
    wlen = 0;
    for (;*c && isspace(*c); ++c);
  }
  return wind;
}


/* Find next instance of a parameter within a word. Sets
 * start and end pointers to the start and end of the parameter
 * token.
 */
char
param_scan(char const *word, char **start, char **end)
{
  static char *prev;
  if (!word) word = prev;
  
  char ret = 0;
  *start = NULL;
  *end = NULL;
  char *s = strchr(word, '$');
  if (s) {
    char *c = strchr("$!?", s[1]);
    if (c) {
      ret = *c;
      *start = s;
      *end = s + 2;
    }
    else if (s[1] == '{') {
      char *e = strchr(s + 2, '}');
      if (e) {
        ret = '{';
        *start = s;
        *end = e + 1;
      }
    }
  }
  prev = *end;
  return ret;
}

/* Simple string-builder function. Builds up a base
 * string by appending supplied strings/character ranges
 * to it.
 */
char *
build_str(char const *start, char const *end)
{
  static size_t base_len = 0;
  static char *base = 0;


  //printf("end: %c\n", *end);


  if (!start) {
    /* Reset; new base string, return old one */
    char *ret = base;
    base = NULL;
    base_len = 0;
    return ret;
  }
  /* Append [start, end) to base string 
   * If end is NULL, append whole start string to base string.
   * Returns a newly allocated string that the caller must free.
   */
  //printf("start: %c\n", *start); 
  //printf("end: %c\n", *end);

  size_t n = end ? end - start : strlen(start);
  size_t newsize = sizeof *base *(base_len + n + 1);
  void *tmp = realloc(base, newsize);
  if (!tmp) err(1, "realloc");
  base = tmp;
  memcpy(base + base_len, start, n);
  base_len += n;
  base[base_len] = '\0';

  return base;
}

/* Expands all instances of $! $$ $? and ${param} in a string 
 * Returns a newly allocated string that the caller must free:
 */
char *
expand(char const *word)
{
  char const *pos = word;
  char *start, *end;
  char c = param_scan(pos, &start, &end);
  build_str(NULL, NULL);
  build_str(pos, start);
  while (c) {
    if (c == '!'){
      //update with most recent background process which is shell variable $!
      //build_str("<BGPID>", NULL);
      if (PID_most_recent_background_process == INT_MIN) {
        //build_str("<BGPID>",NULL);
        build_str("", NULL);
      }else {
        char pid_string_bg[100];
        sprintf(pid_string_bg, "%jd", (intmax_t) PID_most_recent_background_process);
        build_str(pid_string_bg, NULL);
      }
    }
    else if (c == '$') {
      // REFERENCE https://edstem.org/us/courses/38065/discussion/3028257 - using printf with types that lack format specifiers
      pid_t mypid = getpid();  //ref canvas exp process concepts & states
      char pid_string[100];
      sprintf(pid_string, "%jd", (intmax_t) mypid); //https://linux.die.net/man/3/sprintf  ref ED discussion above this is type conversion 
      //build_str("<PID>", NULL);
      build_str(pid_string, NULL);
      //printf("%s\n", pid_string);
    }
   // else if (c == '$') build_str("<PID>", NULL);
    else if (c == '?') {
      //char * exit_status = getenv("$?");
      //if (exit_status != NULL) build_str(exit_status, NULL);     
      //else build_str("",NULL);
      if (exit_status_last_foreground_cmd == INT_MIN) {
        //build_str("<STATUS>", NULL);
        build_str("0",NULL);
      } else {
        char string_last_exit_status_fg[100];
        sprintf(string_last_exit_status_fg, "%d", exit_status_last_foreground_cmd);
        build_str(string_last_exit_status_fg, NULL);
      }
   
    }
    else if (c == '{') {
     // build_str("<Parameter: ", NULL);
      //char * env_variable; //https://linux.die.net/man/3/getenv
      size_t str_len = (end-1) - (start+2) + 1;  // from skeleton code plus 1 for null terminator
      //printf("%zu", str_len); //reference https://linux.die.net/man/3/printf z is for size_t and u is for unsigned decimal
      
      size_t i;
      char env_variable[str_len-1];  
      // reference modified strcpy https://linux.die.net/man/3/strncpy to get string.  start and end pointers set by param scan function in skeleton code
      for (i = 0; i < str_len-1; i++) {
        env_variable[i] = *(start + 2 + i);  
      }
      env_variable[str_len-1] = '\0';
      //printf("%s", env_variable);
      
      //env_variable = getenv(build_str(start + 2, end -1));  //this takes the build_str already from skeleton code and puts it in getenv();
      char * actual_env_variable;
      //printf("%s\n", env_variable);
      actual_env_variable = getenv(env_variable);
      //printf("%s", actual_env_variable);
      if (actual_env_variable != NULL) build_str(actual_env_variable, NULL);
      else build_str("",NULL);
      //printf("%s\n", env_variable);
      //build_str(start + 2, end - 1); //this prints word between { } from testing it
      
      //build_str(">", NULL);
    }
    pos = end;
    c = param_scan(pos, &start, &end);
    build_str(pos, start);
  }
  return build_str(start, NULL);
}
