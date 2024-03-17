#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define FIFO_IN "fifo1"
#define FIFO_OUT "fifo2"

#define hndl(code) if (code < 0)

const size_t BUFFER_SIZE = 5000;

void read_process(const char *input_file) {
  int in_fd = open(input_file, O_RDONLY);
  int out_fd = open(FIFO_IN, O_WRONLY);
  hndl(in_fd) {
    perror("open");
    close(in_fd);
    close(out_fd);
    exit(EXIT_FAILURE);
  }
  hndl(out_fd) {
    perror("open");
    close(in_fd);
    close(out_fd);
    exit(EXIT_FAILURE);
  }

  char buffer[BUFFER_SIZE];
  int bytes_read = read(in_fd, buffer, BUFFER_SIZE);
  hndl(bytes_read) {
    perror("read");
    close(in_fd);
    close(out_fd);
    exit(EXIT_FAILURE);
  }

  int bytes_written = write(out_fd, buffer, bytes_read);
  hndl(bytes_written) {
    perror("write");
    close(in_fd);
    close(out_fd);
    exit(EXIT_FAILURE);
  }

  close(in_fd);
  close(out_fd);
  exit(EXIT_SUCCESS);
}

void find_sum_process() {
  int in_fd = open(FIFO_IN, O_RDONLY);
  int out_fd = open(FIFO_OUT, O_WRONLY);
  hndl(in_fd) {
    perror("open");
    close(in_fd);
    close(out_fd);
    exit(EXIT_FAILURE);
  }
  hndl(out_fd) {
    perror("open");
    close(in_fd);
    close(out_fd);
    exit(EXIT_FAILURE);
  }

  char buffer[BUFFER_SIZE];
  int bytes_read = read(in_fd, buffer, BUFFER_SIZE);
  hndl(bytes_read) {
    perror("read");
    close(in_fd);
    close(out_fd);
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

  int bytes_written = write(out_fd, buffer, bytes_to_write);
  hndl(bytes_written) {
    perror("write");
    close(in_fd);
    close(out_fd);
    exit(EXIT_FAILURE);
  }

  close(in_fd);
  close(out_fd);
  exit(EXIT_SUCCESS);
}

void write_process(const char *output_file) {
  int in_fd = open(FIFO_OUT, O_RDONLY);
  int out_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  hndl(in_fd) {
    perror("open");
    close(in_fd);
    close(out_fd);
    exit(EXIT_FAILURE);
  }
  hndl(out_fd) {
    perror("open");
    close(in_fd);
    close(out_fd);
    exit(EXIT_FAILURE);
  }

  char buffer[BUFFER_SIZE];
  int bytes_read = read(in_fd, buffer, BUFFER_SIZE);
  hndl(bytes_read) {
    perror("read");
    close(in_fd);
    close(out_fd);
    exit(EXIT_FAILURE);
  }

  int bytes_written = write(out_fd, buffer, bytes_read);
  hndl(bytes_written) {
    perror("write");
    close(in_fd);
    close(out_fd);
    exit(EXIT_FAILURE);
  }

  close(in_fd);
  close(out_fd);
  exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <input_file> <output_file>\n", argv[0]);
    return EXIT_FAILURE;
  }

  if (mkfifo(FIFO_IN, 0666) < 0 || mkfifo(FIFO_OUT, 0666) < 0) {
    perror("mkfifo");
    return EXIT_FAILURE;
  }

  pid_t read_pid = fork();
  if (read_pid == 0) {
    read_process(argv[1]);
  }

  pid_t process_pid = fork();
  if (process_pid == 0) {
    find_sum_process();
  }

  pid_t write_pid = fork();
  if (write_pid == 0) {
    write_process(argv[2]);
  }

  waitpid(read_pid, NULL, 0);
  waitpid(process_pid, NULL, 0);
  waitpid(write_pid, NULL, 0);

  unlink(FIFO_IN);
  unlink(FIFO_OUT);

  return EXIT_SUCCESS;
}
