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

#ifndef MAX_WORDS
#define MAX_WORDS 512
#endif

char *words[MAX_WORDS];
size_t wordsplit(char const *line);
char * expand(char const *word);

int main(int argc, char *argv[])
{
  
  FILE *input = stdin;
  char *input_fn = "(stdin)";

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
  
 
    /* TODO: Manage background processes */

    /* TODO: prompt */
    char *prompt = getenv("PS1");
    if (prompt != NULL) printf("%s", prompt);  // print PS1
    else printf("");                           // expand empty string if NULL - ref 2. expansion in instructions
//    char *expanded_prompt = expand(prompt);
    //printf("%s", prompt);
   // printf("%s", expanded_prompt);


    if (input == stdin) {

    }
    ssize_t line_len = getline(&line, &n, input);
    if (line_len < 0) err(1, "%s", input_fn);
    
    size_t nwords = wordsplit(line);
    
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

    for (size_t i = 0; i < nwords; ++i) {
      fprintf(stderr, "Word %zu: %s  -->  ", i, words[i]);

      

      char *exp_word = expand(words[i]);
      free(words[i]);
      words[i] = exp_word;
      fprintf(stderr, "%s\n", words[i]);
    }   
    

   
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
    if (c == '!') build_str("<BGPID>", NULL);
    else if (c == '$') {
      // REFERENCE https://edstem.org/us/courses/38065/discussion/3028257 - using printf with types that lack format specifiers
      pid_t mypid = getpid();  //ref canvas exp process concepts & states
      char pid_string[100];
      sprintf(pid_string, "%jd\n", (intmax_t) mypid); //https://linux.die.net/man/3/sprintf  ref ED discussion above this is type conversion 
      //build_str("<PID>", NULL);
      build_str(pid_string, NULL);
      //printf("%s\n", pid_string);
    }
   // else if (c == '$') build_str("<PID>", NULL);
    else if (c == '?') {
      //char * exit_status = getenv("$?");
      //if (exit_status != NULL) build_str(exit_status, NULL);     
      //else build_str("",NULL);
      build_str("<STATUS>", NULL);
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
