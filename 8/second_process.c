#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#define FIFO_TO_SECOND "fifo_to_second_process"
#define FIFO_FROM_SECOND "fifo_from_second_process"
#define BUFFER_SIZE 5000

int main() {
  int from_first_fd = open(FIFO_TO_SECOND, O_RDONLY);
  if (from_first_fd < 0) {
    perror("Cannot open FIFO to second process");
    exit(EXIT_FAILURE);
  }

  if (mkfifo(FIFO_FROM_SECOND, 0666) == -1) {
    perror("mkfifo");
    close(from_first_fd);
    exit(EXIT_FAILURE);
  }

  char buffer[BUFFER_SIZE];
  ssize_t num_read = read(from_first_fd, buffer, BUFFER_SIZE);
  if (num_read < 0) {
    perror("Cannot read from FIFO to second process");
    close(from_first_fd);
    unlink(FIFO_TO_SECOND);
    exit(EXIT_FAILURE);
  }
  close(from_first_fd);

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

  int to_first_fd = open(FIFO_FROM_SECOND, O_WRONLY);
  if (to_first_fd < 0) {
    perror("Cannot open FIFO from second process");
    unlink(FIFO_TO_SECOND);
    unlink(FIFO_FROM_SECOND);
    exit(EXIT_FAILURE);
  }

  if (write(to_first_fd, buffer, bytes_to_write) != bytes_to_write) {
    perror("Cannot write to FIFO from second process");
    close(to_first_fd);
    unlink(FIFO_TO_SECOND);
    unlink(FIFO_FROM_SECOND);
    exit(EXIT_FAILURE);
  }

  close(to_first_fd);
  unlink(FIFO_TO_SECOND);
  unlink(FIFO_FROM_SECOND);

  return EXIT_SUCCESS;
}