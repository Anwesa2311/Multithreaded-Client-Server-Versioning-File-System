#ifndef SERVER_H
#define SERVER_H

#include <sys/socket.h>

// Constants
#define DEFAULT_IP "127.0.0.1"
#define CONFIG_FILE "Config.txt"

// Structure declarations
struct handler_args {
  int client_sock;
};

// Function declarations
char *extract_timestamp_info(char *filename);
char **list_file_versions(const char *full_path, int *count);
void create_versioned_backup(const char *original_path,
                             const char *versioned_path);
int isDirectory(const char *path);
int remove_dir(const char *path);
void *client_handler(void *arguments);
char *readIP(const char *config);
void append_latest_file_info(const char *filepath, char *server_message);

#endif // SERVER_H
