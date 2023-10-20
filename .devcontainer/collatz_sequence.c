#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_SEQUENCE 100
#define MAX_NUMBERS 10

void generate_collatz_sequence(int n, int *sequence, int *size) {
    int index = 0;
    while (n != 1 && index < MAX_SEQUENCE - 1) {
        sequence[index++] = n;
        if (n % 2 == 0) {
            n /= 2;
        } else {
            n = 3*n + 1;
        }
    }
    sequence[index++] = 1; // Add 1 to the end of the sequence
    *size = index; // Set the size of the sequence
}

int main() {
    int start_numbers[MAX_NUMBERS];
    int count = 0;

    // Read start numbers from file
    FILE *file = fopen("start_numbers.txt", "r");
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }
    while (fscanf(file, "%d", &start_numbers[count]) != EOF && count < MAX_NUMBERS) {
        count++;
    }
    fclose(file);

    for (int i = 0; i < count; i++) {
        int sequence[MAX_SEQUENCE];
        int size;
        generate_collatz_sequence(start_numbers[i], sequence, &size);

        // Create shared memory object
        int shm_fd = shm_open("collatz", O_CREAT | O_RDWR, 0666);
        ftruncate(shm_fd, MAX_SEQUENCE * sizeof(int));
        int *shared_memory = mmap(0, MAX_SEQUENCE * sizeof(int), PROT_WRITE, MAP_SHARED, shm_fd, 0);

        // Copy the sequence to shared memory
        for (int j = 0; j < size; j++) {
            shared_memory[j] = sequence[j];
        }

        if (fork() == 0) { // Child process
            printf("Child Process: The generated collatz sequence is ");
            for (int j = 0; j < size; j++) {
                printf("%d ", shared_memory[j]);
            }
            printf("\n");

            // Remove shared memory object
            shm_unlink("collatz");
            exit(0);
        } else { // Parent process
            printf("Parent Process: The positive integer read from file is %d\n", start_numbers[i]);
            wait(NULL); // Wait for child process to complete
        }
    }

    return 0;
}
