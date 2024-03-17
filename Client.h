#ifndef CLIENT_H
#define CLIENT_H

#include <netinet/in.h>
#include <sys/socket.h>

// Function declarations
void write_command(int socket_desc, char *local_path, char *remote_path);
void get_command(int socket_desc, char *client_message, char *server_message,
                 char *local_path);
char *extract_file_name(const char *file_path);

#endif // CLIENT_H
