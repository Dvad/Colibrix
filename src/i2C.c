/*
 * i2C.c
 *
 *  Created on: 19 janv. 2012
 *      Author: dav
 *      Ces fonctions gère les périphérique commandés en I2C et initialise le port I2C
 */
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/i2c-dev.h> /* for I2C_SLAVE */
#include <stdio.h>

int fdc = -1;
int estInitialiseI2C;
int I2C_Initialise(){
	estInitialiseI2C=0;
	printf("Ouverture de l'interface I2C...");
		fdc = open("/dev/i2c-3", O_RDWR); // | O_NONBLOCK); // | O_NOCTTY | O_NDELAY);
		if (fdc < 0) {
			printf("\nERREUR: open() é renvoyé %i !\n", fdc);
			return fdc;
		} else {
			printf(" OK\n");
			estInitialiseI2C=1;
			return 0;
		}
}

int I2C_Envoyer_Commande_Moteur(int moteur, unsigned char commande){
	if(commande >255){
		printf("Commande envoyé au moteur hors de portée");
						return -1;
	}
	else{
		switch(moteur){
		case 1:{
			ioctl(fdc, I2C_SLAVE, 0x52);
			write(fdc, commande, 1);
			break;
		}
		case 2:{
			ioctl(fdc, I2C_SLAVE, 0x54);
			write(fdc, commande, 1);
			break;
		}
		case 3:{
			ioctl(fdc, I2C_SLAVE, 0x56);
			write(fdc, commande, 1);
			break;
		}
		case 4:{
			ioctl(fdc, I2C_SLAVE, 0x58);
			write(fdc, commande, 1);
			break;
		}
		default:{
			printf("Adresse moteur inconnue");
			return -1;
		}
		}
		return (0);
	}
}

int I2C_Envoyer_Commande_Tout_Moteur(unsigned char commande[4]){
	ioctl(fdc, I2C_SLAVE, 0x52);
	write(fdc, commande[0], 1);

	ioctl(fdc, I2C_SLAVE, 0x54);
	write(fdc, commande[1], 1);

	ioctl(fdc, I2C_SLAVE, 0x56);
	write(fdc, commande[2], 1);

	ioctl(fdc, I2C_SLAVE, 0x58);
	write(fdc, commande[3], 1);

	return 0;
}

int I2C_Envoyer_Pulse_Sonar(){
//TODO Pulse SOnar
	return 0;
}

int I2C_Envoyer_Lecture_Sonar(){
	//TODO Ordre Lecture Sonar
	return 0;
}
