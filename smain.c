#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <dirent.h>


#define PORT 12329
#define BUFSIZE 1024
#define MAX_BUFFER 1024
#define BASE_DIR "/home/mehta5f/smain"
#define PDF_PORT 88888
#define TXT_PORT 88887

int server_sock, client_sock;
struct sockaddr_in server_addr, client_addr;
socklen_t client_len = sizeof(client_addr);

void handle_client(int client_sock);
void handle_ufile(const char *filename, const char *dest_path);
void send_response(int sock, const char *response);
int create_directory(const char *path);
void forward_to_server(const char *filename, const char *dest_path, const char *server_ip, int port);
void handle_rmfile(const char *filepath);
void send_delete_request_to_server(const char *server_ip, int server_port, const char *filepath);
void save_c_file(const char *filename, const char *dest_path);
void list_files_recursive(const char *dir_path, char *response);
void handle_display(const char *pathname, int client_sock);
void handle_dfile(const char *client_path, int client_sock);

//new
void handle_dtar(int client_socket, const char *filetype);


int create_tar_directory(const char *dir_path);


void create_tar_for_files(const char *filetype, const char *base_dir);




int main() {
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

    printf("Smain is listening on port %d...\n", PORT);

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
        char filename[BUFSIZE], dest_path[BUFSIZE];
        int result = sscanf(buffer, "%s %s %s", command, filename, dest_path);

        // Handle 'ufile' command
        if (result == 3 && strcmp(command, "ufile") == 0) {
            handle_ufile(filename, dest_path);
        }
        // Handle 'rmfile' command
        else if (result == 2 && strcmp(command, "rmfile") == 0) {
            handle_rmfile(filename);
        }
        // Handle 'dtar' command
        else if (strncmp(buffer, "dtar", 4) == 0) {
            char filetype[10];
            sscanf(buffer + 5, "%s", filetype);

            handle_dtar(client_sock, filetype);
        }else if (strcmp(command, "display") == 0) {
            if (result == 2) {
                handle_display(filename, client_sock);
            } else {
                printf("ERROR: Invalid display command syntax.\n");
                send_response(client_sock, "ERROR: Invalid display command syntax.");
            }
        }else if (strcmp(command, "dfile") == 0) {
            if (result == 2) {
                handle_dfile(filename, client_sock);
            } else {
                printf("ERROR: Invalid display command syntax.\n");
                send_response(client_sock, "ERROR: Invalid display command syntax.");
            }
        }else {
            // Invalid command syntax
            printf("ERROR: Invalid command syntax.\n");
            send_response(client_sock, "ERROR: Invalid command syntax.");
        }
    }

    if (bytes_received < 0) {
        perror("Receive failed");
    }
}

// void handle_client(int client_sock) {
//     char buffer[BUFSIZE];
//     ssize_t bytes_received;

//     while ((bytes_received = recv(client_sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
//         buffer[bytes_received] = '\0'; // Null-terminate string
//         printf("Received command: %s\n", buffer);

//         char command[BUFSIZE];
//         char filename[BUFSIZE], dest_path[BUFSIZE];
//         int result = sscanf(buffer, "%s %s %s", command, filename, dest_path);

//         // Handle 'ufile' command
//         if (result == 3 && strcmp(command, "ufile") == 0) {
//             handle_ufile(filename, dest_path);
//         }
//         // Handle 'rmfile' command
//         else if (result == 2 && strcmp(command, "rmfile") == 0) {
//             handle_rmfile(filename);
//         }
//         // Handle 'dtar' command
//         else if (strncmp(buffer, "dtar", 4) == 0) {
//             char filetype[10];
//             sscanf(buffer + 5, "%s", filetype);

//             if (create_tar_directory("/home/mehta5f") == 0) { // Create tar directories
//                 printf("Tar directories created successfully.\n");
//                 create_tar_for_files(filetype, "/home/mehta5f"); // Create tar file for the specified file type

//                 // Send confirmation to client
//                 char response[BUFSIZE];
//                 snprintf(response, sizeof(response), "Tar file created at: /home/mehta5f/tar/%sfiles.tar", filetype);
//                 send_response(client_sock, response);

//                 // Prepare to send the tar file
//                 char tarfile_path[BUFSIZE];
//                 snprintf(tarfile_path, sizeof(tarfile_path), "/home/mehta5f/tar/%sfiles.tar", filetype);

//                 FILE *tar_file = fopen(tarfile_path, "rb");
//                 if (tar_file) {
//                     // Send file content
//                     while ((bytes_received = fread(buffer, 1, sizeof(buffer), tar_file)) > 0) {
//                         if (send(client_sock, buffer, bytes_received, 0) < 0) {
//                             perror("Send failed");
//                             fclose(tar_file);
//                             return;
//                         }
//                     }
//                     fclose(tar_file);

//                     // Notify end of file transfer
//                     snprintf(response, sizeof(response), "END_OF_FILE");
//                     send(client_sock, response, strlen(response), 0);
//                 } else {
//                     perror("Failed to open tar file");
//                     send_response(client_sock, "ERROR: Failed to open tar file.");
//                 }
//             } else {
//                 printf("Failed to create tar directories.\n");
//                 send_response(client_sock, "ERROR: Failed to create tar directories.");
//             }
//         } else {
//             // Invalid command syntax
//             printf("ERROR: Invalid command syntax.\n");
//             send_response(client_sock, "ERROR: Invalid command syntax.");
//         }
//     }

//     if (bytes_received < 0) {
//         perror("Receive failed");
//     }
// }


void handle_rmfile(const char *filepath) {
    // Determine file extension
    const char *ext = strrchr(filepath, '.');
    if (!ext) {
        send_response(client_sock, "ERROR: Invalid file extension.");
        return;
    }

    char adjusted_path[BUFSIZE];
    snprintf(adjusted_path, sizeof(adjusted_path), "%s%s", BASE_DIR, filepath + strlen("~smain"));

    // Handle .pdf files by forwarding the request to Spdf
    if (strcmp(ext, ".pdf") == 0) {
        send_delete_request_to_server("127.0.0.1", PDF_PORT, adjusted_path);
    }
    // Handle .txt files by forwarding the request to Stext
    else if (strcmp(ext, ".txt") == 0) {
        send_delete_request_to_server("127.0.0.1", TXT_PORT, adjusted_path);
    }
     // Handle .c files stored directly in Smain
    else if (strcmp(ext, ".c") == 0) {
        if (remove(adjusted_path) == 0) {
            printf("File %s deleted successfully.\n", adjusted_path);
            send_response(client_sock, "File deleted successfully.");
        } else {
            perror("File deletion failed");
            send_response(client_sock, "ERROR: File deletion failed.");
        }
    }
    // Handle files stored directly in Smain
    else {
        if (remove(adjusted_path) == 0) {
            printf("File %s deleted successfully.\n", adjusted_path);
            send_response(client_sock, "File deleted successfully.");
        } else {
            perror("File deletion failed");
            send_response(client_sock, "ERROR: File deletion failed.");
        }
    }
}

void send_delete_request_to_server(const char *server_ip, int server_port, const char *filepath) {
    int sock;
    struct sockaddr_in server_addr;

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        send_response(client_sock, "ERROR: Unable to connect to server.");
        return;
    }

    // Set up server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        send_response(client_sock, "ERROR: Unable to connect to server.");
        close(sock);
        return;
    }

    // Send delete command to the server
    char command[BUFSIZE];
    snprintf(command, sizeof(command), "rmfile %s", filepath);
    send(sock, command, strlen(command), 0);

    // Receive response from the server
    char response[BUFSIZE];
    ssize_t bytes_received = recv(sock, response, sizeof(response) - 1, 0);
    if (bytes_received > 0) {
        response[bytes_received] = '\0';
        send_response(client_sock, response);
    }

    close(sock);
}

void save_c_file(const char *filename, const char *dest_path) {
    char adjusted_path[BUFSIZE];
    snprintf(adjusted_path, sizeof(adjusted_path), "%s%s", BASE_DIR, dest_path + strlen("~smain"));

    // Create directory if not exists
    if (create_directory(adjusted_path) != 0) {
        send_response(client_sock, "ERROR: Directory creation failed.");
        return;
    }

    // Save the .c file locally
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("File open failed");
        send_response(client_sock, "ERROR: File open failed.");
        return;
    }

    char dest_file[BUFSIZE];
    snprintf(dest_file, sizeof(dest_file), "%s/%s", adjusted_path, filename);
    FILE *dest = fopen(dest_file, "wb");
    if (!dest) {
        perror("Destination file open failed");
        fclose(file);
        send_response(client_sock, "ERROR: File creation failed.");
        return;
    }

    char buffer[BUFSIZE];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        fwrite(buffer, 1, bytes, dest);
    }

    fclose(file);
    fclose(dest);

    printf("File %s saved successfully.\n", dest_file);
    send_response(client_sock, "File saved successfully.");
}

void handle_ufile(const char *filename, const char *dest_path) {
    char extension[BUFSIZE];
    const char *file_ext = strrchr(filename, '.');
    if (file_ext == NULL) {
        printf("ERROR: File extension missing.\n");
        return;
    }

    // Determine which server to forward the command to
    int port;
    char server_ip[BUFSIZE];
    if (strcmp(file_ext, ".pdf") == 0) {
        port = PDF_PORT;
        snprintf(server_ip, sizeof(server_ip), "127.0.0.1"); // Replace with actual IP if needed
    } else if (strcmp(file_ext, ".txt") == 0) {
        port = TXT_PORT;
        snprintf(server_ip, sizeof(server_ip), "127.0.0.1"); // Replace with actual IP if needed
    } else if (strcmp(file_ext, ".c") == 0) {
        save_c_file(filename, dest_path);
        return;
    }
    else {
        printf("ERROR: Unsupported file type.\n");
        return;
    }

    forward_to_server(filename, dest_path, server_ip, port);
}

void forward_to_server(const char *filename, const char *dest_path, const char *server_ip, int port) {
    int sock;
    struct sockaddr_in server_addr;
    char command[BUFSIZE];
    char response[BUFSIZE];
    ssize_t bytes_received;

    // Create socket for communication with other servers
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        return;
    }

    // Set up server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    server_addr.sin_port = htons(port);

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connect failed");
        close(sock);
        return;
    }

    // Send the command
    snprintf(command, sizeof(command), "ufile %s %s", filename, dest_path);
    if (send(sock, command, strlen(command), 0) < 0) {
        perror("Send failed");
        close(sock);
        return;
    }

    // Receive the response
    bytes_received = recv(sock, response, sizeof(response) - 1, 0);
    if (bytes_received > 0) {
        response[bytes_received] = '\0'; // Null-terminate string
        printf("Response from server: %s\n", response);
        send_response(client_sock, response);
    } else if (bytes_received < 0) {
        perror("Receive failed");
    }

    close(sock);
}

void send_response(int sock, const char *response) {
    send(sock, response, strlen(response), 0);
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





int create_tar_directory(const char *base_dir) {
    char tar_dir[BUFSIZE];

    snprintf(tar_dir, sizeof(tar_dir), "%s/tar", base_dir);

    // Create the tar directory
    if (create_directory(tar_dir) != 0) {
        return -1; // Return -1 if directory creation failed
    }

    return 0; // Return 0 on success
}


void create_tar_for_files(const char *filetype, const char *base_dir) {
    char command[BUFSIZE];
    char tarfile[BUFSIZE];
    char *directory;

    // Determine directory and tar file path based on filetype
    if (strcmp(filetype, ".c") == 0) {
        directory = "/home/mehta5f/smain";  // Assume base_dir points to /home/mehta5f/smain
        snprintf(tarfile, sizeof(tarfile), "%s/tar/cfiles.tar", base_dir);
    } else if (strcmp(filetype, ".pdf") == 0) {
        directory = "/home/mehta5f/spdf";  // Assume base_dir points to /home/mehta5f/spdf
        snprintf(tarfile, sizeof(tarfile), "%s/tar/pdffiles.tar", base_dir);
    } else if (strcmp(filetype, ".txt") == 0) {
        directory = "/home/mehta5f/stext";  // Assume base_dir points to /home/mehta5f/stext
        snprintf(tarfile, sizeof(tarfile), "%s/tar/txtfiles.tar", base_dir);
    } else {
        printf("Invalid file type\n");
        return;
    }

    // Create tar file command
    snprintf(command, sizeof(command), "cd %s && find . -type f -name '*%s' | tar -cvf %s -T -", directory, filetype, tarfile);

    // Execute tar command
    int result = system(command);
    if (result != 0) {
        printf("Failed to create tar file\n");
    } else {
        printf("Tar file created at: %s\n", tarfile);
    }
}

void handle_dtar(int client_socket, const char *filetype) {
    if (create_tar_directory("/home/mehta5f") == 0) { // Create tar directories
        printf("Tar directories created successfully.\n");
        create_tar_for_files(filetype, "/home/mehta5f"); // Create tar file for the specified file type

        // Prepare to send the tar file
        char tarfile_path[BUFSIZE];
        snprintf(tarfile_path, sizeof(tarfile_path), "%sfiles.tar", filetype);

        // Send tar file to client
        send_tar_file(client_socket, tarfile_path);
    } else {
        printf("Failed to create tar directories.\n");
        send_response(client_socket, "ERROR: Failed to create tar directories.");
    }
}

void send_tar_file(int client_socket, const char *tar_filename) {
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "/home/mehta5f/tar/%s", tar_filename);
    
    // Send tar file path to client
    send(client_socket, file_path, strlen(file_path), 0);
   /* 
    // Open and send the tar file contents
    FILE *tar_file = fopen(file_path, "rb");
    if (tar_file != NULL) {
        char buffer[1024];
        size_t bytes_read;
        while ((bytes_read = fread(buffer, 1, sizeof(buffer), tar_file)) > 0) {
            send(client_socket, buffer, bytes_read, 0);
        }
        fclose(tar_file);
        // Send end-of-file message
        send(client_socket, "END_OF_FILE", strlen("END_OF_FILE"), 0);
    } else {
        perror("Error opening tar file");
    }
    */
}


void list_files_recursive(const char *dir_path, char *response) {
    DIR *dir = opendir(dir_path);
    if (dir == NULL) {
        perror("opendir failed");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {  // Regular file
            const char *ext = strrchr(entry->d_name, '.');
            if (ext && (strcmp(ext, ".c") == 0 || strcmp(ext, ".txt") == 0 || strcmp(ext, ".pdf") == 0)) {
                char file_path[BUFSIZE];
                snprintf(file_path, sizeof(file_path), "%s/%s", dir_path, entry->d_name);

                // Replace directory prefixes with ~smain
                char display_path[BUFSIZE];
                if (strncmp(file_path, "/home/mehta5f/spdf", 19) == 0) {
                    snprintf(display_path, sizeof(display_path), "~smain%s", file_path + 19);
                } else if (strncmp(file_path, "/home/mehta5f/stext", 20) == 0) {
                    snprintf(display_path, sizeof(display_path), "~smain%s", file_path + 20);
                } else if (strncmp(file_path, "/home/mehta5f/smain", 19) == 0) {
                    snprintf(display_path, sizeof(display_path), "~smain%s", file_path + 19);
                } else {
                    snprintf(display_path, sizeof(display_path), "%s", file_path);
                }

                // Append the file path to the response
                snprintf(response + strlen(response), BUFSIZE - strlen(response), "%s\n", display_path);
            }
        } else if (entry->d_type == DT_DIR) {  // Directory
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                char sub_dir[BUFSIZE];
                snprintf(sub_dir, sizeof(sub_dir), "%s/%s", dir_path, entry->d_name);
                list_files_recursive(sub_dir, response);
            }
        }
    }

    closedir(dir);
}




void handle_display(const char *pathname, int client_sock) {
    char smain_path[BUFSIZE];
    char spdf_path[BUFSIZE];
    char stext_path[BUFSIZE];
    char response[BUFSIZE];

    response[0] = '\0';  // Initialize the response buffer

    if (strcmp(pathname, "~smain") == 0) {
        // Check all three directories
        strcpy(smain_path, "/home/mehta5f/smain");
        strcpy(spdf_path, "/home/mehta5f/spdf");
        strcpy(stext_path, "/home/mehta5f/stext");

        list_files_recursive(smain_path, response);
        list_files_recursive(spdf_path, response);
        list_files_recursive(stext_path, response);
    } else {
        // Check specific subdirectory under each main directory
        snprintf(smain_path, sizeof(smain_path), "/home/mehta5f/smain%s", pathname + strlen("~smain"));
        snprintf(spdf_path, sizeof(spdf_path), "/home/mehta5f/spdf%s", pathname + strlen("~smain"));
        snprintf(stext_path, sizeof(stext_path), "/home/mehta5f/stext%s", pathname + strlen("~smain"));

        list_files_recursive(smain_path, response);
        list_files_recursive(spdf_path, response);
        list_files_recursive(stext_path, response);
    }

    if (strlen(response) == 0) {
        strcpy(response, "No files found.");
    }

    send_response(client_sock, response);
}



void handle_dfile(const char *client_path, int client_sock) {
    char smain_base[] = "/home/mehta5f/smain";
    char spdf_base[] = "/home/mehta5f/spdf";
    char stext_base[] = "/home/mehta5f/stext";
    char actual_path[BUFSIZE];
    char destination_path[BUFSIZE];
    const char *filename = strrchr(client_path, '/');
    filename = filename ? filename + 1 : client_path;

    // Determine the correct base path based on the file extension
    const char *ext = strrchr(filename, '.');
    if (ext == NULL) {
        perror("Invalid file extension");
        return;
    }

    if (strcmp(ext, ".c") == 0) {
        snprintf(actual_path, BUFSIZE, "%s/%s", smain_base, client_path + strlen("~smain/"));
    } else if (strcmp(ext, ".pdf") == 0) {
        snprintf(actual_path, BUFSIZE, "%s/%s", spdf_base, client_path + strlen("~smain/"));
    } else if (strcmp(ext, ".txt") == 0) {
        snprintf(actual_path, BUFSIZE, "%s/%s", stext_base, client_path + strlen("~smain/"));
    } else {
        perror("Unsupported file extension");
        return;
    }

    // Set the destination path to /home/mehta5f/filename
    snprintf(destination_path, BUFSIZE, "/home/mehta5f/downloads/%s", filename);

    // Perform the file copy operation
    char command[BUFSIZE * 2];
    snprintf(command, sizeof(command), "cp '%s' '%s'", actual_path, destination_path);

    if (system(command) == 0) {
        // Success: send a response to the client
        char response[BUFSIZE];
        snprintf(response, BUFSIZE, "File '%s' copied successfully to '%s'.\n", filename, destination_path);
        send_response(client_sock, response);
    } else {
        // Failure: send an error response to the client
        char response[BUFSIZE];
        snprintf(response, BUFSIZE, "Failed to copy file '%s'.\n", filename);
        send_response(client_sock, response);
    }
}













// void send_file(int client_socket, const char *file_path) {
//     FILE *file = fopen(file_path, "rb");
//     if (file == NULL) {
//         perror("fopen");
//         char error_msg[MAX_BUFFER];
//         snprintf(error_msg, sizeof(error_msg), "ERROR: Could not open file %s", file_path);
//         send(client_socket, error_msg, strlen(error_msg), 0);
//         return;
//     }

//     char buffer[MAX_BUFFER];
//     size_t bytes_read;

//     while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
//         if (send(client_socket, buffer, bytes_read, 0) == -1) {
//             perror("send");
//             fclose(file);
//             return;
//         }
//     }

//     fclose(file);
//     printf("File %s sent successfully.\n", file_path);
// }


// void handle_dtar(int client_socket, const char *filetype) {
//     char command[MAX_BUFFER];
//     char tarfile[MAX_BUFFER];
//     char *directory;
//     char tarfile_path[MAX_BUFFER];
//     struct stat st = {0};

//     // Determine directory and tar file path based on filetype
//     if (strcmp(filetype, ".c") == 0) {
//         directory = "/home/mehta5f/smain";
//         snprintf(tarfile_path, sizeof(tarfile_path), "/home/mehta5f/smain/tar/c_files.tar");
//     } else if (strcmp(filetype, ".pdf") == 0) {
//         directory = "/home/mehta5f/spdf";
//         snprintf(tarfile_path, sizeof(tarfile_path), "/home/mehta5f/spdf/tar/pdf_files.tar");
//     } else if (strcmp(filetype, ".txt") == 0) {
//         directory = "/home/mehta5f/stext";
//         snprintf(tarfile_path, sizeof(tarfile_path), "/home/mehta5f/stext/tar/txt_files.tar");
//     } else {
//         const char *error_msg = "Invalid file type";
//         send(client_socket, error_msg, strlen(error_msg), 0);
//         return;
//     }

    // // Create directory if it doesn't exist
    // if (stat(directory, &st) == -1) {
    //     if (mkdir(directory, 0700) != 0) {
    //         perror("mkdir failed");
    //         send(client_socket, "ERROR: Directory creation failed.", 32, 0);
    //         return;
    //     }
    // }

//     // Create tar file name
//     snprintf(tarfile, sizeof(tarfile), "%s", tarfile_path);

//     // Create tar command
//     snprintf(command, sizeof(command), "cd %s && find . -type f -name '*%s' | tar -cvf %s -T -", directory, filetype, tarfile);

//     // Execute tar command
//     int result = system(command);
//     if (result != 0) {
//         const char *error_msg = "Failed to create tar file";
//         send(client_socket, error_msg, strlen(error_msg), 0);
//         return;
//     }

//     // Send the path of the tar file to the client
//     char path_msg[MAX_BUFFER];
//     snprintf(path_msg, sizeof(path_msg), "Tar file created at: %s", tarfile);
//     send(client_socket, path_msg, strlen(path_msg), 0);

//     // Send tar file to client
//     send_file(client_socket, tarfile);

//     // Remove the tar file after sending
//     unlink(tarfile);
// }