/*
 * client.c -- TCP Socket Client
 *
 * adapted from:
 *   https://www.educative.io/answers/how-to-implement-tcp-sockets-in-c
 */

#include "Client.h"
#include <arpa/inet.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

/*This function handles the write command from client side. It takes
local path and remote path as inputs and writes the file from local to remote.
Note:- If remote path is not provided, it creates a file same as local path.
(We pass local_path value to remote_path in such scenarios)
*/
void write_command(int socket_desc, char *local_path, char *remote_path) {
  char client_message[2000];
  FILE *file;
  struct stat file_stat;
  long file_size;
  char *file_data;

  // Open the file
  file = fopen(local_path, "rb");
  if (!file) {
    perror("Error opening file");
    return;
  }

  // Get the size of the file
  stat(local_path, &file_stat);
  file_size = file_stat.st_size;

  // Allocate memory for file data
  file_data = (char *)malloc(file_size + 1);
  if (!file_data) {
    perror("Memory allocation failed");
    fclose(file);
    return;
  }

  // Read file data
  fread(file_data, file_size, 1, file);
  fclose(file);

  // Prepare the message
  snprintf(client_message, sizeof(client_message), "WRITE %s %s", remote_path,
           file_data);
  free(file_data);

  // Send the message to the server
  if (send(socket_desc, client_message, strlen(client_message), 0) < 0) {
    perror("Unable to send message");
    return;
  }

  // Clear the buffer to receive the server's response
  memset(client_message, '\0', sizeof(client_message));
  if (recv(socket_desc, client_message, sizeof(client_message), 0) < 0) {
    perror("Error while receiving server's response");
    return;
  }

  printf("Server's response: %s\n", client_message);
}

/*
 * This function handles the GET command from the client side.
 * It takes the server socket descriptor, client message, server message,
 * and local file path as inputs and retrieves the file from the server.
 */
void get_command(int socket_desc, char *client_message, char *server_message,
                 char *local_path) {
  // Send the GET request
  if (send(socket_desc, client_message, strlen(client_message), 0) < 0) {
    perror("Unable to send message");
    return;
  }

  FILE *file1 = fopen(local_path, "wb");
  if (file1 == NULL) {
    perror("Error opening File");
    return;
  }

  int bytes_received;
  while ((bytes_received = recv(socket_desc, server_message,
                                sizeof(server_message), 0)) > 0) {
    fwrite(server_message, 1, bytes_received, file1);
  }
  fclose(file1);
  printf("File received and saved to %s\n", local_path);
}

char *extract_file_name(const char *file_path) {
  // Create a copy of the file_path to avoid modifying the original string
  char *path_copy = strdup(file_path);
  if (path_copy == NULL) {
    // Handle memory allocation error
    return NULL;
  }

  // Extract the base name
  char *file_name = basename(path_copy);

  // Make a copy of the file name to return
  char *result = strdup(file_name);

  // Free the path_copy as it's no longer needed
  free(path_copy);

  return result;
}

int main(int argc, char *argv[]) {
  int socket_desc;
  struct sockaddr_in server_addr;
  char server_message[2000], client_message[2000];

  if (argc < 3 && (strcmp(argv[1], "STOP") != 0)) {
    printf("Usage: %s COMMAND remote-file-path\n", argv[0]);
    return -1;
  }

  // Clean buffers:
  memset(server_message, '\0', sizeof(server_message));
  memset(client_message, '\0', sizeof(client_message));

  // Create socket:
  socket_desc = socket(AF_INET, SOCK_STREAM, 0);

  if (socket_desc < 0) {
    printf("Unable to create socket\n");
    return -1;
  }

  printf("Socket created successfully\n");

  // Set port and IP the same as server-side:
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(2000);
  server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

  // Send connection request to server:
  if (connect(socket_desc, (struct sockaddr *)&server_addr,
              sizeof(server_addr)) < 0) {
    printf("Unable to connect\n");
    return -1;
  }
  printf("Connected with server successfully\n");

  if (strcmp(argv[1], "WRITE") == 0) {
    // Send the WRITE command and the remote file path to the server

    if (argc < 3) {
      printf("Usage for WRITE: %s WRITE <local-file-path> "
             "<remote-file-path>(optional)\n",
             argv[0]);
      return -1;
    }

    write_command(socket_desc, argv[2], argc == 4 ? argv[3] : argv[2]);
  }

  if (strcmp(argv[1], "GET") == 0) {
    // Send the GET command and the remote file path to the server
    if (argc == 5) {
      // Command, remote path, local path, and version are provided
      snprintf(client_message, sizeof(client_message), "%s %s %s %s", argv[1],
               argv[2], argv[3], argv[4]);
    } else if (argc == 4) {
      // Command, remote path, and either local path or version are provided
      snprintf(client_message, sizeof(client_message), "%s %s %s", argv[1],
               argv[2], argv[3]);
    } else {
      // Only command and remote path are provided
      snprintf(client_message, sizeof(client_message), "%s %s", argv[1],
               argv[2]);
    }
    char *name = extract_file_name(argv[2]);
    char filename[1024]; // Adjust the size as needed
    snprintf(filename, sizeof(filename), "./%s", name);

    char *local_filepath = argv[3] != NULL ? argv[3] : filename;

    get_command(socket_desc, client_message, server_message, local_filepath);
    //  Send the client message to the server
  }

  if (strcmp(argv[1], "LS") == 0) {
    // Send the GET command and the remote file path to the server
    snprintf(client_message, sizeof(client_message), "%s %s", argv[1], argv[2]);

    if (send(socket_desc, client_message, strlen(client_message), 0) < 0) {
      printf("Unable to send message\n");
      return -1;
    }
    while (recv(socket_desc, server_message, sizeof(server_message), 0) > 0) {
      printf("%s", server_message); // Print to stdout
      memset(server_message, '\0',
             sizeof(server_message)); // Clear buffer for next message
    }
    printf("\n");
  }

  if (strcmp(argv[1], "RM") == 0) {
    snprintf(client_message, sizeof(client_message), "%s %s", argv[1], argv[2]);

    if (send(socket_desc, client_message, strlen(client_message), 0) < 0) {
      printf("Unable to send message\n");
      return -1;
    }
    if (recv(socket_desc, server_message, sizeof(server_message), 0) < 0) {
      printf("Error while receiving server's response\n");
      return -1;
    }
    printf("Server's response to DELETE: %s\n", server_message);
  }

  if (strcmp(argv[1], "STOP") == 0) {
    // Handle STOP command
    snprintf(client_message, sizeof(client_message), "%s", argv[1]);
    if (send(socket_desc, client_message, strlen(client_message), 0) < 0) {
      printf("Unable to send message\n");
      return -1;
    }
    if (recv(socket_desc, server_message, sizeof(server_message), 0) < 0) {
      printf("Error while receiving server's response\n");
      return -1;
    }
    printf("server's response to STOP: %s\n", server_message);
  }

  // Close the socket:
  close(socket_desc);

  return 0;
}
