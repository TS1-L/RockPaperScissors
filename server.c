#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include "game.h"

void *handle_game(void *arg);
int determine_winner(char choice1, char choice2);
void get_player_names(int client1, int client2, char *name1, char *name2);
int ask_play_again(int client);

int server_fd;

void cleanup() {
    close(server_fd);
    printf("Server shut down.\n");
    exit(0);
}

int main() {
    int client_fd1, client_fd2;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // this handle SIGINT to clean up
    signal(SIGINT, cleanup);

    // for creating sockets
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    //  to bind socket
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // listening for connections
    if (listen(server_fd, 10) == -1) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server is running on port %d\n", PORT);

    while (1) {
        printf("Waiting for players to connect...\n");

        // the clients acception
        if ((client_fd1 = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len)) == -1) {
            perror("Accept client 1 failed");
            continue;
        }
        printf("Client 1 connected.\n");

        if ((client_fd2 = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len)) == -1) {
            perror("Accept client 2 failed");
            close(client_fd1);
            continue;
        }
        printf("Client 2 connected.\n");

        // for the multi thread for each game
        pthread_t thread_id;
        int *client_fds = malloc(2 * sizeof(int));
        client_fds[0] = client_fd1;
        client_fds[1] = client_fd2;

        if (pthread_create(&thread_id, NULL, handle_game, (void *)client_fds) != 0) {
            perror("Thread creation failed");
            close(client_fd1);
            close(client_fd2);
            free(client_fds);
            continue;
        }
        
	// detaching the thread to auto clean up
        pthread_detach(thread_id);
    }

    close(server_fd);
    return 0;
}

void *handle_game(void *arg) {
    int *client_fds = (int *)arg;
    int client1 = client_fds[0];
    int client2 = client_fds[1];
    free(client_fds);

    char name1[35], name2[35];
    char choice1, choice2;
    FILE *log_file = fopen("logs.txt", "a");
    if (!log_file) {
        perror("Failed to open log file");
        close(client1);
        close(client2);
        return NULL;
    }

    get_player_names(client1, client2, name1, name2);

    fprintf(log_file, "Game started: %s vs %s\n", name1, name2);
    fflush(log_file);

    do {
        // the choices of every client
        if (recv(client1, &choice1, sizeof(choice1), 0) <= 0) {
            perror("Failed to receive choice from client 1");
            break;
        }
        if (recv(client2, &choice2, sizeof(choice2), 0) <= 0) {
            perror("Failed to receive choice from client 2");
            break;
        }

        int result1, result2;

        // determine results for both players
        if (choice1 == choice2) {
            result1 = result2 = DRAW;
        } else if ((choice1 == 'R' && choice2 == 'S') || (choice1 == 'S' && choice2 == 'P') || (choice1 == 'P' && choice2 == 'R')) {
            result1 = WIN;
            result2 = LOSE;
        } else {
            result1 = LOSE;
            result2 = WIN;
        }

        // save the results into the logs.txt
        fprintf(log_file, "%s chose %c, %s chose %c: %s\n", name1, choice1, name2, choice2,
                result1 == WIN ? name1 : result1 == LOSE ? name2 : "Draw");
        fflush(log_file);


        send(client1, &result1, sizeof(result1), 0);
        send(client2, &result2, sizeof(result2), 0);
    } while (ask_play_again(client1) && ask_play_again(client2));

    fprintf(log_file, "Game ended: %s vs %s\n", name1, name2);
    fflush(log_file);
    fclose(log_file);

    close(client1);
    close(client2);

    return NULL;
}

void get_player_names(int client1, int client2, char *name1, char *name2) {
    // Ask for player names
    send(client1, "Enter your name: ", 17, 0);
    recv(client1, name1, 35, 0);

    send(client2, "Enter your name: ", 17, 0);
    recv(client2, name2, 35, 0);
}

int ask_play_again(int client) {
    char response;
    send(client, "Do you want to play again? (Y/N): ", 32, 0);
    recv(client, &response, sizeof(response), 0);
    return (response == 'Y' || response == 'y');
}
