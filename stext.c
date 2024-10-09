#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define PORT 88887
#define BUFSIZE 1024
#define BASE_DIR "/home/mehta5f/stext"
int server_sock, client_sock;

void handle_client(int client_sock);
void handle_ufile(const char *filename, const char *dest_path);
int create_directory(const char *path);
void handle_rmfile(const char *filepath);

void send_response(int sock, const char *response) {
    send(sock, response, strlen(response), 0);
}

int main() {
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Create socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set up server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind socket
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_sock, 5) < 0) {
        perror("Listen failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("Stext is listening on port %d...\n", PORT);

    while (1) {
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock < 0) {
            perror("Accept failed");
            continue;
        }

        // Fork to handle the client
        pid_t pid = fork();
        if (pid == 0) { // Child process
            close(server_sock);
            handle_client(client_sock);
            close(client_sock);
            exit(0);
        } else if (pid > 0) { // Parent process
            close(client_sock);
        } else {
            perror("Fork failed");
        }
    }

    close(server_sock);
    return 0;
}

void handle_client(int client_sock) {
    char buffer[BUFSIZE];
    ssize_t bytes_received;

    while ((bytes_received = recv(client_sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_received] = '\0'; // Null-terminate string
        printf("Received command: %s\n", buffer);

        char command[BUFSIZE];
        char arg1[BUFSIZE], arg2[BUFSIZE];
        int result = sscanf(buffer, "%s %s %s", command, arg1, arg2);

        if (strcmp(command, "ufile") == 0) {
            if (result == 3) {
                handle_ufile(arg1, arg2);
            } else {
                printf("ERROR: Invalid ufile command syntax.\n");
                send_response(client_sock, "ERROR: Invalid ufile command syntax.");
            }
        } else if (strcmp(command, "rmfile") == 0) {
            if (result == 2) {
                handle_rmfile(arg1);
            } else {
                printf("ERROR: Invalid rmfile command syntax.\n");
                send_response(client_sock, "ERROR: Invalid rmfile command syntax.");
            }
        } else {
            printf("ERROR: Unknown command.\n");
            send_response(client_sock, "ERROR: Unknown command.");
        }
    }

    if (bytes_received < 0) {
        perror("Receive failed");
    }
}

void handle_ufile(const char *filename, const char *dest_path) {
    // Adjust destination path for Spdf
    char adjusted_path[BUFSIZE];
    snprintf(adjusted_path, sizeof(adjusted_path), "%s%s", BASE_DIR, dest_path + strlen("~smain"));

    // Ensure destination path exists or create it
    if (create_directory(adjusted_path) != 0) {
        printf("ERROR: Could not create destination directory.\n");
        send_response(client_sock, "ERROR: Could not create destination directory.");
        return;
    }

    char full_path[BUFSIZE];
    snprintf(full_path, sizeof(full_path), "%s/%s", adjusted_path, filename);

    // Attempt to open the destination file for writing
    FILE *dest_file = fopen(full_path, "wb");
    if (dest_file == NULL) {
        if (errno == EEXIST) {
            printf("ERROR: File already exists: %s\n", full_path);
            send_response(client_sock, "ERROR: File already exists.");
        } else {
            perror("Destination file creation failed");
            send_response(client_sock, "ERROR: Destination file creation failed.");
        }
        return;
    }

    // Now handle the actual file copy
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Source file open failed");
        fclose(dest_file); // Cleanup
        send_response(client_sock, "ERROR: Source file open failed.");
        return;
    }

    char buffer[BUFSIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, BUFSIZE, file)) > 0) {
        if (fwrite(buffer, 1, bytes_read, dest_file) != bytes_read) {
            perror("File write failed");
            fclose(file);
            fclose(dest_file);
            send_response(client_sock, "ERROR: File write failed.");
            return;
        }
    }

    fclose(file);
    fclose(dest_file);

    /*if (feof(file)) {*/
        printf("File transferred successfully to %s\n", full_path);
        send_response(client_sock, "File transfer successful.");
   /* } else {
        perror("File transfer failed");
        send_response(client_sock, "ERROR: File transfer failed.");
    }*/
}


void handle_rmfile(const char *filepath) {
    // Debug: Print the received filepath
    printf("Debug: Received filepath: %s\n", filepath);

    char full_path[BUFSIZE];

    // Check if the path starts with "~smain"
    if (strncmp(filepath, "~smain", 6) == 0) {
        // Adjust file path for Spdf
     //   printf("\nThis is for check1\n");
        snprintf(full_path, sizeof(full_path), "%s%s", BASE_DIR, filepath + 6);
    } 
    // Check if the path starts with the full Smain directory
    else if (strncmp(filepath, "/home/mehta5f/smain", 19) == 0) {
        // Replace the "/home/mehta5f/smain" prefix with "/home/mehta5f/spdf"
        snprintf(full_path, sizeof(full_path), "%s%s", BASE_DIR, filepath + 19);
    } else {
        printf("Debug: ERROR: Invalid file path. It should start with '~smain' or be within '/home/mehta5f/smain'.\n");
        send_response(client_sock, "ERROR: Invalid file path.");
        return;
    }

    // Debug: Print the adjusted path
    printf("Debug: Adjusted full path: %s\n", full_path);

    // Delete the file
    if (remove(full_path) == 0) {
        printf("File %s deleted successfully.\n", full_path);
        send_response(client_sock, "File deletion successful.");
    } else {
        perror("File deletion failed");
        send_response(client_sock, "ERROR: File deletion failed.");
    }
}




int create_directory(const char *path) {
    char temp[BUFSIZE];
    snprintf(temp, sizeof(temp), "%s", path);
    for (char *p = temp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(temp, S_IRWXU) != 0 && errno != EEXIST) {
                perror("mkdir failed");
                return -1;
            }
            *p = '/';
        }
    }
    if (mkdir(temp, S_IRWXU) != 0 && errno != EEXIST) {
        perror("mkdir failed");
        return -1;
    }
    return 0;
}
