#include "fcntl.h"
#include "stdio.h"
#include "stdlib.h"
#include "sys/wait.h"
#include "unistd.h"

#define hndl(code) if (code < 0)

const size_t BUFFER_SIZE = 5000;

typedef int Pipe[2];

#define pipe_close_read(pipe) close(pipe[0])

#define pipe_close_write(pipe) close(pipe[1])

void read_process(Pipe pipe, const char *input_file) {
  pipe_close_read(pipe);
  int descriptor = open(input_file, O_RDONLY);

  hndl(descriptor) {
    perror("open");
    exit(EXIT_FAILURE);
  }

  char buffer[BUFFER_SIZE];
  int bytes_read = read(descriptor, buffer, BUFFER_SIZE);
  hndl(bytes_read) {
    perror("read");
    close(descriptor);
    exit(EXIT_FAILURE);
  }

  int bytes_written = write(pipe[1], buffer, bytes_read);
  hndl(bytes_written) {
    perror("write");
    close(descriptor);
    exit(EXIT_FAILURE);
  }

  close(descriptor);
  pipe_close_write(pipe);
  exit(EXIT_SUCCESS);
}

void find_sum_process(Pipe pipe_in, Pipe pipe_out) {
  pipe_close_write(pipe_in);
  pipe_close_read(pipe_out);

  char buffer[BUFFER_SIZE];
  int bytes_read = read(pipe_in[0], buffer, BUFFER_SIZE);
  hndl(bytes_read) {
    perror("read");
    pipe_close_read(pipe_in);
    exit(EXIT_FAILURE);
  }

  size_t sum = 0;
  for (size_t i = 0; i < bytes_read; ++i) {
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
  int bytes_written = write(pipe_out[1], buffer, bytes_to_write);
  hndl(bytes_written) {
    perror("write");
    exit(EXIT_FAILURE);
  }

  pipe_close_read(pipe_in);
  pipe_close_write(pipe_out);
  exit(EXIT_SUCCESS);
}

void write_process(Pipe pipe, const char *output_file) {
  pipe_close_write(pipe);
  int descriptor = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  hndl(descriptor) {
    perror("open");
    close(descriptor);
    exit(EXIT_FAILURE);
  }

  char buffer[BUFFER_SIZE];
  int bytes_read = read(pipe[0], buffer, BUFFER_SIZE);
  hndl(bytes_read) {
    perror("read");
    close(descriptor);
    exit(EXIT_FAILURE);
  }

  int bytes_written = write(descriptor, buffer, bytes_read);
  hndl(bytes_written) {
    perror("write");
    close(descriptor);
    pipe_close_read(pipe);
  }
  close(descriptor);
  pipe_close_read(pipe);
  exit(EXIT_SUCCESS);
}

int main(int argc, const char *argv[1]) {
  if (argc != 3) {
    fprintf(stderr, "usage: %s <input_file> <output_file>\n", argv[0]);
    exit(EXIT_FAILURE);
  }
  Pipe pipe_in, pipe_out;
  if (pipe(pipe_in) == -1 || pipe(pipe_out) == -1) {
    perror("pipe");
    exit(EXIT_FAILURE);
  }

  pid_t first_pid = fork(), second_pid;
  hndl(first_pid) {
    perror("fork");
    exit(EXIT_FAILURE);
  }

  if (first_pid == 0) {
    read_process(pipe_in, argv[1]);
  } else {
    second_pid = fork();
    hndl(second_pid) {
      perror("fork");
      exit(EXIT_FAILURE);
    }
    if (second_pid == 0) {
      find_sum_process(pipe_in, pipe_out);
    }
  }

  pipe_close_read(pipe_in);
  pipe_close_write(pipe_in);

  if (first_pid > 0 && second_pid > 0) {
    pid_t third_pid = fork();
    hndl(third_pid) {
      perror("fork");
      exit(EXIT_FAILURE);
    }

    if (third_pid == 0) {
      write_process(pipe_out, argv[2]);
    }

    pipe_close_read(pipe_out);
    pipe_close_write(pipe_out);

    int status;
    waitpid(first_pid, &status, 0);
    waitpid(second_pid, &status, 0);
    waitpid(third_pid, &status, 0);
  }
}
