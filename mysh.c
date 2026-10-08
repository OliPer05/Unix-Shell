#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <ctype.h>

#define MAX_QUOTES 10000
#define MAX_LINE 4096
#define MAX_CMDS 100
#define MAX_ARGS 100

char *quotes[MAX_QUOTES];
int numQuotes = 0;

/* remove leading/trailing whitespace */
char *trim(char *s) {
    while (isspace((unsigned char)*s)) s++;

    if (*s == '\0') return s;

    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }
    return s;
}

/* read all quotes at startup */
void loadQuotes(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("quotes.txt");
        exit(1);
    }

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), fp) && numQuotes < MAX_QUOTES) {
        line[strcspn(line, "\n")] = '\0';
        quotes[numQuotes] = malloc(strlen(line) + 1);
        if (!quotes[numQuotes]) {
            perror("malloc");
            fclose(fp);
            exit(1);
        }
        strcpy(quotes[numQuotes], line);
        numQuotes++;
    }

    fclose(fp);
}

/* print a random quote */
void printRandomQuote(void) {
    if (numQuotes > 0) {
        int idx = rand() % numQuotes;
        printf("%s\n", quotes[idx]);
    }
}

/* convert one command string into argv[] for execvp */
void parseArgs(char *cmd, char *argv[]) {
    int i = 0;
    char *token = strtok(cmd, " \t");
    while (token != NULL && i < MAX_ARGS - 1) {
        argv[i++] = token;
        token = strtok(NULL, " \t");
    }
    argv[i] = NULL;
}

/* execute a single simple command */
void runSingleCommand(char *cmd) {
    char *argv[MAX_ARGS];
    parseArgs(cmd, argv);

    if (argv[0] == NULL) return;

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return;
    }

    if (pid == 0) {
        execvp(argv[0], argv);
        perror("execvp");
        exit(1);
    } else {
        waitpid(pid, NULL, 0);
    }
}

/* execute command1 | command2 | command3 ... */
void runPipeline(char *line) {
    char *commands[MAX_CMDS];
    int n = 0;

    char *token = strtok(line, "|");
    while (token != NULL && n < MAX_CMDS) {
        commands[n++] = trim(token);
        token = strtok(NULL, "|");
    }

    if (n == 0) return;

    int pipes[MAX_CMDS - 1][2];

    for (int i = 0; i < n - 1; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe");
            return;
        }
    }

    for (int i = 0; i < n; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            return;
        }

        if (pid == 0) {
            if (i > 0) {
                dup2(pipes[i - 1][0], STDIN_FILENO);
            }
            if (i < n - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
            }

            for (int j = 0; j < n - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            char *argv[MAX_ARGS];
            parseArgs(commands[i], argv);

            if (argv[0] == NULL) exit(0);

            execvp(argv[0], argv);
            perror("execvp");
            exit(1);
        }
    }

    for (int i = 0; i < n - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    for (int i = 0; i < n; i++) {
        wait(NULL);
    }
}

/* execute command1 = command2 using two pipes */
void runTwoWayPipe(char *line) {
    char *left = strtok(line, "=");
    char *right = strtok(NULL, "=");

    if (left == NULL || right == NULL) return;

    left = trim(left);
    right = trim(right);

    int pipe1[2]; /* left stdout -> right stdin */
    int pipe2[2]; /* right stdout -> left stdin */

    if (pipe(pipe1) < 0 || pipe(pipe2) < 0) {
        perror("pipe");
        return;
    }

    pid_t pid1 = fork();
    if (pid1 < 0) {
        perror("fork");
        return;
    }

    if (pid1 == 0) {
        dup2(pipe2[0], STDIN_FILENO);
        dup2(pipe1[1], STDOUT_FILENO);

        close(pipe1[0]);
        close(pipe1[1]);
        close(pipe2[0]);
        close(pipe2[1]);

        char *argv1[MAX_ARGS];
        parseArgs(left, argv1);

        if (argv1[0] == NULL) exit(0);

        execvp(argv1[0], argv1);
        perror("execvp");
        exit(1);
    }

    pid_t pid2 = fork();
    if (pid2 < 0) {
        perror("fork");
        return;
    }

    if (pid2 == 0) {
        dup2(pipe1[0], STDIN_FILENO);
        dup2(pipe2[1], STDOUT_FILENO);

        close(pipe1[0]);
        close(pipe1[1]);
        close(pipe2[0]);
        close(pipe2[1]);

        char *argv2[MAX_ARGS];
        parseArgs(right, argv2);

        if (argv2[0] == NULL) exit(0);

        execvp(argv2[0], argv2);
        perror("execvp");
        exit(1);
    }

    close(pipe1[0]);
    close(pipe1[1]);
    close(pipe2[0]);
    close(pipe2[1]);

    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
}

int main(void) {
    srand((unsigned int)time(NULL));
    loadQuotes("quotes.txt");

    char line[MAX_LINE];

    while (1) {
        printRandomQuote();
        printf("# ");
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            break;
        }

        line[strcspn(line, "\n")] = '\0';

        char *input = trim(line);
        if (*input == '\0') continue;

        if (strcmp(input, "exit") == 0) break;

        char work[MAX_LINE];
        strcpy(work, input);

        if (strchr(work, '=')) {
            runTwoWayPipe(work);
        } else if (strchr(work, '|')) {
            runPipeline(work);
        } else {
            runSingleCommand(work);
        }
    }

    for (int i = 0; i < numQuotes; i++) {
        free(quotes[i]);
    }

    return 0;
}
