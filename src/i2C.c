/*
 * i2C.c
 *
 *  Created on: 19 janv. 2012
 *      Author: dav
 *      Ces fonctions gèrent les périphériques commandés en I2C et initialise le port I2C
 */
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>/* for I2C_SLAVE */
#include <linux/i2c.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h> //pour x86
#include "i2C.h"
#include "Params.h"

int fdc = -1;

unsigned char commandePulseSonar[7];
unsigned char commandeLectureSonar[5];
int ret;

struct i2c_msg m[2];
struct i2c_rdwr_ioctl_data rdwr;
unsigned char rbuf[5] = {0};//read buffer
unsigned char wbuf[4] = {0};//write buffer
int estInitialiseI2C;
int I2C_Initialise(){
	printf("Ouverture de l'interface I2C...");
	fdc = open("/dev/i2c-3", O_RDWR); // | O_NONBLOCK); // | O_NOCTTY | O_NDELAY);
	if (fdc < 0) {
		printf("\nERREUR: open() à renvoyé %i !\n", fdc);
		return fdc;
	} else {
		printf(" OK\n");
		estInitialiseI2C=1;
		return 0;
	}
}

int I2C_Termine(){
	estInitialiseI2C=0;
	printf("Fermeture de l'interface I2C...");
	fdc = close("/dev/i2c-3");
	if (fdc < 0) {
		printf("\nERREUR: close() à renvoyé %i !\n", fdc);
		return -1;
	} else {
		printf(" OK\n");
		estInitialiseI2C=0;
		return 0;
	}
}

int I2C_Envoyer_Commande_Moteur(int moteur, unsigned char commande[1]){

	switch(moteur){
	case 1:{
		ioctl(fdc, I2C_SLAVE, 0x52); //TODO vérifier que l'adresse est correctement écrite, la gumst a tendance à inverser les bit lors de l'envoi ex
		write(fdc, commande[0], 1);  // 0xE0=0b1110 0000 envoi en réalité 0x70=0b0111 0000
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

//
//Envoie le début de la lecture du sonar. Attention pas testé au sein de ce programme.
//
int I2C_Envoyer_Pulse_Sonar(){
	ioctl(fdc,I2C_SLAVE, 0x70);
	//On lui rappelle ces propriété au passage/ On pourra éventuellement mettre
	//ceci dans l'initialisation
	commandePulseSonar[0]=0X01;
	commandePulseSonar[1]=GAIN_MAX_SONAR;
	commandePulseSonar[2]=RANGE_SONAR_REGISTER;
	if (write(fdc,commandePulseSonar,3)<0) {
		printf("Erreur communication Sonar : %s\n",strerror(errno));
		return -1;

	}

	commandePulseSonar[0]=0x00; //registre
	commandePulseSonar[1]=0x51; //Pour une réponse en cm voir datasheet; //registre
	if (write(fdc,commandePulseSonar,2)<0) {
		printf("Erreur communication Sonar : %s\n",strerror(errno));
		return -1;

	}

	return 0 ;
}

//
//Envoie une tentative de lecture sur le sonar, si celui si n'a pas fini la mesure il renvoie 255 ou bien fait une erreur
//On renvoie 0 dans ce cas, 1 sinon;
//
int GotSonarData(){
	ioctl(fdc,I2C_SLAVE, 0x70);
	wbuf[0]=0x00; //registre
	write(fdc,wbuf,1);
	if (read(fdc,rbuf,1)<0) return 0;
	else return 1;
}

	//
//Renvoie l'entier représentant la dernière lecture Sonar, -1 en cas d'erreur de lecture,
//ou en cas de mesure non terminé, il faut alors continuer de tenter de lire une valeur correcte.
//(Prochaine boucle)
//
int I2C_Envoyer_Lecture_Sonar(){
	/*
	//
	//Ici on n'utilise une fonction du driver i2C qui permet d'initier l'écrture (Pour sélectionner le registre voulu)
	//En même temps que la lecture (Pour lire à partir de ce registre dans la mémoire du sonar)
	// Cela règle les problème de conflit éventuel avec les vario i2C sur le même bus.
	// Pour sélectionner le registre
	//
	wbuf[0] = 0x02	;//Registre qui servira de début de lecture!
	m[0].len = 1;//2 byte write
	m[0].flags = 0;
	m[0].addr = 0x70;//slave addr
	m[0].buf = wbuf;//write buffer
	//
	//On lit m[1].len byte à partir du registre
	//
	m[1].len = 2;//1 byte read
	m[1].flags = I2C_M_RD;//prepare to read
	m[1].addr = 0x70;//slave addr
	m[1].buf = rbuf;//read buffer

	rdwr.msgs= m;//first msg
	rdwr.nmsgs=2;// number of msgs

	if (ioctl(fdc,I2C_RDWR,&rdwr) < 0) {
		printf("Ne peut pas lire les données sonar\n");
	}*/
	ioctl(fdc,I2C_SLAVE, 0x70);
	wbuf[0]=0x00; //registre
	write(fdc,wbuf,1);
	if(read(fdc,rbuf,4)==4){
		if(DEBUG_SONAR==1) printf("AltitudeSonar : %i\n",(int)rbuf[2]*255+(int)rbuf[3]);
		return ((int)rbuf[2]*255+(int)rbuf[3]);
	}
	else{
		return -1;
	}
}
