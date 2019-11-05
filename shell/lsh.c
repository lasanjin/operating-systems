/*
 * Main source code file for lsh shell program
 *
 * You are free to add functions to this file.
 * If you want to add functions in a separate file
 * you will need to modify Makefile to compile
 * your additional functions.
 *
 * Add appropriate comments in your code to make it
 * easier for us while grading your assignment.
 *
 * Submit the entire lab1 folder as a tar archive (.tgz)
 * Command to create submission archive:
 * $ tar cvf lab1.tgz lab1/
 *
 * All the best
 */

#include <stdio.h>
#include <stdlib.h>

#include <readline/history.h>
#include <readline/readline.h>
#include "parse.h"

/* - - - - - - - - START OUR CODE - - - - - - - - */
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "wait.h"

#define R 0
#define W 1
/* - - - - - - - - END OUR CODE - - - - - - - - */

/*
 * Function declarations
 */
void PrintCommand(int, Command *);
void PrintPgm(Pgm *);
void stripwhite(char *);

/* - - - - - - - - START OUR CODE - - - - - - - - */
void sigchild_handler(int sig);
void sigint_handler(int sig);
void exec_cd(char **args);
void exec_commands(Command *cmd);
void redirect_rstd(Command *cmd);
void redirect_io(int fd, int io);
void pipe_commands(Pgm *pgm);
void _wait();
void exec_args(char **args);
/* - - - - - - - - END OUR CODE - - - - - - - - */

/* When non-zero, this global means the user is done using this program. */
int done = 0;

/*
 * Gets the ball rolling...
 */
int main(void) {
  Command cmd;
  int n;

  /* - - - - - - - - START OUR CODE - - - - - - - - */
  signal(SIGCHLD, sigchild_handler);

  // Don't kill lsh
  signal(SIGINT, SIG_IGN);
  char **args;
  /* - - - - - - - - END OUR CODE - - - - - - - - */

  while (!done) {
    char *line;
    line = readline("> ");

    if (!line) {
      /* Encountered EOF at top level */
      done = 1;
    } else {
      /*
       * Remove leading and trailing whitespace from the line
       * Then, if there is anything left, add it to the history list
       * and execute it.
       */
      stripwhite(line);

      if (*line) {
        add_history(line);
        /* execute it */
        n = parse(line, &cmd);
        // PrintCommand(n, &cmd);

        /* - - - - - - - - START OUR CODE - - - - - - - - */
        if (n > 0) {
          args = (&cmd)->pgm->pgmlist;

          if (!strcmp(*args, "exit")) {
            exit(0);

          } else if (!strcmp(*args, "cd")) {
            exec_cd(args);

          } else {
            exec_commands(&cmd);
          }
        }
        /* - - - - - - - - END OUR CODE - - - - - - - - */
      }
    }

    if (line) {
      free(line);
    }
  }

  return 0;
}

/* - - - - - - - - START OUR CODE - - - - - - - - */
/*
 * Handle SIGCHLD signal
 */
void sigchild_handler(int sig) {
  while ((waitpid(-1, NULL, WNOHANG) > 0)) {
  }
}

/*
 * Execute 'cd' command
 */
void exec_cd(char **args) {
  char *dir = *(++args);

  if (chdir(dir) == -1) {
    perror("chdir()");
  }
}

/* Fork commands */
void exec_commands(Command *cmd) {
  Pgm *pgm = cmd->pgm;

  switch (fork()) {
    case -1:
      perror("fork()");
      break;

    case 0:
      if (cmd->bakground == 1) {
        /* Don't kill background process */
        signal(SIGINT, SIG_IGN);

      } else {
        signal(SIGINT, SIG_DFL);
      }

      redirect_rstd(cmd);
      pipe_commands(pgm);
      break;

    default:
      if (cmd->bakground == 0) {
        _wait();
      }
      break;
  }
}

/* Handle '<' and '>' command */
void redirect_rstd(Command *cmd) {
  int fd;

  if (cmd->rstdout != NULL) {
    fd = open(cmd->rstdout, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    redirect_io(fd, 1);
  }

  if (cmd->rstdin != NULL) {
    fd = open(cmd->rstdin, O_RDONLY);
    redirect_io(fd, 0);
  }
}

/* Redirect input/output */
void redirect_io(int fd, int io) {
  if (fd == -1) {
    perror("open()");
  }

  if (dup2(fd, io) == -1) {
    perror("dup2()");

  } else {
    close(fd);
  }
}

/* Digs recursively to last pgm which holds the first command */
void pipe_commands(Pgm *pgm) {
  Pgm *next = pgm->next;

  if (next == NULL) {
    /* Last pgm */
    exec_args(pgm->pgmlist);
    return;
  }

  int pipefd[2];
  if (pipe(pipefd)) {
    perror("pipe()");
  }

  switch (fork()) {
    case -1:
      perror("fork()");
      break;

    /* Make sure child can write to parent */
    case 0:
      close(STDOUT_FILENO);
      redirect_io(pipefd[W], STDOUT_FILENO);
      close(pipefd[R]);

      pipe_commands(next);
      break;

    /* Make sure parent can read from child */
    default:
      close(STDIN_FILENO);
      redirect_io(pipefd[R], STDIN_FILENO);
      close(pipefd[W]);

      _wait();

      exec_args(pgm->pgmlist);
      break;
  }
}

/* Wait for child */
void _wait() {
  if (wait(NULL) == -1) {
    perror("wait()");
  }
}

/* Execute commands */
void exec_args(char **args) {
  if (execvp(args[0], args) == -1) {
    perror("execvp()");
    exit(1);
  }
}
/* - - - - - - - - END OUR CODE - - - - - - - - */

/*
 * Prints a Command structure as returned by parse on stdout
 */
void PrintCommand(int n, Command *cmd) {
  printf("Parse returned %d:\n", n);
  printf("   stdin : %s\n", cmd->rstdin ? cmd->rstdin : "<none>");
  printf("   stdout: %s\n", cmd->rstdout ? cmd->rstdout : "<none>");
  printf("   bg    : %s\n", cmd->bakground ? "yes" : "no");
  PrintPgm(cmd->pgm);
}

/*
 * Prints a list of Pgm:s
 */
void PrintPgm(Pgm *p) {
  if (p == NULL) {
    return;
  } else {
    char **pl = p->pgmlist;

    /*
     * The list is in reversed order so print
     * it reversed to get right
     */
    PrintPgm(p->next);
    printf("    [");
    while (*pl) {
      printf("%s ", *pl++);
    }
    printf("]\n");
  }
}

/*
 * Strip whitespace from the start and end of STRING.
 */
void stripwhite(char *string) {
  register int i = 0;

  while (isspace(string[i])) {
    i++;
  }

  if (i) {
    strcpy(string, string + i);
  }

  i = strlen(string) - 1;
  while (i > 0 && isspace(string[i])) {
    i--;
  }

  string[++i] = '\0';
}
