#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define hndl(code) if (code < 0)

#define FIFO_TO_SECOND "fifo_to_second_process"
#define FIFO_FROM_SECOND "fifo_from_second_process"

#define BUFFER_SIZE 5000

void first_process(const char *input_file, const char *output_file) {
  int in_fd = open(input_file, O_RDONLY);
  hndl(in_fd) {
    perror("Cannot open input file");
    exit(EXIT_FAILURE);
  }

  int to_second_fd = open(FIFO_TO_SECOND, O_WRONLY);
  hndl(to_second_fd) {
    perror("Cannot open FIFO to second process");
    close(in_fd);
    close(to_second_fd);
    exit(EXIT_FAILURE);
  }

  char buffer[BUFFER_SIZE];

  ssize_t num_read = read(in_fd, buffer, BUFFER_SIZE);
  hndl(num_read) {
    perror("Cannot read from file");
    close(in_fd);
    close(to_second_fd);
    exit(EXIT_FAILURE);
  }

  if (write(to_second_fd, buffer, num_read) != num_read) {
    perror("Cannot write to FIFO to second process");
    close(in_fd);
    close(to_second_fd);
    exit(EXIT_FAILURE);
  }

  close(in_fd);
  close(to_second_fd);

  int from_second_fd = open(FIFO_FROM_SECOND, O_RDONLY);
  hndl(from_second_fd) {
    perror("Cannot open FIFO from second process");
    exit(EXIT_FAILURE);
  }

  int out_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  hndl(out_fd) {
    perror("Cannot open output file");
    close(from_second_fd);
    exit(EXIT_FAILURE);
  }

  num_read = read(from_second_fd, buffer, BUFFER_SIZE);
  hndl(num_read) {
    perror("Cannot from second FIFO");
    close(from_second_fd);
  }

  if (write(out_fd, buffer, num_read) != num_read) {
    perror("Cannot write to output file");
    close(from_second_fd);
    close(out_fd);
    exit(EXIT_FAILURE);
  }

  close(from_second_fd);
  close(out_fd);
}

void second_process(void) {
  int from_first_fd = open(FIFO_TO_SECOND, O_RDONLY);
  hndl(from_first_fd) {
    perror("Cannot open FIFO to second process");
    exit(EXIT_FAILURE);
  }

  int to_first_fd = open(FIFO_FROM_SECOND, O_WRONLY);
  hndl(to_first_fd) {
    perror("Cannot open FIFO from second process");
    close(from_first_fd);
    exit(EXIT_FAILURE);
  }

  char buffer[BUFFER_SIZE];

  ssize_t num_read = read(from_first_fd, buffer, BUFFER_SIZE);
  hndl(num_read) {
    perror("Cannot read from FIFO to second process");
    close(from_first_fd);
    close(to_first_fd);
    exit(EXIT_FAILURE);
  }
  int sum = 0;
  for (int i = 0; i < num_read; ++i) {
    if ('0' <= buffer[i] && buffer[i] <= '9') {
      sum += buffer[i] - '0';
    }
  }
  size_t bytes_to_write = 0;
  if (sum == 0) {
    buffer[0] = '0';
    bytes_to_write = 1;
  } else {
    while (sum != 0) {
      buffer[bytes_to_write++] = (sum % 10) + '0';
      sum /= 10;
    }
  }
  for (int i = 0; i < bytes_to_write / 2; ++i) {
    buffer[i] ^= buffer[bytes_to_write - i - 1];
    buffer[bytes_to_write - i - 1] ^= buffer[i];
    buffer[i] ^= buffer[bytes_to_write - i - 1];
  }

  if (write(to_first_fd, buffer, bytes_to_write) != bytes_to_write) {
    perror("Cannot write to FIFO from second process");
    close(from_first_fd);
    close(to_first_fd);
    exit(EXIT_FAILURE);
  }

  close(from_first_fd);
  close(to_first_fd);
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <input_file> <output_file>\n", argv[0]);
    return EXIT_FAILURE;
  }

  if (mkfifo(FIFO_TO_SECOND, 0666) < 0) {
    perror("mkfifo");
    exit(EXIT_FAILURE);
  }
  if (mkfifo(FIFO_FROM_SECOND, 0666) < 0) {
    perror("mkfifo");
    unlink(FIFO_TO_SECOND);
    exit(EXIT_FAILURE);
  }

  pid_t pid = fork();

  if (pid < 0) {
    perror("fork");
    unlink(FIFO_TO_SECOND);
    unlink(FIFO_FROM_SECOND);
    exit(EXIT_FAILURE);
  } else if (pid == 0) {
    second_process();
    exit(EXIT_SUCCESS);
  } else {
    first_process(argv[1], argv[2]);
    wait(NULL);
    unlink(FIFO_TO_SECOND);
    unlink(FIFO_FROM_SECOND);
  }

  return EXIT_SUCCESS;
}