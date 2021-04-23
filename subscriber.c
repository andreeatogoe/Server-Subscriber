#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include "helpers.h"

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_address server_port\n", file);
	exit(0);
}

int main(int argc, char *argv[])
{
	int sockfd, n, ret;
	struct sockaddr_in serv_addr;
	char buffer[BUFLEN];

	fd_set read_fds;
	fd_set aux_fds;
	int fmax;

	if (argc < 3) {
		usage(argv[0]);
	}

	FD_ZERO(&read_fds);
	FD_ZERO(&aux_fds);
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");

	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect");

	FD_SET(sockfd, &read_fds);
	FD_SET(0, &read_fds);
	fmax = sockfd;

	n = send(sockfd, argv[1], strlen(argv[1]), 0);
	DIE(n < 0, "send");
	int yes = 1;
	setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *) &yes, sizeof(int));
	while (1) {

		aux_fds = read_fds;
  		// se citeste de la tastatura
  		ret  = select(fmax + 1, &aux_fds, NULL, NULL, NULL);
  		memset(buffer, 0, BUFLEN);

		if(FD_ISSET(0, &aux_fds)) {
			
			fgets(buffer, BUFLEN - 1, stdin);

			if (strncmp(buffer, "exit", 4) == 0) {
				break;
			}

			// se trimite mesaj la server
			n = send(sockfd, buffer, strlen(buffer), 0);
			DIE(n < 0, "send");

			char *topic = strtok(buffer, " ");
			if (strncmp(topic, "subscribe", 10) == 0) {
				topic = strtok(NULL, " ");
				printf("subscribed %s\n", topic);
			} else {
				if (strncmp(topic, "unsubscribe", 12) == 0) {
					topic = strtok(NULL, " ");
					printf("unsubscribed %s\n", topic);
				}
			}
		
		} else {
			
			int buf;
			ret = recv(sockfd, &buf, sizeof(int), 0);

			memset(buffer, 0, BUFLEN);
			ret = recv(sockfd, buffer, buf, 0);
			char *token = strtok(buffer, "-");
			printf("%s-", token);
			int poz;
			char * tip = malloc(sizeof(char) * 10);
			for (poz = 0;poz < 2;poz++) {
				token = strtok(NULL, "-");
				if(poz == 1) {
					strcpy(tip, token);
				}
				printf("%s-", token);
			}
			token = strtok(NULL, "-");
			if (strncmp(tip, " INT", 4) == 0) {
				//ntohl
				int *aux = malloc(sizeof(int) * 4);
				memcpy(aux, token + 2, 4);
				*aux = ntohl(*aux);
				int *sign = malloc(sizeof(int));
				memcpy(sign, token + 1, 1);
				if ((*sign) == 0)
					printf(" %d\n", *aux);
				else 
					printf(" %d\n", -*aux);
				free(aux);
				free(sign);

			} else {
				if (strncmp(tip, " FLOAT", 6) == 0) {
					unsigned int *val_int = malloc(sizeof(unsigned int));
					memcpy(val_int, token + 2, 4);
					int *putere = malloc(sizeof(int));
					memcpy(putere, token + 6, 1);
					*val_int = ntohl(*val_int);
					int j;
					long put = 1;
					for (j = 0;j < *putere;j++) {
						put = put * 10;
					}
					float rasp = (float)(*val_int / put);
					int *sgn = malloc(sizeof(int));
					memcpy(sgn, token + 1, 1);
					if(*sgn == 0)
						printf(" %f\n", rasp);
					else 
						printf(" %f\n", -rasp);
					free(val_int);
					free(putere);
					free(sgn);

				} else {
					if (strncmp(tip, " STRING", 7) == 0) {
						printf(" %s\n", token);
					} else {
						if (strncmp(tip, " SHORT_REAL", 11) == 0) {
							short *aux = malloc(sizeof(float));
							memcpy(aux, token + 1, 2);
							*aux = ntohs(*aux);
							float cv = (float)(*aux / 100);
							printf(" %.2f\n", cv);
							free(aux);
						}
					}
				}
			}
			free(tip);

		}

	}

	close(sockfd);

	return 0;
}
