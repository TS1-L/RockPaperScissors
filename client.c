#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "game.h"

int main() {
    int client_fd;
    struct sockaddr_in server_addr;
    char choice;
    int result;
    char name[50];

    // Create socket
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to the server
    if (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Connection to server failed");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    printf("Connected to the server.\n");

    // Enter player name
    char buffer[35];
    recv(client_fd, buffer, sizeof(buffer), 0);
    printf("%s", buffer);
    scanf(" %49s", name);
    send(client_fd, name, strlen(name) + 1, 0);

    do {
        // Get player choice
        printf("Enter your choice (R for Rock, P for Paper, S for Scissors): ");
        scanf(" %c", &choice);
        // Validate choice
        while (choice != 'R' && choice != 'P' && choice != 'S') {
            printf("Invalid choice! Please enter 'R' for Rock, 'P' for Paper, or 'S' for Scissors: ");
            scanf(" %c", &choice);
        }

        // Send choice to server
        if (send(client_fd, &choice, sizeof(choice), 0) == -1) {
            perror("Failed to send choice to server");
            close(client_fd);
            exit(EXIT_FAILURE);
        }

        // Receive result from server
        if (recv(client_fd, &result, sizeof(result), 0) <= 0) {
            perror("Failed to receive result from server");
            close(client_fd);
            exit(EXIT_FAILURE);
        }

        // Display result
        if (result == WIN) {
            printf("You win!\n");
        } else if (result == LOSE) {
            printf("You lose!\n");
        } else {
            printf("It's a draw!\n");
        }

        // Ask if the player wants to play again
        char play_again_prompt[50];
        recv(client_fd, play_again_prompt, sizeof(play_again_prompt), 0);
        printf("%s", play_again_prompt);
        char response;
        scanf(" %c", &response);
        send(client_fd, &response, sizeof(response), 0);

        if (response != 'Y' && response != 'y') {
            printf("Exiting the game. Goodbye!\n");
            break;
        }

    } while (1);

    close(client_fd);
    return 0;
}
