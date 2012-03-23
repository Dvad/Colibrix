#include "Params.h"
#include "Pilote.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <fcntl.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include "Centrale.h"

//////////////////////////// INTERDICTION D'UTILISER DSE VARIABLES TYPES "i" DANS CE THREAD //////////////


#define PORT "1234" //Port D'écoute
// number of incoming cnxs to buffer
#define BACKLOG 1 

pthread_t thread;
void* CommWiFi(void* data);
int termine;
int askedState =0;
int askedStateAnc=0;
int isThereAskedState=0;
struct timeval datationWifi;
long long int dateWifiInitial;
long long int dateWifi;
long long int dateRelativeWifi;
int controlWifi;

int CommWiFi_Initialise() {
	
	
	printf("Démarrage de la communication WiFi...");
	termine = 0;
	gettimeofday(&datationWifi,NULL);
	dateWifiInitial=datationWifi.tv_sec *1000000 + (datationWifi.tv_usec);
	printf("\nDate Wifi Initial : %lli \n",dateWifiInitial);
	fflush(stdout);
	controlWifi=0;
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
	int MAX_LENGTH = 33; //longueur du message RECU
	char msgwifienv[MAX_LENGTH_S+1];
	char msg[MAX_LENGTH + 1]; //buffer contenant les données recus.
	int CHARS_PAR_INT = 6; //nb de caracére représentant les ordres d'assiette.
	msg[MAX_LENGTH]= '\0';// important : caractére de fin de message
	msgwifienv[MAX_LENGTH_S]='\0';
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


			attenteSignal = 0; // La liaison est active ! (Bah oui! on a recu des données non?)

			//
			// Detectons s'il s'agit d'un ordre d'assiette
			//
			if (msg[0] == 'C' && msg[1] == 'L' && msg[1 + 1 * (CHARS_PAR_INT + 1)] == 'R'
					&& msg[1 + 2 * (CHARS_PAR_INT + 1)] == 'T' && msg[1 + 3 * (CHARS_PAR_INT + 1)] == 'A'
							&& msg[1 + 4 * (CHARS_PAR_INT + 1)] == 'E'
							&& msg[1 + 3 + 4 * (CHARS_PAR_INT + 1)] == 'S') {

				char tempCharsForInt[CHARS_PAR_INT + 1];
				tempCharsForInt[CHARS_PAR_INT] = '\0';
				int a;
				int c;
				for (a = 0; a < 4; a++) { 
					for (c = 0; c < CHARS_PAR_INT; c++) {
						tempCharsForInt[c] = msg[2 + a * (CHARS_PAR_INT + 1) + c];
					}
					float value = atof(tempCharsForInt)/1000;
					//printf("Valeur consigne %f \n", value);

					ConsLRTA[a] = value;
				}
				//Pilote_AppliqueOrdreLRTA();
				tempCharsForInt[0]=msg[1+ 1 + 4 * (CHARS_PAR_INT + 1)];
				tempCharsForInt[1]=msg[1+ 2 + 4 * (CHARS_PAR_INT + 1)];
				tempCharsForInt[2]='\0';
				askedState=atoi(tempCharsForInt);
				isThereAskedState=1;
			} else if (msg[0] == 'A' && msg[1] == 'T' && msg[2] == 'T') {
				printf("\nRECEPTION ORDRE ATTERISSAGE\n");
				if (state == 4 || state == 3) {
					printf("Atterissage...\n");
					state = 5;
				}
			} else if (msg[0] == 'D' && msg[1] == 'E' && msg[2] == 'C') {
				printf("\nRECEPTION ORDRE DECOLLAGE\n");
				if (state == 2) {
					printf("Décollage...\n");
					Vactu = 0.0F;
					state = 3;
				}

			} else if (msg[0] == 'S' && msg[1] == 'T' && msg[2] == 'O'
					&& msg[3] == 'P') {
				printf("\nRECEPTION ORDRE ARRET D'URGENCE\n");
				state = -1;
			}

			else {
				printf("\nERREUR WIFI: Réception d'un ordre inconnu: %s\n", msg);
				fflush(stdout);

			}
			//printf("\nWIFI: Réception de l'ordre : %s\n", msg);
			//fflush(stdout);
			//Ces quelques lignes permettent d'envoyer des données à la base.
			gettimeofday(&datationWifi,NULL);
			dateWifi=datationWifi.tv_sec *1000000 + (datationWifi.tv_usec);
			dateRelativeWifi=(dateWifi-dateWifiInitial);

			snprintf(msgwifienv,MAX_LENGTH_S,"D"
					"CL%+06.6iR%+06.6iT%+06.6iA%+06.6i"
					"PL%+06.6iR%+06.6iT%+06.6iA%+06.6i"
					"UL%+06.6iR%+06.6iT%+06.6iA%+06.6i"
					"OL%+06.6iR%+06.6iT%+06.6iA%+06.6i"
					"E%+iD%+.10lli"
					/*"Ac1%fAc2%fAc3%f"
					"Vi1%fVi2%fVi3%f"
					"Po1%fPo2%fPo3%f"
					"B%f"*/
					,
					(int)(100000*ConsLRTA[0]),(int)(100000*ConsLRTA[1]),(int)(100000*ConsLRTA[2]),(int)(100000*ConsLRTA[3]),
					(int)(100000*PosLRTA[0]),(int)(100000*PosLRTA[1]),(int)(100000*PosLRTA[2]),(int)(100000*PosLRTA[3]),
					(int)(100000*ULRTA[0]),(int)(100000*ULRTA[1]),(int)(100000*ULRTA[2]),(int)(100000*ULRTA[3]),
					(int)(100000*OrdreLRTA[0]),(int)(100000*OrdreLRTA[1]),(int)(100000*OrdreLRTA[2]),(int)(100000*OrdreLRTA[3])
					,state,dateRelativeWifi/*,1000000*Acc[0],1000000*Acc[1],1000000*Acc[2],
					1000000*Vit[0],1000000*Vit[1],1000000*Vit[2],
					1000000*Pos[0],1000000*Pos[1],1000000*Pos[2],
					1000000*AltitudeBaro*/);

			if((send(new_fd,msgwifienv,MAX_LENGTH_S+1,MSG_NOSIGNAL)) < 0) break;
			sleep(0.1);
		}

	
		close(new_fd); 	// terminate the cnx
		printf("\nWIFI: Cnx terminated !");
		//state = -1; //Arret d'urgence! car plus de cnx, délicat si l'ordre était important
		printf("\nWIFI: waiting for connections on port %s...\n", PORT); //on attend une nouvelle connection

		if (termine) {
			break;
		}
	}
	return (void*)1;
}

void CommWifi_AppliqueEtat(){
	//On vérifie que l'état demandé est d'actualité
	if(state==askedState || askedState==askedStateAnc){
		isThereAskedState=0;
		askedStateAnc=askedState;
		return;
	}
	askedStateAnc=askedState;

	//------------------------------------------------
	//--------------- COMMANDE "w" -----------------
	//------------------------------------------------
	if ((isThereAskedState==1 && askedState==0)) {
		if (state == -1) {
			printf("\n\nErreur ignoré !\n");
			state = 0;
		} else {
			printf("ERREUR: La commande ne peut étre appliqué dans ce contexte.\n");
		}
		isThereAskedState=0;
		fflush(stdout);
	}
	//------------------------------------------------
	//--------------- COMMANDE "x" -----------------
	//------------------------------------------------
	else if ((isThereAskedState==1 && askedState==1)) {
		if (state == -1 || state == 0 || state == 1 ) {
			printf("\n\nInitialisation... OK\n");
			//--RAZ attitude et position
			Centrale_RAZAttitude();
			//Centrale_RAZPosition();

			videBufferSonar();


			//--RAZ des consignes
			ConsLRTA[0] = 0.0F; // 0 en lacet
			ConsLRTA[3] = 0.0F; // 0 en altitude
			if (ConsLRTA[1] != 0.0F || ConsLRTA[2] != 0) {
				printf("\n\nWARNING: Initialisé avec consignes RT non nulles !!\n\n");
			}

			if (state == -1) {
				printf("Il faut encore faire 'clr'\n");
			} else {
				printf("Pret pour mise en rotation\n");
				state = 1;
			}
			fflush(stdout);
		} else {
			printf("\n\nERREUR: La commande ne peut étre appliqué dans ce contexte.\n");
		}
		isThereAskedState=0;
		fflush(stdout);
	}
	//------------------------------------------------
	//--------------- COMMANDE "c" -----------------
	//------------------------------------------------
	else if ((isThereAskedState==1 && askedState==2)) {
		if (state == 1) {
			printf("\n\nMise en rotation...\n");
			state = 2;
		} else {
			printf("\n\nERREUR: La commande ne peut étre appliquée dans ce contexte.\n");
		}
		isThereAskedState=0;
		fflush(stdout);
	}
	//------------------------------------------------
	//--------------- COMMANDE "v" -----------------
	//------------------------------------------------
	else if ((isThereAskedState==1 && askedState==3)) {
		if (state == 2) {
			printf("\n\nDécollage...\n");
			Vactu = 0.0F;
			state = 3;
		} else {
			printf("\n\nERREUR: La commande ne peut étre appliqué dans ce contexte.\n");
		}
		isThereAskedState=0;
		fflush(stdout);
	}
	//------------------------------------------------
	//--------------- COMMANDE "b" -----------------
	//------------------------------------------------
	else if ((isThereAskedState==1 && askedState==5)) {
		if (state == 4 || state == 3) {
			printf("\n\nAtterissage...\n");
			state = 5;
		} else {
			printf("\n\nERREUR: La commande ne peut étre appliqué dans ce contexte.\n");
		}
		isThereAskedState=0;
		fflush(stdout);
	}

	//------------------------------------------------
	//--------------- COMMANDE "STOP" ----------------
	//------------------------------------------------
	else if ((isThereAskedState==1 && askedState==-1)) {
		state = -1;
		printf("\n !! ARRET D'URGENCE !! \n");
		fflush(stdout);
		isThereAskedState=0;
	}
}
