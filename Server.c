/*
 * server.c -- TCP Socket Server
 * adapted from:
 * https://www.educative.io/answers/how-to-implement-tcp-sockets-in-c
 */

#include "Server.h"
#include <arpa/inet.h>
#include <dirent.h>
#include <libgen.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

pthread_mutex_t file_mutex;
pthread_mutex_t file_write_mutex;
int server_stopped = 0;

/* The below method is used to extract timestamp information from the provided
  file. It takes the file name as an input and extracts the timestamp info
  associated with it.
  */
char *extract_timestamp_info(char *filename) {
  char *pos;
  pos = strstr(filename, "backup");

  if (pos) {
    // Calculate the position where the timestamp starts
    char *timestampPos = pos + strlen("backup");

    // Assuming the timestamp is 14 characters long (YYYYMMDDHHMMSS)
    char *formatted_timestamp = malloc(20); // Formatted timestamp length
    if (formatted_timestamp) {
      // Extract and format the timestamp
      snprintf(formatted_timestamp, 20, "%.4s-%.2s-%.2s %.2s:%.2s:%.2s",
               timestampPos, timestampPos + 4, timestampPos + 6,
               timestampPos + 8, timestampPos + 10, timestampPos + 12);

      return formatted_timestamp;
    }
  }
  return NULL;
}

/* This method lists all the previous veriosns of a provided current file.
  It takes the file path and count as inputs and returns the list of previous
  versions of that file.
  */
char **list_file_versions(const char *full_path, int *count) {
  char path_copy[1024];
  strncpy(path_copy, full_path, sizeof(path_copy) - 1);
  path_copy[sizeof(path_copy) - 1] = '\0';

  char *directory = dirname(strdup(full_path));
  char *full_base_filename = basename(strdup(full_path));

  // Remove file extension from base filename for matching
  char *dot = strchr(full_base_filename, '.');
  size_t base_len =
      dot ? (size_t)(dot - full_base_filename) : strlen(full_base_filename);

  DIR *dir;
  struct dirent *entry;
  char **file_list = NULL;
  *count = 0;

  printf("Debug: Directory = %s, Base Filename = %s\n", directory,
         full_base_filename);

  if ((dir = opendir(directory)) != NULL) {
    while ((entry = readdir(dir)) != NULL) {
      printf("Debug: Found file = %s\n", entry->d_name);

      // Check if the filename starts with the base filename (without extension)
      if (strncmp(entry->d_name, full_base_filename, base_len) == 0) {
        if (strstr(entry->d_name, "backup") != NULL) {
          printf("Debug: File matching base filename identified = %s\n",
                 entry->d_name);

          file_list = realloc(file_list, (*count + 1) * sizeof(char *));
          file_list[*count] = strdup(entry->d_name);
          (*count)++;
        }
      }
    }
    closedir(dir);
  } else {
    printf("Debug: Failed to open directory %s\n", directory);
  }

  return file_list;
}

/* This method is used to create a backup of a file if it already exists.
It creates the backup of the file by adding timestamp information as an unique
identifier with that file. It returns a versioned path with that unique
idetifier.*/

void create_versioned_backup(const char *original_path,
                             const char *versioned_path) {
  char backup_file_path[255];
  char base_file_name[255];
  char extension[50];

  // Split the original path into base name and extension
  char *ext_ptr = strrchr(original_path, '.');
  if (ext_ptr) {
    strncpy(extension, ext_ptr,
            sizeof(extension) - 1); // Copy the extension including the dot
    extension[sizeof(extension) - 1] = '\0'; // Ensure null-termination

    snprintf(base_file_name, ext_ptr - original_path + 1, "%s", original_path);
  } else {
    strncpy(extension, "", sizeof(extension)); // No extension
    strncpy(base_file_name, original_path, sizeof(base_file_name) - 1);
    base_file_name[sizeof(base_file_name) - 1] =
        '\0'; // Ensure null-termination
  }

  // Generate a timestamp
  time_t t = time(NULL);
  struct tm *tm = localtime(&t);
  char timestamp[64];
  strftime(timestamp, sizeof(timestamp), "%Y%m%d%H%M%S", tm);

  // Create backup file path with timestamp
  snprintf(versioned_path, sizeof(backup_file_path), "%sbackup%s%s",
           base_file_name, timestamp, extension);

  // Rename the original file to the backup path
  // rename(original_path, backup_file_path);
}
/* This method is used to identify if the provided directory path exists or not
 */
int isDirectory(const char *path) {
  struct stat statbuf;
  if (stat(path, &statbuf) != 0) {
    return 0;
  }
  return S_ISDIR(statbuf.st_mode);
}
/* This method is used for recursive deletion of a directory using 'rm' command
 * with the -r flag
 */
int remove_dir(const char *path) {
  // Using `system()` to run the 'rm' command with the '-r' flag for recursive
  // deletion
  char command[strlen(path) + 10];
  snprintf(command, sizeof(command), "rm -r \"%s\"", path);
  return system(command);
}
/* This file extracts the timestamp info for the latest file and stores it in
 * the server message */
void append_latest_file_info(const char *filepath, char *server_message) {
  struct stat fileInfo;
  if (stat(filepath, &fileInfo) == 0) {
    char modTimeStr[20];
    strftime(modTimeStr, 20, "%Y-%m-%d %H:%M:%S",
             localtime(&fileInfo.st_mtime));

    char latestFileInfo[1024];
    snprintf(latestFileInfo, sizeof(latestFileInfo),
             "Latest version: %s, Last modified: %s\n", basename(filepath),
             modTimeStr);

    strcat(server_message, latestFileInfo);
  } else {
    strcat(server_message, "Failed to retrieve latest file info.\n");
  }
}
/* This method is used to handle the arguemnts passed by a client
   Based on the each command passed by client, it performs some actions
  and append the result esponse into the server_message. In this method
  we check commands such as 'WRITE', 'GET', 'RM', 'LS', 'STOP' etc.
*/
void *client_handler(void *arguments) {
  struct handler_args *client_data = (struct handler_args *)arguments;
  int client_sock = client_data->client_sock;
  // free(client_data);

  char server_message[8196], client_message[8196];
  memset(server_message, '\0', sizeof(server_message));
  memset(client_message, '\0', sizeof(client_message));

  // Receive client's message:
  if (recv(client_sock, client_message, sizeof(client_message), 0) < 0) {
    printf("Couldn't receive\n");
    close(client_sock);
    return NULL;
  }

  char command[10], remote_file_path[200], local_file_path[200];
  int result = sscanf(client_message, "%9s %199s", command, remote_file_path);
  printf("Remote file path: %s\n", remote_file_path);

  if (result > 0) {
    if (strcmp(command, "WRITE") == 0) {
      pthread_mutex_lock(&file_write_mutex);
      char *file_data_start = strstr(client_message, remote_file_path);
      if (file_data_start != NULL) {
        file_data_start += strlen(remote_file_path);
        if (*file_data_start == ' ')
          file_data_start++; // Skip the space

        // trying out versioning with static file names
        FILE *file;
        int status = 0;
        char dir_path[255];
        strcpy(dir_path, remote_file_path);
        dirname(dir_path); // Extracts the directory part of the path

        // Check if the directory exists
        struct stat st = {0};
        if (stat(dir_path, &st) == -1) {
          // Directory does not exist, create it
          mkdir(dir_path, 0700); // Adjust permissions as needed
        }
        if (access(remote_file_path, F_OK) == 0) {

          char versioned_path[255];

          // Creating a new file with the new static path
          // char *temp = remote_file_path;
          create_versioned_backup(remote_file_path, versioned_path);
          rename(remote_file_path, versioned_path);
          file = fopen(remote_file_path, "wb");
          status = 1;

        } else {

          file = fopen(remote_file_path, "wb");
        }

        if (file) {
          fwrite(file_data_start, 1, strlen(file_data_start), file);
          fclose(file);
          snprintf(server_message, sizeof(server_message),
                   "File is written successfully in the server with "
                   "versioning.");
        } else {
          snprintf(server_message, sizeof(server_message),
                   "Error opening file.");
        }
      } else {
        snprintf(server_message, sizeof(server_message),
                 "Remote file path not found in message.");
      }
      pthread_mutex_unlock(&file_write_mutex);
    }

    if (strcmp(command, "GET") == 0) {
      char version[200] = {0};
      char local_file_path[200] = {0};
      int num_scanned = sscanf(client_message, "%*s %199s %199s %199s",
                               remote_file_path, local_file_path, version);

      printf("Debug: Client message = %s\n", client_message);

      int count;
      char **fileLists = list_file_versions(remote_file_path, &count);
      char file_path_with_version[400] = {0};
      int version_found = 0;

      if (num_scanned >= 3 && strlen(version) > 0) {
        char *dir = dirname(strdup(remote_file_path));
        for (int i = 0; i < count; i++) {
          if (strstr(fileLists[i], version) != NULL) {
            snprintf(file_path_with_version, sizeof(file_path_with_version),
                     "%s/%s", dir, fileLists[i]);
            version_found = 1;
            break;
          }
        }
        free(dir);
      }

      if (!version_found) {
        strncpy(file_path_with_version, remote_file_path,
                sizeof(file_path_with_version) - 1);
      }

      printf("Debug: Sending file from path = %s\n", file_path_with_version);
      FILE *file = fopen(file_path_with_version, "rb");
      if (file == NULL) {
        printf("Debug: Unable to open file at %s\n", file_path_with_version);
      } else {
        // Send file to the client
        int size;
        while ((size = fread(server_message, 1, sizeof(server_message), file)) >
               0) {
          send(client_sock, server_message, size, 0);
          memset(server_message, '\0', sizeof(server_message));
        }
        fclose(file);
      }

      // Free the file list
      for (int i = 0; i < count; i++) {
        free(fileLists[i]);
      }
      free(fileLists);
    }

    if (strcmp(command, "LS") == 0) {
      int count;
      char **fileLists = list_file_versions(remote_file_path, &count);

      // Append initial message or no files found message
      if (count == 0) {
        strcat(server_message, "No file versions found.\n");
      } else {
        snprintf(server_message, sizeof(server_message),
                 "%d Old file versions found:\n", count);
      }

      // Loop through each file and append information

      for (int i = 0; i < count; i++) {
        char temp[1024]; // Temporary buffer for a single line
        snprintf(temp, sizeof(temp), "File: %s", fileLists[i]);
        strcat(server_message, temp);

        // Extracting the timestamp info from the file name
        char *timestamp = extract_timestamp_info(fileLists[i]);
        if (timestamp) {
          snprintf(temp, sizeof(temp), ", Creation Timestamp: %s\n", timestamp);
          strcat(server_message, temp);
          free(timestamp); // Free the allocated memory
        } else {
          strcat(server_message,
                 ", Timestamp extraction failed or not present\n");
        }

        free(fileLists[i]); // Free the filename string
      }
      // Getting the timestamp info for the latest file
      append_latest_file_info(remote_file_path, server_message);
      free(fileLists); // Free the file list
    }

    if (strcmp(command, "RM") == 0) {
      int success = 1;
      if (isDirectory(remote_file_path)) {
        // If it's a directory, use remove_dir function
        success = remove_dir(remote_file_path) == 0;
      } else {
        // If it's a file, use remove function
        int count;
        char **fileLists = list_file_versions(remote_file_path, &count);
        if (fileLists != NULL) {
          char file_path_with_version[400] = {0};
          char *dir = dirname(strdup(remote_file_path));

          for (int i = 0; i < count; i++) {
            snprintf(file_path_with_version, sizeof(file_path_with_version),
                     "%s/%s", dir, fileLists[i]);
            if (remove(file_path_with_version) != 0) {
              success = 0; // Failed to remove at least one file
              printf("Failed to remove file: %s\n", file_path_with_version);
            }
            free(fileLists[i]);
          }

          free(fileLists);
          free(dir);
        }

        // Remove the original file (if it still exists)
        if (remove(remote_file_path) != 0) {
          success = 0;
          printf("Failed to remove original file: %s\n", remote_file_path);
        }
      }

      if (success) {
        snprintf(server_message, sizeof(server_message),
                 "Deletion sucessful for path:- %s", remote_file_path);
      } else {
        snprintf(server_message, sizeof(server_message),
                 "Deleteion failed for path:-%s", remote_file_path);
      }
    }
    if (strcmp(command, "STOP") == 0) {
      pthread_mutex_lock(&file_mutex); // Lock the mutex
      server_stopped = 1;
      pthread_mutex_unlock(&file_mutex);
      snprintf(server_message, sizeof(server_message), "Server Stopped");
      send(client_sock, server_message, strlen(server_message), 0);
      close(client_sock);
      exit(0);
    }
  }

  else {
    snprintf(server_message, sizeof(server_message),
             "Invalid command or parsing error.");
  }

  // printf("Msg from client: %s\n", client_message);

  // Respond to client:
  // strcpy(server_message, "File is written successfully in the server.");

  if (send(client_sock, server_message, strlen(server_message), 0) < 0) {
    printf("Can't send\n");
    return -1;
  }
  // Close the socket and free the memory for the handler args
  close(client_sock);
  free(arguments);

  // pthread_exit(NULL

  return NULL;
}

/* This method is used to read the ip adress from a config file*/
char *readIP(const char *config) {
  FILE *file = fopen(config, "r");
  if (file == NULL) {
    perror("Error while opening config");
    return NULL;
  }

  static char ip[16];
  if (fscanf(file, "%15s", ip) != 1) {
    perror("Error reading IP from config file");
    fclose(file);
    return NULL;
  }

  fclose(file);
  return ip;
}

int main(void) {
  int socket_desc, client_sock;
  socklen_t client_size;
  struct sockaddr_in server_addr, client_addr;

  // Create socket:
  socket_desc = socket(AF_INET, SOCK_STREAM, 0);

  if (socket_desc < 0) {
    printf("Error while creating socket\n");
    return -1;
  }
  printf("Socket created successfully\n");

  // Set port and IP
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(2000);
  char *config_ip = readIP(CONFIG_FILE);
  if (config_ip == NULL) {
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  } else {
    server_addr.sin_addr.s_addr = inet_addr(config_ip);
  }

  // Bind to the set port and IP:
  if (bind(socket_desc, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {
    printf("Couldn't bind to the port\n");
    return -1;
  }
  printf("Done with binding\n");

  // Listen for clients:
  if (listen(socket_desc, 1) < 0) {
    printf("Error while listening\n");
    return -1;
  }
  if (pthread_mutex_init(&file_mutex, NULL) != 0) {
    printf("failed mutex initialisation\n");
    return 1;
  }
  pthread_mutex_init(&file_write_mutex, NULL);
  while (!server_stopped) {
    printf("\nListening for incoming connections.....\n");

    // Accept an incoming connection:

    client_size = sizeof(client_addr);
    client_sock =
        accept(socket_desc, (struct sockaddr *)&client_addr, &client_size);

    if (client_sock < 0) {
      printf("Can't accept\n");
      return -1;
    }
    printf("Client connected at IP: %s and port: %i\n",
           inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    // Allocate memory for thread arguments
    pthread_t client_thread;
    struct handler_args *args =
        (struct handler_args *)malloc(sizeof(struct handler_args));
    args->client_sock = client_sock;

    // Create a new thread for each client connection
    if (pthread_create(&client_thread, NULL, client_handler, (void *)args) !=
        0) {
      printf("Error creating client thread\n");
      free(args); // Free allocated memory in case of thread creation failure
      close(client_sock); // Close the client socket
      continue;
    }

    // Detach the thread so it doesn't need to be joined later
    pthread_detach(client_thread);
  }

  close(socket_desc);
  pthread_mutex_destroy(&file_mutex);
  pthread_mutex_destroy(&file_write_mutex);
  return 0;
}
