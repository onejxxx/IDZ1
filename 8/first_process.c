#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define FIFO_TO_SECOND "fifo_to_second_process"
#define FIFO_FROM_SECOND "fifo_from_second_process"
#define BUFFER_SIZE 5000

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input_file> <output_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *input_file = argv[1];
    const char *output_file = argv[2];

    int in_fd = open(input_file, O_RDONLY);
    if (in_fd < 0) {
        perror("Cannot open input file");
        exit(EXIT_FAILURE);
    }

    if (mkfifo(FIFO_TO_SECOND, 0666) == -1) {
        perror("mkfifo");
        close(in_fd);
        exit(EXIT_FAILURE);
    }

    int to_second_fd = open(FIFO_TO_SECOND, O_WRONLY);
    if (to_second_fd < 0) {
        perror("Cannot open FIFO to second process");
        close(in_fd);
        unlink(FIFO_TO_SECOND);
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    ssize_t num_read = read(in_fd, buffer, BUFFER_SIZE);
    if (num_read < 0) {
        perror("Cannot read input file");
        close(in_fd);
        close(to_second_fd);
        unlink(FIFO_TO_SECOND);
        exit(EXIT_FAILURE);
    }

    if (write(to_second_fd, buffer, num_read) != num_read) {
        perror("Cannot write to FIFO to second process");
        close(in_fd);
        close(to_second_fd);
        unlink(FIFO_TO_SECOND);
        exit(EXIT_FAILURE);
    }

    close(to_second_fd);

    int from_second_fd = open(FIFO_FROM_SECOND, O_RDONLY);
    if (from_second_fd < 0) {
        perror("Cannot open FIFO from second process");
        unlink(FIFO_TO_SECOND);
        exit(EXIT_FAILURE);
    }

    num_read = read(from_second_fd, buffer, BUFFER_SIZE);
    if (num_read < 0) {
        perror("Cannot read from FIFO from second process");
        close(from_second_fd);
        unlink(FIFO_TO_SECOND);
        unlink(FIFO_FROM_SECOND);
        exit(EXIT_FAILURE);
    }

    int out_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (out_fd < 0) {
        perror("Cannot open output file");
        close(from_second_fd);
        unlink(FIFO_TO_SECOND);
        unlink(FIFO_FROM_SECOND);
        exit(EXIT_FAILURE);
    }

    if (write(out_fd, buffer, num_read) != num_read) {
        perror("Cannot write to output file");
        close(out_fd);
        close(from_second_fd);
        unlink(FIFO_TO_SECOND);
        unlink(FIFO_FROM_SECOND);
        exit(EXIT_FAILURE);
    }

    close(out_fd);
    close(from_second_fd);
    unlink(FIFO_TO_SECOND);
    unlink(FIFO_FROM_SECOND);

    return EXIT_SUCCESS;
}