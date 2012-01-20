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
#include <linux/i2c-dev.h>/* for I2C_SLAVE */
#include <linux/i2c.h>
#include <stdio.h>
#include <sys/ioctl.h> //pour x86
#include "i2C.h"
#include "Params.h"

int fdc = -1;

unsigned char commandePulseSonar[3];
unsigned char commandeLectureSonar[3];

struct i2c_msg m[2];
struct i2c_rdwr_ioctl_data rdwr;
unsigned char rbuf[5] = {0};//read buffer
unsigned char wbuf[4] = {0};//write buffer
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

int I2C_Envoyer_Commande_Moteur(int moteur, unsigned char commande[1]){

	switch(moteur){
	case 1:{
		ioctl(fdc, I2C_SLAVE, 0x52);
		write(fdc, commande[0], 1);
		break;
	}
	case 2:{
		ioctl(fdc, I2C_SLAVE, 0x54);
		write(fdc, commande[0], 1);
		break;
	}
	case 3:{
		ioctl(fdc, I2C_SLAVE, 0x56);
		write(fdc, commande[0], 1);
		break;
	}
	case 4:{
		ioctl(fdc, I2C_SLAVE, 0x58);
		write(fdc, commande[0], 1);
		break;
	}
	default:{
		printf("Adresse moteur inconnue");
		return -1;
	}
	}
	return (0);
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
	ioctl(fdc,I2C_SLAVE, 0xE0);
	commandePulseSonar[0]=0x55; //Pour une réponse en cm voir datasheet
	commandePulseSonar[1]=GAIN_MAX_SONAR;
	commandePulseSonar[2]=RANGE_SONAR;
	write(fdc,commandePulseSonar,3);
	return 0;
}

int I2C_Envoyer_Lecture_Sonar(){


	wbuf[0] = 0x02;//Register
	wbuf[1] = 0x02;//byte to read
	m[0].len = 2;//2 byte write
	m[0].flags = 0;//prepare to write
	m[0].addr = 0xE0;//slave addr
	m[0].buf = wbuf;//write buffer

	m[1].len = 2;//1 byte read
	m[1].flags = I2C_M_RD;//prepare to read
	m[1].addr = 0xE0;//slave addr
	m[1].buf = rbuf;//read buffer

	rdwr.msgs= m;//first msg
	rdwr.nmsgs=2;// number of msgs

	if (ioctl(fdc,I2C_RDWR,&rdwr) < 0) {
		printf("Ne peut pas lire les données sonar\n");
		return -1;
	}
	return (int)((int)rbuf[0]*255+(int)rbuf[1]);
}
