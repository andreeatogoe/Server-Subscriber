#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include "helpers.h"

struct UdpMsg{
	char topic[50];
	char tip;
	char msg[1500];
};

typedef struct msg{
	char* ip_sender;
	unsigned int port;
	struct UdpMsg info;
	struct msg *next;
}Msg, *MList, **MAList;

typedef struct topics{
	char name[51];
	int sf;
	struct topics *next;
}T, *TList, **TAList;

typedef struct subscriber {
	char id[10];
	int socket;
	TList topics;
	MList messages;
	struct subscriber *next;
}S, *SList, **SAList;

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}

void add(SList sub, SAList list) {
	SList aux = *list;
	sub -> next = aux;
	*list = sub;
}

char* search_sock(SList list, int sock){
	while (list != NULL) {
		if(list -> socket == sock)
			return list -> id;
		list = list -> next;
	}
	return NULL;
}

void add_topic(SAList list, char *topic, char *sf, char *id){
	TList new = (struct topics*)malloc(sizeof(struct topics));
	strcpy(new -> name, topic);
	if(strncmp(sf, "1", 1) == 0) {
		new -> sf = 1;
	} 
	if(strncmp(sf, "0", 1) == 0) {
		new -> sf = 0;
	}
	SList l = *list;
	while (l != NULL) {
		if(l -> id == id) {
			new -> next = l -> topics;
			l -> topics = new;
			break;
		}
		l = l -> next;
	}
}

int search_id(SList list, char *id){
	while (list != NULL) {
		if (strcmp(list -> id, id) == 0) {
			return 1;
		}
		list = list -> next;
	}
	return 0;
}

void remove_topic(TAList l, char *topic){
	TList list = *l;
	if(strcmp(list -> name, topic) == 0){
		*l = list -> next;
	} else {
		while(list -> next != NULL) {
			if(strcmp(list -> next -> name, topic) == 0) {
				list -> next = list -> next -> next;
			}
			list = list -> next;
		}
	}
}

void unsubscribe(SAList l, char *id, char *topic){
	SList list = *l;
	while (list != NULL) {
		if(strcmp(list -> id, id) == 0) {
			remove_topic(&list -> topics, topic);
			break;
		}
		list = list -> next;
	}
}

void log_out(SAList list, int socket){
	SList l = *list;
	while (l != NULL) {
		if(l -> socket == socket) {
			l -> socket = -socket;
			break;
		}
		l = l -> next;
	}
}


//trimit toate mesajele stocate, daca exista
void empty_messages(SAList list, char *id) {
	SList l = *list;
	char buffer[BUFLEN];
	while (l != NULL) {
		if (strcmp(l -> id, id) == 0) {
			while (l -> messages != NULL) {
				memset(buffer, 0, BUFLEN);
				char *aux;
				strcpy(buffer, l -> messages -> ip_sender);
				aux = buffer + strlen(l -> messages -> ip_sender);
				strcpy(aux, ":");
				aux++;
				char *port = malloc(10 * sizeof(char));
				sprintf(port, "%d", l -> messages -> port);
				strcpy(aux, port);
				aux += strlen(port);
				free(port);
				strcpy(aux, " - ");
				aux += 3;
				strcpy(aux, l -> messages -> info.topic);
				aux += strlen(l -> messages -> info.topic);
				strcpy(aux, " - ");
				aux += 3;
				if(l -> messages -> info.tip == 0) {
					strcpy(aux, "INT - ");
					aux += 6;
				} else {
					if(l -> messages -> info.tip == 1) {
						strcpy(aux, "SHORT_REAL - ");
						aux += 13;
					} else {
						if (l -> messages -> info.tip == 2) {
							strcpy(aux, "FLOAT - ");
							aux += 8;
						} else {
							strcpy(aux, "STRING - ");
							aux += 9;
						}
					}
				}
				strcpy(aux, l -> messages -> info.msg);
				int buf_size = strlen(buffer);
				send(l -> socket, &buf_size, sizeof(int), 0);
				send(l -> socket, buffer, strlen(buffer), 0);
				l -> messages = l -> messages -> next;
			}
			l -> messages = NULL;
			break;
		}
		l = l -> next;
	}
}

void set_sock(SAList list, char *id, int new_socket) {
	SList l = *list;
	while (l != NULL) {
		if (strcmp(l -> id, id) == 0) {
			l -> socket = new_socket;
			break;
		}
		l = l -> next;
	}
}

int has_topic(TList topics, char *topic) {
	while (topics != NULL) {
		if (strcmp(topics -> name, topic) == 0) {
			if (topics -> sf == 1) {
				return 2;
			} else {
				return 1;
			}
		}
		topics = topics -> next;
	}
	return 0;
}

void save_msg(SAList list, char *id, struct UdpMsg msg, char *ip, int port){
	SList l = *list;
	while (l != NULL) {

		if (strcmp(l -> id, id) == 0) {
			MList aux_msg = malloc(sizeof(struct msg));
			aux_msg -> ip_sender = malloc(sizeof(char) * strlen(ip));
			strcpy(aux_msg -> ip_sender, ip);
			aux_msg -> port = port;
			strcpy(aux_msg -> info.topic, msg.topic);
			aux_msg -> info.tip = msg.tip;
			strcpy(aux_msg -> info.msg, msg.msg);
			aux_msg -> next = l -> messages;
			l -> messages = aux_msg;
			break; 
		}

		l = l -> next;
	}
}



int main(int argc, char *argv[])
{
	int sockfd1, sockfd2, newsockfd, portno;
	char *buffer = malloc(sizeof(char) * BUFLEN);
	struct sockaddr_in serv_addr, cli_addr;
	int n, i, ret;
	socklen_t clilen;

	fd_set read_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar
	int fdmax;			// valoare maxima fd din multimea read_fds

	if (argc < 2) {
		usage(argv[0]);
	}

	// se goleste multimea de descriptori de citire (read_fds) si multimea temporara (tmp_fds)
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);
	//pt cele doua socket-uri
	sockfd1 = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd1 < 0, "socket");

	sockfd2 = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(sockfd2 < 0, "socket");

	portno = atoi(argv[1]);
	DIE(portno == 0, "atoi");

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	ret = bind(sockfd1, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind");

	ret = listen(sockfd1, MAX_CLIENTS);
	DIE(ret < 0, "listen");

	ret = bind(sockfd2, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind");

	// se adauga noul file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds
	FD_SET(sockfd1, &read_fds);
	FD_SET(sockfd2, &read_fds);
	FD_SET(0, &read_fds);
	if(sockfd1 > sockfd2) {
		fdmax = sockfd1;
	} else {
		fdmax = sockfd2;
	}

	SList subscribers_list = NULL;

	int yes = 1;
	setsockopt(sockfd1, IPPROTO_TCP, TCP_NODELAY, (char *) &yes, sizeof(int));

	while (1) {
		tmp_fds = read_fds; 
		
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);

		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				if(i == 0) {
					read(0, buffer, BUFLEN);
					//daca am primit comanda "exit" de la tastatura
					if(strcmp(buffer, "exit") == 0) {
						int j;
						for (j = 3; j <= fdmax; j++) {
							if(FD_ISSET(j, &tmp_fds))
								close(j);
						}
						break;
					}
				}
				if (i == sockfd1) {
					// a venit o cerere de conexiune pe socketul inactiv (cel cu listen),
					// pe care serverul o accepta
					clilen = sizeof(cli_addr);
					newsockfd = accept(sockfd1, (struct sockaddr *) &cli_addr, &clilen);

					// se adauga noul socket intors de accept() la multimea descriptorilor de citire
					FD_SET(newsockfd, &read_fds);
					if (newsockfd > fdmax) { 
						fdmax = newsockfd;
					}

					memset(buffer, 0, BUFLEN);
					recv(newsockfd, buffer, BUFLEN, 0);
					printf("New client %s connected from %s:%d.\n", buffer, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

					if(search_id(subscribers_list, buffer) == 0) {

						//adaug clientul in lista de subscribers
						SList sub = malloc(sizeof(struct subscriber));
						strcpy(sub -> id, buffer);
						sub -> socket = newsockfd;
						sub -> topics = NULL;
						sub -> messages = NULL;
						sub -> next = NULL;
						add(sub, &subscribers_list);
					} else {
						//retin noul socket al clientului 
						set_sock(&subscribers_list, buffer, newsockfd);

						//ii trimit toate mesajele stocate, daca exista
						empty_messages(&subscribers_list, buffer);
					}

				} else {
					if (i == sockfd2) {
						//client UDP
						memset(buffer, 0, BUFLEN);
						n = recvfrom(i, buffer, BUFLEN, 0, (struct sockaddr *) &cli_addr, &clilen);

						//prelucrerea mesajului primit:
						// daca un client TCP abonat la topicul respectiv e logat, trimit mesajul catre el
						// daca un client TCP abonat la topic e offline dar are optiunea sf activata, 
						//stochez mesajul pt a fi trimis cand acesta se va conecta

						struct UdpMsg mesaj = (*(struct UdpMsg *)buffer);

						SList aux = subscribers_list;
						while (aux != NULL) {
							if (has_topic(aux -> topics, mesaj.topic) >= 1) {
								if (aux -> socket > 0) {
									//client activ, ii trimit mesajul
									memset(buffer, 0, BUFLEN);
									char *aux_buf;
									strcpy(buffer, inet_ntoa(cli_addr.sin_addr));
									aux_buf = buffer + strlen(inet_ntoa(cli_addr.sin_addr));
									strcpy(aux_buf, ":");
									aux_buf++;
									char *port = malloc(10 * sizeof(char));
									sprintf(port, "%d", cli_addr.sin_port);
									strcpy(aux_buf, port);
									aux_buf += strlen(port);
									free(port);
									strcpy(aux_buf, " - ");
									aux_buf += 3;
									strcpy(aux_buf, mesaj.topic);
									aux_buf += strlen(mesaj.topic);
									strcpy(aux_buf, " - ");
									aux_buf += 3;
									int buf_size = 0;
									if(mesaj.tip == 0) {
										strcpy(aux_buf, "INT - ");
										aux_buf += 6;
										buf_size = 5;
										memcpy(aux_buf, mesaj.msg, 5);
									} else {
										if(mesaj.tip == 1) {
											strcpy(aux_buf, "SHORT_REAL - ");
											aux_buf += 13;
											buf_size = 2;
											memcpy(aux_buf, mesaj.msg, 2);
										} else {
											if (mesaj.tip == 2) {
												strcpy(aux_buf, "FLOAT - ");
												aux_buf += 8;
												buf_size = 6;
												memcpy(aux_buf, mesaj.msg, 6);

											} else {
												strcpy(aux_buf, "STRING - ");
												aux_buf += 9;
												memcpy(aux_buf, mesaj.msg, 1500);
											}
										}
									}

									buf_size += strlen(buffer);
									send(aux -> socket, &buf_size, sizeof(int), 0);
									send(aux -> socket, buffer, buf_size, 0);

								} else {
									//daca clientul are activa optiunea save&forward
									if (has_topic(aux -> topics, mesaj.topic) == 2) {
										//client deconectat, ii stochez mesajul
										save_msg(&subscribers_list, aux -> id, mesaj, inet_ntoa(cli_addr.sin_addr), cli_addr.sin_port);
									}
								}
							}
							aux = aux -> next;
						}

					} else {

						// s-au primit date pe unul din socketii de client,
						// asa ca serverul trebuie sa le receptioneze
						memset(buffer, 0, BUFLEN);
						n = recv(i, buffer, BUFLEN, 0);

						if (n == 0) {
							// conexiunea s-a inchis

							printf("Client %s disconnected.\n", search_sock(subscribers_list, i));
							log_out(&subscribers_list, i);
							close(i);
						
							// se scoate din multimea de citire socketul inchis 
							FD_CLR(i, &read_fds);
						} else {
							//mesaj TCP
							char *token = strtok(buffer, " ");
							if (strcmp(token, "subscribe") == 0) {
								//client subscribes to new topic
								token = strtok(NULL, " ");
								add_topic(&subscribers_list, token, strtok(NULL, " "), search_sock(subscribers_list, i));
							} else {
								if (strcmp(token, "unsubscribe") == 0) {
									token = strtok(NULL, " ");
									unsubscribe(&subscribers_list, search_sock(subscribers_list, i), token); 
								}
							}
						}
					}
				}
			}
		}
	}

	close(sockfd1);
	close(sockfd2);
	
	return 0;
}