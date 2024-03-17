#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUFFER_SIZE 5000

void first_process(int read_pipe[2], int write_pipe[2], const char *input_file,
                   const char *output_file) {
  close(write_pipe[0]);
  close(read_pipe[1]);

  int input_fd = open(input_file, O_RDONLY);
  if (input_fd < 0) {
    perror("Unable to open the input file");
    exit(EXIT_FAILURE);
  }

  char buffer[BUFFER_SIZE];
  int read_bytes = read(input_fd, buffer, BUFFER_SIZE);
  if (read_bytes < 0) {
    perror("Unable to read");
    exit(EXIT_FAILURE);
  }
  if (write(write_pipe[1], buffer, read_bytes) != read_bytes) {
    perror("Unable to write to pipe");
    exit(EXIT_FAILURE);
  }

  close(input_fd);
  close(write_pipe[1]);

  int output_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (output_fd < 0) {
    perror("Unable to open the output file");
    exit(EXIT_FAILURE);
  }

  while ((read_bytes = read(read_pipe[0], buffer, BUFFER_SIZE)) > 0) {
    if (write(output_fd, buffer, read_bytes) != read_bytes) {
      perror("Unable to write to the output file");
      exit(EXIT_FAILURE);
    }
  }

  close(output_fd);
  close(read_pipe[0]);
  exit(EXIT_SUCCESS);
}

void second_process(int read_pipe[2], int write_pipe[2]) {
  close(read_pipe[1]);
  close(write_pipe[0]);

  int sum = 0;
  char buffer[BUFFER_SIZE];
  int read_bytes;

  while ((read_bytes = read(read_pipe[0], buffer, BUFFER_SIZE)) > 0) {
    for (int i = 0; i < read_bytes; ++i) {
      if ('0' <= buffer[i] && buffer[i] <= '9') {
        sum += buffer[i] - '0';
      }
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
  if (write(write_pipe[1], buffer, bytes_to_write) < 0) {
    perror("Unable to write to pipe");
    exit(EXIT_FAILURE);
  }

  close(read_pipe[0]);
  close(write_pipe[1]);
  exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <input_file> <output_file>\n", argv[0]);
    return EXIT_FAILURE;
  }

  int pipe_to_process[2];
  int pipe_from_process[2];

  if (pipe(pipe_to_process) == -1 || pipe(pipe_from_process) == -1) {
    perror("pipe");
    exit(EXIT_FAILURE);
  }

  pid_t pid = fork();
  if (pid == -1) {
    perror("fork");
    exit(EXIT_FAILURE);
  } else if (pid == 0) {
    first_process(pipe_from_process, pipe_to_process, argv[1], argv[2]);
  } else {
    pid_t pid_process = fork();
    if (pid_process == -1) {
      perror("fork");
      exit(EXIT_FAILURE);
    } else if (pid_process == 0) {
      second_process(pipe_to_process, pipe_from_process);
    } else {
      int status;
      close(pipe_to_process[0]);
      close(pipe_to_process[1]);
      close(pipe_from_process[0]);
      close(pipe_from_process[1]);

      waitpid(pid, &status, 0);
      waitpid(pid_process, &status, 0);
    }
  }

  return EXIT_SUCCESS;
}
