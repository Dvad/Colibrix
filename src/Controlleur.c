#include "Misc.h"
#include "Params.h"
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <linux/i2c-dev.h> // New
#include <unistd.h>  // New
#include <stdint.h>  // New
#include <termios.h>
#include "i2C.h"

//void usleep(int us); // New: commented out
//void close(int id);  // New: commented out
//size_t getpagesize(void); // New: commented out
//Attention IL faut mettre des pointeurs dans les write.
//--Variables communes
int cycle;
int requests;
int commandeNumerique[4];
int derCommandeNumerique[4];
int lastSend[4];
int requestSend[4];

unsigned char buf4[4];

int estInitialiseControlleur;


//
// Remet les consignes à Zero et envoie cette consigne au moteur
//
void Controlleur_Initialise(int testMoteurs) {

	estInitialiseControlleur = 0;

	if (estInitialiseI2C==0){
		printf("ERREUR:I2C non initialisé\n");
		return;
	}
	cycle = 0;

	for (i = 0; i < 4; i++) { //Remet les consignes à Zero
		lastSend[i] = 0;
		requestSend[i] = 0;
		buf4[i]=0;
	}

	printf("Ecriture d'une commande zéro...");
	fflush(stdout);
	int ret=I2C_Envoyer_Commande_Tout_Moteur(buf4);
	if (ret < 0) {
		printf("\nERREUR: write a renvoyé ret=%i au lieu de 12 !\n", ret);
		fflush(stdout);
	} else {
		printf(" OK\n");
	}

	estInitialiseControlleur = 1;
}


//
// mode=0: Envoi de la valeur zero
// mode=1: Envoi de la valeur de rotation lente
// mode=2: Envoi de la valeur réelle, saturée.
//


void Controlleur_Envoi(int mode) {

	if (!estInitialiseControlleur) {
		return;
	}

	//
	//--1ere étape: calcul de la commande numérique
	//
	if (mode == 0) {
		// Envoi de la valeur ZERO
		for (i = 0; i < 4; i++) {
			commandeNumerique[i] = 0;
		}
	} else if (mode == 1) {
		// Envoi de la valeur ROTATION_LENTE
		for (i = 0; i < 4; i++) {
			commandeNumerique[i] = ROTATION_LENTE;
		}
	} else if (mode == 2) {
		// Envoi de valeur calculée a partir de la commande
		for (i = 0; i < 4; i++) {

			commandeNumerique[i] = (int)(0.1F * Commande[i]);

			//--Saturation 0 - 255
			if (commandeNumerique[i] < ROTATION_LENTE) {
				commandeNumerique[i] = ROTATION_LENTE;
			} else if (commandeNumerique[i] > ROTATION_MAXI) {
				commandeNumerique[i] = ROTATION_MAXI;
				printf("BNUM");
			}
		}
	} else {
		printf(
				"\nERREUR GRAVE: La variable 'mode' n'est pas dans le domaine attendu (mode=%i) !\n",
				mode);
		return;
	}

	//
	//--2eme étape: détermination des moteurs à commander
	//
	/* MEthode non utilisé
	 * 	requests = 0;
	for (i = 0; i < 4; i++) {

		//printf("%i, %i, %i, %i VS %i, %i, %i, %i", )

		if (commandeNumerique[i] != derCommandeNumerique[i] || lastSend[i]
				> MAX_WAIT_BEF_SEND) {
			derCommandeNumerique[i] = commandeNumerique[i];
			requestSend[i] = 1; // Enverra la commande
			requests++;
			lastSend[i] = 0;
		} else {
			lastSend[i]++;
			requestSend[i] = 0;
		}
	}
	 */

	buf4[0]=commandeNumerique[0];
	buf4[1]=commandeNumerique[1];
	buf4[2]=commandeNumerique[2];
	buf4[3]=commandeNumerique[3];

	if(modeI2C == 1) {
		int ret = I2C_Envoyer_Commande_Tout_Moteur(buf4);
		if (ret < 0) {
			printf("_");

		}

	}
}
int Test_Moteur(){
	int NB_ITERATIONS= 500;
	int WAIT_MS=500;
	unsigned char buf1[1]={0x3C};

	printf("Rotation moteur 1...");
	fflush(stdout);
	for (i = 0; i < NB_ITERATIONS; i++) {
		I2C_Envoyer_Commande_Moteur(1, buf1);
		usleep(WAIT_MS);
	}
	printf(" OK\n");

	printf("Rotation moteur 2...");
	fflush(stdout);
	for (i = 0; i < NB_ITERATIONS; i++) {
		I2C_Envoyer_Commande_Moteur(2, buf1);
		usleep(WAIT_MS);
	}
	printf(" OK\n");

	printf("Rotation moteur 3...");
	fflush(stdout);
	for (i = 0; i < NB_ITERATIONS; i++) {
		I2C_Envoyer_Commande_Moteur(3, buf1);
		usleep(WAIT_MS);
	}
	printf(" OK\n");

	printf("Rotation moteur 4...");
	fflush(stdout);
	for (i = 0; i < NB_ITERATIONS; i++) {
		I2C_Envoyer_Commande_Moteur(4, buf1);
		usleep(WAIT_MS);
	}
	printf(" OK\n");

	printf("Si tout les moteurs ont tournés, appuyer sur entrée.\n");
	fflush(stdout);
	Misc_WaitEnter();
	return 0;
}
int Controlleur_VMoyen() {
	return (int)(2.5F * (commandeNumerique[0] + commandeNumerique[1] 
	                                                              + commandeNumerique[2] + commandeNumerique[3]));
}

void Controlleur_PrintCmd() {
	printf(" Comm: %4i  %4i  %4i  %4i", commandeNumerique[0],
			commandeNumerique[1], commandeNumerique[2], commandeNumerique[3]);
}

void Controlleur_Termine() {
	estInitialiseControlleur = 0;
}

