#include "Params.h"
#include "Pilote.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>


//////////////////////////// INTERDICTION D'UTILISER DSE VARIABLES TYPES "i" DANS CE THREAD //////////////


#define PORT "1234" //Port D'�coute
// number of incoming cnxs to buffer
#define BACKLOG 1 

/*
void close(int id);
void usleep(int us);
int read(int id, unsigned char* buf, int length);
*/


pthread_t thread;
void* CommWiFi(void* data);
int termine;

int CommWiFi_Initialise() {
	
	
	printf("D�marrage de la communication WiFi...");
	termine = 0;
	pthread_create(&thread, NULL, CommWiFi, NULL);
	usleep(100*1000); // 100ms
	return 1;
	
	
	//return -1;

}

void CommWiFi_Termine() {
	termine = 1;
	printf("Fermeture de la communication wifi...\n");
}

void* CommWiFi(void* data) {

	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	int yes=1;
	int sockfd;
	int rv;
	int MAX_LENGTH_S = 256; //longueur du message ENVOYE
	int MAX_LENGTH = 31; //longueur du message RECU
	char msgwifienv[MAX_LENGTH_S+1];
	char msg[MAX_LENGTH + 1]; //buffer contenant les donn�es recus.
	int CHARS_PAR_INT = 6; //nb de carac�re repr�sentant les ordres d'assiette.
	//
	// Initialize the socket
	//
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	rv = getaddrinfo(NULL, PORT, &hints, &servinfo);
	if (rv == -1) {
		printf("\nERREUR GRAVE: Wifi error on getaddrinfo\n");
		return (void*)-1;
	}

	// loop through all the results and bind to the first we can
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))
				== -1) {
			printf("\nServeur WiFi: socket\n");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))
				== -1) {
			printf("\nERREUR GRAVE: setsockopt\n");
			return (void*)-1;
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			printf("\nServeur WiFi: bind\n");
			continue;
		}
		break;
	}

	if (p == NULL) {
		printf("\nERREUR GRAVE: server: failed to bind\n");
		return (void*)-1;
	} else {
		printf(" OK\n");
	}
	freeaddrinfo(servinfo); // all done with this structure


	//
	// Declare the socket as a listener
	//
	if (listen(sockfd, BACKLOG) == -1) {
		printf("listen");
	}

	//
	// Wait for connexions : infinite loop
	//
	printf("\nWIFI: waiting for connections on port %s...\n", PORT);

	for (;;) {
		// check for an incoming connexion, and accept it
		sin_size = sizeof their_addr;
		int new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1)
			continue;

		printf("\nWIFI: Accepted a cnx");

		
		while(new_fd != -1) {
			if (termine) {
				break;
			}
			// new_fd now contains the file descriptor of the established new connexion
			// do something with it : get 3 chars, and give them back 

			read(new_fd, msg,MAX_LENGTH); //lit ce qui arrive sur le socket
			msg[MAX_LENGTH]= '\0';// important : caract�re de fin de message

			attenteSignal = 0; // La liaison est active ! (Bah oui! on a recu des donn�es non?)

			//
			// Detectons s'il s'agit d'un ordre d'assiette
			//
			if (msg[0] == 'C' && msg[1] == 'L' && msg[1 + 1 * (CHARS_PAR_INT + 1)] == 'R' && msg[1 + 2 * (CHARS_PAR_INT + 1)] == 'T' && msg[1 + 3 * (CHARS_PAR_INT + 1)] == 'A' && msg[1 + 4 * (CHARS_PAR_INT + 1)] == 'S') {

				char tempCharsForInt[CHARS_PAR_INT + 1];
				tempCharsForInt[CHARS_PAR_INT] = '\0';
				int a;
				int c;
				for (a = 0; a < 4; a++) { 
					for (c = 0; c < CHARS_PAR_INT; c++) {
						tempCharsForInt[c] = msg[2 + a * (CHARS_PAR_INT + 1) + c];
					}
					float value = atof(tempCharsForInt);
					//printf("Valeur consigne %f \n", value);
					//float val = 0.00001F * (float)intValue;

					OrdreLRTA[a] = value;
				}
				Pilote_AppliqueOrdreLRTA();
				
			} else if (msg[0] == 'A' && msg[1] == 'T' && msg[2] == 'T') {
				printf("\nRECEPTION ORDRE ATTERISSAGE\n");
				if (state == 4 || state == 3) {
					printf("Atterissage...\n");
					state = 5;
				}
			} else if (msg[0] == 'D' && msg[1] == 'E' && msg[2] == 'C') {
				printf("\nRECEPTION ORDRE DECOLLAGE\n");
				if (state == 2) {
					printf("D�collage...\n");
					Vactu = 0.0F;
					state = 3;
				}

			} else if (msg[0] == 'S' && msg[1] == 'T' && msg[2] == 'O'
					&& msg[3] == 'P') {
				printf("\nRECEPTION ORDRE ARRET D'URGENCE\n");
				state = -1;
			}

			else {
				printf("\nERREUR WIFI: R�ception d'un ordre inconnu: %s\n", msg);
				fflush(stdout);

			}
			//printf("\nWIFI: R�ception de l'ordre : %s\n", msg);

			//Ces quelques lignes permettent d'envoyer des donn�es � la base.
			snprintf(msgwifienv,MAX_LENGTH_S+1,"D"
					"CL%fR%fT%fA%f"
					"PL%fR%fT%fA%f"
					"UL%fR%fT%fA%f"
					"OL%fR%fT%fA%f",
					1000000*ConsLRTA[0],1000000*ConsLRTA[1],1000000*ConsLRTA[2],1000000*ConsLRTA[3],
					1000000*PosLRTA[0],1000000*PosLRTA[1],1000000*PosLRTA[2],1000000*PosLRTA[3],
					1000000*PosLRTA[0],1000000*PosLRTA[1],1000000*PosLRTA[2],1000000*PosLRTA[3],
					1000000*OrdreLRTA[0],1000000*OrdreLRTA[1],1000000*OrdreLRTA[2],1000000*OrdreLRTA[3]);
			if((send(new_fd,msgwifienv,MAX_LENGTH_S+1,MSG_NOSIGNAL)) < 0) break;
			sleep(0.1);
		}

	
		close(new_fd); 	// terminate the cnx
		printf("\nWIFI: Cnx terminated !");
		state = -1; //Arret d'urgence! car plus de cnx, d�licat si l'ordre �tait important
		printf("\nWIFI: waiting for connections on port %s...\n", PORT); //on attend une nouvelle connection

		if (termine) {
			break;
		}
	}
	return (void*)1;
}
