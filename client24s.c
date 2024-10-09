#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>      // For errno
#include <sys/stat.h>   // For mkdir
#include <libgen.h>     // For basename and dirname

#define PORT 12329
#define BUFSIZE 1024

void handle_client_input(int sock);

int main() {
    int sock;
    struct sockaddr_in server_addr;

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set up server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Connect to server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("Connected to server...\n");

    // Handle input from user
    handle_client_input(sock);

    close(sock);
    return 0;
}

void handle_client_input(int sock) {
    char buffer[BUFSIZE];
    char response[BUFSIZE];

    while (1) {
        printf("Enter command: ");
        fgets(buffer, BUFSIZE, stdin);

        // Remove trailing newline character if present
        buffer[strcspn(buffer, "\n")] = '\0';

        char command[BUFSIZE];
        char arg1[BUFSIZE], arg2[BUFSIZE];
        int result = sscanf(buffer, "%s %s %s", command, arg1, arg2);

        if (strcmp(command, "ufile") == 0) {
            if (result != 3) {
                printf("ERROR: Invalid ufile syntax. Expected: ufile <filename> <destination>\n");
                continue;
            }
        } else if (strcmp(command, "rmfile") == 0) {
            if (result != 2) {
                printf("ERROR: Invalid rmfile syntax. Expected: rmfile <filename>\n");
                continue;
            }
        } else if (strcmp(command, "dtar") == 0) {
            if (result != 2) {
                printf("ERROR: Invalid dtar syntax. Expected: dtar <filetype>\n");
                continue;
            }
        } else if (strcmp(command, "display") == 0) {
            if (result != 2) {
                printf("ERROR: Invalid display syntax. Expected: display <pathname>\n");
                continue;
            }
        }else if (strcmp(command, "dfile") == 0) {
            if (result != 2) {
                printf("ERROR: Invalid display syntax. Expected: display <pathname>\n");
                continue;
            }
        }else {
            printf("ERROR: Unknown command.\n");
            continue;
        }

        if (send(sock, buffer, strlen(buffer), 0) < 0) {
            perror("Send failed");
            break;
        }

        // Wait for response
        ssize_t bytes_received = recv(sock, response, sizeof(response) - 1, 0);
        if (bytes_received < 0) {
            perror("Receive failed");
            break;
        }

        response[bytes_received] = '\0';
        printf("Server response: %s\n", response);

        // Handle file saving if a tar file is being created
        if (strstr(response, "Tar file created at:") != NULL) {
            // Extract the tar file path from the response
            char tarfile_path[BUFSIZE];
            sscanf(response, "Tar file created at: %s", tarfile_path);

            // Adjust path to client's directory
            snprintf(tarfile_path, sizeof(tarfile_path), "/home/mehta5f/client/tar/%s", basename(tarfile_path));

            // Create directory if not exists
            char dir[BUFSIZE];
            snprintf(dir, sizeof(dir), "%s", dirname(tarfile_path));
            if (mkdir(dir, 0755) != 0 && errno != EEXIST) {
                perror("mkdir failed");
                continue;
            }

            FILE *file = fopen(tarfile_path, "wb");
            if (file) {
                while ((bytes_received = recv(sock, response, sizeof(response), 0)) > 0) {
                    fwrite(response, 1, bytes_received, file);
                }
                fclose(file);
                printf("Tar file saved at: %s\n", tarfile_path);
            } else {
                perror("Failed to create file");
            }
        }
    }
}














// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <arpa/inet.h>
// #include <errno.h>      // For errno
// #include <sys/stat.h>   // For mkdir
// #include <libgen.h>     // For basename and dirname

// #define PORT 12329
// #define BUFSIZE 1024

// void handle_client_input(int sock);

// int main() {
//     int sock;
//     struct sockaddr_in server_addr;

//     // Create socket
//     sock = socket(AF_INET, SOCK_STREAM, 0);
//     if (sock < 0) {
//         perror("Socket creation failed");
//         exit(EXIT_FAILURE);
//     }

//     // Set up server address
//     server_addr.sin_family = AF_INET;
//     server_addr.sin_port = htons(PORT);
//     server_addr.sin_addr.s_addr = INADDR_ANY;

//     // Connect to server
//     if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
//         perror("Connection failed");
//         close(sock);
//         exit(EXIT_FAILURE);
//     }

//     printf("Connected to server...\n");

//     // Handle input from user
//     handle_client_input(sock);

//     close(sock);
//     return 0;
// }


// void handle_client_input(int sock) {
//     char buffer[BUFSIZE];
//     char response[BUFSIZE];

//     while (1) {
//         printf("Enter command: ");
//         fgets(buffer, BUFSIZE, stdin);

//         // Remove trailing newline character if present
//         buffer[strcspn(buffer, "\n")] = '\0';

//         char command[BUFSIZE];
//         char arg1[BUFSIZE], arg2[BUFSIZE];
//         int result = sscanf(buffer, "%s %s %s", command, arg1, arg2);

//         if (strcmp(command, "ufile") == 0) {
//             if (result != 3) {
//                 printf("ERROR: Invalid ufile syntax. Expected: ufile <filename> <destination>\n");
//                 continue;
//             }
//         } else if (strcmp(command, "rmfile") == 0) {
//             if (result != 2) {
//                 printf("ERROR: Invalid rmfile syntax. Expected: rmfile <filename>\n");
//                 continue;
//             }
//         } else if (strcmp(command, "dtar") == 0) {
//             if (result != 2) {
//                 printf("ERROR: Invalid dtar syntax. Expected: dtar <filetype>\n");
//                 continue;
//             }
//         } else {
//             printf("ERROR: Unknown command.\n");
//             continue;
//         }

//         if (send(sock, buffer, strlen(buffer), 0) < 0) {
//             perror("Send failed");
//             break;
//         }

//         // Wait for response
//         ssize_t bytes_received = recv(sock, response, sizeof(response) - 1, 0);
//         if (bytes_received < 0) {
//             perror("Receive failed");
//             break;
//         }

//         response[bytes_received] = '\0';
//         if (strcmp(response, "END_OF_FILE") == 0) {
//             // Handle end of file case
//             printf("File transfer complete.\n");
//         } else {
//             printf("Server response: %s\n", response);
//         }
//     }
// }



// // void handle_client_input(int sock) {
// //     char buffer[BUFSIZE];
// //     char response[BUFSIZE];

// //     while (1) {
// //         printf("Enter command: ");
// //         fgets(buffer, BUFSIZE, stdin);

// //         // Remove trailing newline character if present
// //         buffer[strcspn(buffer, "\n")] = '\0';

// //         char command[BUFSIZE];
// //         char arg1[BUFSIZE], arg2[BUFSIZE];
// //         int result = sscanf(buffer, "%s %s %s", command, arg1, arg2);

// //         // Validate the command
// //         if (result < 2) {
// //             printf("ERROR: Invalid command syntax.\n");
// //             continue;
// //         }

// //         if (strcmp(command, "ufile") == 0) {
// //             if (result != 3) {
// //                 printf("ERROR: Invalid ufile syntax. Expected: ufile <filename> <destination>\n");
// //                 continue;
// //             }
// //             // Proceed to send 'ufile' command
// //         } else if (strcmp(command, "rmfile") == 0) {
// //             if (result != 2) {
// //                 printf("ERROR: Invalid rmfile syntax. Expected: rmfile <filename>\n");
// //                 continue;
// //             }
// //             // Proceed to send 'rmfile' command
// //         } else if (strcmp(command, "dtar") == 0) {
// //             if (result != 2) {
// //                 printf("ERROR: Invalid dtar syntax. Expected: dtar <filetype>\n");
// //                 continue;
// //             }
// //         } else {
// //             printf("ERROR: Unknown command.\n");
// //             continue;
// //         }

// //         // Send command to server
// //         if (send(sock, buffer, strlen(buffer), 0) < 0) {
// //             perror("Send failed");
// //             break;
// //         }

// //         // Wait for response
// //         ssize_t bytes_received = recv(sock, response, sizeof(response) - 1, 0);
// //         if (bytes_received > 0) {
// //             response[bytes_received] = '\0';
// //             printf("Response from server: %s\n", response);

// //             // Save the tar file if path is provided
// //             if (strstr(response, "Tar file created at:") != NULL) {
// //                 // Extract the tar file path from the response
// //                 char tarfile_path[BUFSIZE];
// //                 sscanf(response, "Tar file created at: %s", tarfile_path);

// //                 // Adjust path to client's directory
// //                 snprintf(tarfile_path, sizeof(tarfile_path), "/home/mehta5f/client/tar/%s", basename(tarfile_path));

// //                 // Create directory if not exists
// //                 char dir[BUFSIZE];
// //                 snprintf(dir, sizeof(dir), "%s", dirname(tarfile_path));
// //                 if (mkdir(dir, 0755) != 0 && errno != EEXIST) {
// //                     perror("mkdir failed");
// //                     continue;
// //                 }

// //                 FILE *file = fopen(tarfile_path, "wb");
// //                 if (file) {
// //                     while ((bytes_received = recv(sock, response, sizeof(response), 0)) > 0) {
// //                         fwrite(response, 1, bytes_received, file);
// //                     }
// //                     fclose(file);
// //                     printf("Tar file saved at: %s\n", tarfile_path);
// //                 } else {
// //                     perror("Failed to create file");
// //                 }
// //             }
// //         } else {
// //             perror("Receive failed");
// //             break;
// //         }
// //     }
// // }