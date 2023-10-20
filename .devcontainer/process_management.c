#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>

#define MAX_COMMANDS 100
#define MAX_COMMAND_LENGTH 100
#define MAX_OUTPUT_LENGTH 1000

void writeOutput(char *command, char *output) {
    FILE *fp;
    fp = fopen("output.txt", "a");
    fprintf(fp, "The output of: %s is\n", command);
    fprintf(fp, ">>>>>>>>>>>>>>>\n%s<<<<<<<<<<<<<<<\n", output);
    fclose(fp);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    int shm_fd = shm_open("commands", O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, MAX_COMMANDS * MAX_COMMAND_LENGTH * sizeof(char));
    char (*shared_memory)[MAX_COMMAND_LENGTH] = mmap(0, MAX_COMMANDS * MAX_COMMAND_LENGTH * sizeof(char), PROT_WRITE, MAP_SHARED, shm_fd, 0);

    if (fork() == 0) { // Child process to read file content
        FILE *file = fopen(argv[1], "r");
        if (file == NULL) {
            perror("Error opening file");
            exit(1);
        }

        int index = 0;
        while (fgets(shared_memory[index], MAX_COMMAND_LENGTH, file) != NULL && index < MAX_COMMANDS) {
            shared_memory[index][strcspn(shared_memory[index], "\n")] = 0; // Remove newline character
            index++;
        }
        fclose(file);
        exit(0);
    }

    wait(NULL); // Wait for child process to complete

    char commands[MAX_COMMANDS][MAX_COMMAND_LENGTH];
    int index = 0;
    while (strlen(shared_memory[index]) > 0 && index < MAX_COMMANDS) {
        strcpy(commands[index], shared_memory[index]);
        index++;
    }

    shm_unlink("commands"); // Remove shared memory object

    for (int i = 0; i < index; i++) {
        int pipefd[2];
        pipe(pipefd);

        if (fork() == 0) { // Child process to execute commands
            close(pipefd[0]); // Close read end
            dup2(pipefd[1], STDOUT_FILENO); // Redirect stdout to write end of pipe
            close(pipefd[1]);

            char *args[] = {"/bin/sh", "-c", commands[i], NULL};
            execvp(args[0], args);
            perror("Error executing command");
            exit(0);
        }

        close(pipefd[1]); // Close write end
        char output[MAX_OUTPUT_LENGTH];
        read(pipefd[0], output, MAX_OUTPUT_LENGTH); // Read command output from pipe
        close(pipefd[0]);

        wait(NULL); // Wait for child process to complete

        writeOutput(commands[i], output); // Write command output to file
    }

    return 0;
}
