#include <C:\CSD\workspace\helloworld-oabi\src\Misc.h>
#include <C:\CSD\workspace\helloworld-oabi\src\Params.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdlib.h>

#include <linux/i2c-dev.h> // New
#include <unistd.h>  // New
#include <stdint.h>  // New
#include <termios.h>

//void usleep(int us); // New: commented out
//void close(int id);  // New: commented out
//size_t getpagesize(void); // New: commented out

//--Variables communes
int cycle;
int requests;
int commandeNumerique[4];
int derCommandeNumerique[4];
int lastSend[4];
int requestSend[4];

unsigned char buf1[3];
unsigned char buf2[6];
unsigned char buf3[9];
unsigned char buf4[12];

int estInitialiseControlleur;

//--Variables concernant l'I2C embarquée
int fdControlleur = -1;
volatile unsigned *reg=0;
int fdc = -1;

//--Variables concernant le module USB-I2C
int fdi = -1;

//
//
//---Initialisation module USB-I2C
//
//
void Controlleur_Initialise(int testMoteurs) {

	estInitialiseControlleur = 0;

	cycle = 0;

	for (i = 0; i < 4; i++) {
		lastSend[i] = 0;
		requestSend[i] = 0;
	}

	printf("Ouverture de l'interface USB-I2C...");
	fdi = open("/dev/ttyUSB0", O_RDWR); // | O_NONBLOCK); // | O_NOCTTY | O_NDELAY);
	if (fdi < 0) {
		printf("\nERREUR: open() à renvoyé %i !\n", fdi);
		return;
	} else {
		printf(" OK\n");
	}

	printf("Récupération des paramètres série...");
	struct termios options;
	int ret = tcgetattr(fdi, &options);
	if (ret != 0) {
		printf("\nERREUR: tcgetattr() à renvoyé %i !\n", ret);
		return;
	} else {
		printf(" OK\n");
	}

	options.c_cflag |= CSTOPB; // 2 stopbits !!
	options.c_cflag |= CREAD; // Enable reciever

	//  options.c_cflag |= (CLOCAL | CSTOPB | CREAD | B19200); // 2 stopbits !!

	// --Mode 8N1
	//	options.c_cflag &= ~PARENB;
	//	options.c_cflag &= ~CSTOPB;
	//	options.c_cflag &= ~CSIZE;
	//	options.c_cflag |= CS8;

	cfsetispeed(&options, B19200);
	printf("Réglage de la vitesse de sortie...");
	ret = cfsetospeed(&options, B19200);
	if (ret != 0) {
		printf("\nERREUR: cfsetospeed à renvoyé %i !\n", ret);
		return;
	} else {
		printf(" OK\n");
	}

	printf("Ecriture des paramètres série...");
	ret = tcsetattr(fdi, TCSANOW, &options);
	if (ret != 0) {
		printf("\nERREUR: tcsetattr à renvoyé %i !\n", ret);
		return;
	} else {
		printf(" OK\n");
	}

	buf4[0] = 0x53; // Protocole USB-I2C
	buf4[1] = 0x52; // 0x52-58
	buf4[2] = 0; // 0-255

	buf4[3] = 0x53; // Protocole USB-I2C
	buf4[4] = 0x54; // 0x52-58
	buf4[5] = 0; // 0-255

	buf4[6] = 0x53; // Protocole USB-I2C
	buf4[7] = 0x56; // 0x52-58
	buf4[8] = 0; // 0-255

	buf4[9] = 0x53; // Protocole USB-I2C
	buf4[10] = 0x58; // 0x52-58
	buf4[11] = 0; // 0-255

	printf("Ecriture d'une commande zéro...");
	fflush(stdout);

	ret = write(fdi, buf4, 12);
	if (ret != 12) {
		printf("\nERREUR: write a renvoyé ret=%i au lieu de 12 !\n", ret);
		fflush(stdout);
	} else {
		printf(" OK\n");
	}

	estInitialiseControlleur = 1;
}

//
//
//--Envoi de données module USB-I2C
//
//

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
	requests = 0;
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

	//
	//--On pourrait alors utiliser les cas pour calculer le nombre de commandes a envoyé et gagner quelques octets !
	//
	if (requests < 4 && state > 1) {
		//printf("<");
		//printf("R%i", requests);
	}

	if (1 || requests == 4) { // Cas le plus courant

		//--------------------------------- Commenter ici pour désactiver un moteur
		buf4[2] = commandeNumerique[0];
		buf4[5] = commandeNumerique[1];
		buf4[8] = commandeNumerique[2];
		buf4[11] = commandeNumerique[3];

		if(modeI2C == 1) {
		int ret = write(fdi, buf4, 12);
		if (ret != 12) {
			printf("_");
			
		}
		}
		
	} else if (requests == 3) {

		// etc...
	}

}

//----------------- Code I2C embarquée --------------------


//--------------------------------------------------------------------------
//
//                        Fonctions de bas niveau
//
//--------------------------------------------------------------------------
void *map_phys(off_t addr, int *fdControlleur) {
	off_t page;
	unsigned char *start;

	if (*fdControlleur == -1)
		*fdControlleur = open("/dev/mem", O_RDWR|O_SYNC);
	if (*fdControlleur == -1) {
		perror("open(/dev/mem):");
		return 0;
	}
	page = addr & 0xfffff000;
	start = mmap(0, getpagesize(), PROT_READ|PROT_WRITE, MAP_SHARED,
			*fdControlleur, page);
	if (start == MAP_FAILED) {
		perror("mmap:");
		return 0;
	}
	start = start + (addr & 0xfff);
	return start;
}

inline void setbit(volatile unsigned *adrs, unsigned bit) {
	*adrs |= (1 << bit);
}

inline void clrbit(volatile unsigned *adrs, unsigned bit) {
	*adrs &= ~(1 << bit);
}

inline int getbit(volatile unsigned *adrs, unsigned bit) {
	return !!(*adrs & (1 << bit));
}

inline void cavium_enable_TWI() {
	setbit(&reg[0], 31); // enable TWI
	clrbit(&reg[0], 24); // disable TWI data swap
}

void init_TWI() {
	reg = map_phys(0x71000020, &fdControlleur);
	cavium_enable_TWI();
	reg[0x14/sizeof(unsigned)] = 3; // clear any existing errors
}

void print_TWI_error() {
	int status = (reg[0x14/sizeof(unsigned)] >> 8) & 0xFF;

	printf("ERROR:");
	switch (status) {
	case 0x20:
		printf("Slave Address + W has been transmitted, and Slave's Not-Acknowledge(NACK) has been received.\n");
		break;
	case 0x30:
		printf("Data byte in WR_DATA_REG has been transmitted, and NACK has been received.\n");
		break;
	case 0x48:
		printf("Slave Address + R has been transmitted, and NACK has been received.\n");
		break;
	case 0x70:
		printf("Bus error, SDA stuck low.\n");
		break;
	case 0x90:
		printf("Bus error, SCL stuck low.\n");
		break;
	case 0xFF:
		printf("None\n");
		break;
	default:
		printf("Unknown error %X\n", status);
		break;
	}
}

int writeDebug = 1;

void sendData(unsigned adrs, unsigned data) {

	writeDebug = 0;

	//reg[0x14/sizeof(unsigned)] = 3; // clear any existing errors // DECO-------------------------


	//
	// Valeur initiale: 10000
	//
	// 10000: Attente de l'ordre de la milliseconde
	// 10: Attente de l'ordre de la us
	// 240*10: Attente de l'ordre de 240us, le temps nécessaire à la transmission
	int limit = 5000; // 5000 est donc une valeur large


	int len = 1;

	clrbit(&reg[0], 5); // only write
	setbit(&reg[0], 4);
	reg[0] = (reg[0] & 0xFFFFFFF3) | (((len-1) & 3)<<2); // write data length // -2147483632 
	reg[0xC/sizeof(unsigned)] = data; // reg[4]
	reg[0x8/sizeof(unsigned)] = adrs; // reg[3]
	setbit(&reg[0], 6); // start

	while (!getbit(&reg[0x14/sizeof(unsigned)], 1) && --limit>0)
		; // wait done

	if (limit <=0) {

		setbit(&reg[0x14/sizeof(unsigned)], 1); // Set the state to "sent"

		if (writeDebug) {
			printf("-");
			fflush(stdout);
		}
		//return;
	} else if (writeDebug) {
		printf("+");
		fflush(stdout);
	}

	if (writeDebug && getbit(&reg[0x14/sizeof(unsigned)], 0)) {
		print_TWI_error();
	}
}

unsigned read_TWI_data(unsigned adrs, int len) {
	int limit=10000;
	reg[0] = (reg[0] & 0xFFFFFFFC) | ((len - 1) & 3); // read data length
	clrbit(&reg[0], 5); // only read
	clrbit(&reg[0], 4);
	setbit(&reg[0], 6); // start
	while (!getbit(&reg[0x14/sizeof(unsigned)], 1) && --limit>0)
		; // wait done
	if (limit <=0) {
		printf("timeout\n");
		exit(3);
	}
	if (getbit(&reg[0x14/sizeof(unsigned)], 0)) {
		print_TWI_error();
		//exit(1);
	}
	return reg[0x10/sizeof(unsigned)];
}

//--------------------------------------------------------------------------
//
//                        Fonctions de haut niveau
//
//--------------------------------------------------------------------------


void OLD_Controlleur_Initialise(int testMoteurs) {
	estInitialiseControlleur = 0;
	//echo 100000 > /proc/sys/dev/i2c/str8100_clock

	cycle = 0;
	printf("Ouverture port I2C...");

	for (i = 0; i < 4; i++) {
		lastSend[i] = 0;
	}

	//printf("\nEcriture 100000 sur /proc/sys/dev/i2c/str8100_clock...");
	FILE *f=fopen("/proc/sys/dev/i2c/str8100_clock", "w");
	fprintf(f, "100000\n");
	fclose(f);

	//printf("\nOuverture device I2C: /dev/i2c-0...");
	fdc = open("/dev/i2c-0", O_RDWR);
	//printf("\n fdc=%i", fdc);

	if (1) {
		//printf("\nEcriture sur le device...");
		write(fdc, &fdc, sizeof(fdc)); // Force la mise a jour de la fréquence horloge I2C
		// Rq: l'écriture renvoie une erreur, mais force qd même la MAJ !

		//printf("\nFermeture du device...");
		close(fdc);

		usleep(100 * 1000);

		init_TWI();

		printf("\nAttente...");
		fflush(stdout);
		usleep(500 * 1000);

		printf("\nEnvoi signal 1");
		fflush(stdout);
		sendData(0x58, 1);
	}

	printf(" OK\n");

	/*
	 if(detect_LM73) {
	 printf("I2C Fonctionnel, lecture température...\n");
	 // Read temp
	 LM73_power(0); 
	 usleep(150000);
	 LM73_power(1); 
	 usleep(150000);
	 // 0 = 0C
	 // 1/128 degree per tick
	 short temp = read_LM73_temp();
	 printf("Température: %.2fC\n", (float)temp / 128);
	 } else {
	 printf("ERREUR I2C: Le capteur de température ne répond pas\n");
	 return;
	 }
	 */

	int NB_ITERATIONS = 50; // 100*20ms = 2s
	int WAIT_MS = 20000; // 20ms
	unsigned TEST_SPEED = 0x5; // (5/255) %
	writeDebug = 1;

	if (testMoteurs) {
		printf("Rotation moteur 1...");
		fflush(stdout);
		for (i = 0; i < NB_ITERATIONS; i++) {
			sendData(0x52, TEST_SPEED);
			usleep(WAIT_MS);
		}
		printf(" OK\n");

		printf("Rotation moteur 2...");
		fflush(stdout);
		for (i = 0; i < NB_ITERATIONS; i++) {
			sendData(0x54, TEST_SPEED);
			usleep(WAIT_MS);
		}
		printf(" OK\n");

		printf("Rotation moteur 3...");
		fflush(stdout);
		for (i = 0; i < NB_ITERATIONS; i++) {
			sendData(0x56, TEST_SPEED);
			usleep(WAIT_MS);
		}
		printf(" OK\n");

		printf("Rotation moteur 4...");
		fflush(stdout);
		for (i = 0; i < NB_ITERATIONS; i++) {
			sendData(0x58, TEST_SPEED);
			usleep(WAIT_MS);
		}
		printf(" OK\n");

		printf("Si tout les moteurs ont tournés, appuyer sur entrée.\n");
		fflush(stdout);
		Misc_WaitEnter();
	}

	writeDebug = 0;

	/*
	 printf("Ecriture sur 0x60 (Rien)...");
	 write_TWI_data(0x60, 1, 0x0);
	 printf(" FIN\n");
	 */

	estInitialiseControlleur = 1;
}

void OLD_Controlleur_Envoi(int mode) {

	if (modeI2C == 0) {
		// On envoie rien

	} else if (1 || modeI2C == 1) {

		// Alternance par moteur
		if (1 || cycle == 0) { // Envoi sur mot 1 en permanence
			sendData(0x52, commandeNumerique[0]); // T1
		} else if (cycle == 1) {
			sendData(0x56, commandeNumerique[2]); // R1
		} else if (cycle == 2) {
			sendData(0x54, commandeNumerique[1]); // T2
		} else {
			sendData(0x58, commandeNumerique[3]); // R2
		}

	} else if (modeI2C == 2) {

		// Alternance moteurs roulis/tangage
		if (cycle <= 1) {
			sendData(0x52, commandeNumerique[0]); // T1
			sendData(0x56, commandeNumerique[2]); // R1
		} else {
			sendData(0x54, commandeNumerique[1]); // T2
			sendData(0x58, commandeNumerique[3]); // R2
		}
	} else if (modeI2C == 3) {

		// Tout les moteurs
		if (commandeNumerique[0] != derCommandeNumerique[0] || lastSend[0]
				> MAX_WAIT_BEF_SEND) {
			derCommandeNumerique[0] = commandeNumerique[0];
			sendData(0x52, commandeNumerique[0]); // T1
			lastSend[0] = 0;
		} else {
			lastSend[0]++;
		}

		if (commandeNumerique[1] != derCommandeNumerique[1] || lastSend[1]
				> MAX_WAIT_BEF_SEND) {
			derCommandeNumerique[1] = commandeNumerique[1];
			sendData(0x54, commandeNumerique[1]); // T2
			lastSend[1] = 0;
		} else {
			lastSend[1]++;
		}

		if (commandeNumerique[2] != derCommandeNumerique[2] || lastSend[2]
				> MAX_WAIT_BEF_SEND) {
			derCommandeNumerique[2] = commandeNumerique[2];
			sendData(0x56, commandeNumerique[2]); // R1
			lastSend[2] = 0;
		} else {
			lastSend[2]++;
		}

		if (commandeNumerique[3] != derCommandeNumerique[3] || lastSend[3]
				> MAX_WAIT_BEF_SEND) {
			derCommandeNumerique[3] = commandeNumerique[3];
			sendData(0x58, commandeNumerique[3]); // R2
			lastSend[3] = 0;
		} else {
			lastSend[3]++;
		}
	}

	// Moteurs 1 et 2: tangage
	// Moteurs 3 et 4: roulis


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

//--------- Not used stuff


/*
 void write_TWI_data(unsigned adrs, int len, unsigned data) {

 //reg[0x14/sizeof(unsigned)] = 3; // reg[5] // clear any existing errors

 int limit=10000;
 clrbit(&reg[0], 5); // only write
 setbit(&reg[0], 4);
 reg[0] = (reg[0] & 0xFFFFFFF3) | (((len - 1) & 3)<<2); // write data length 
 reg[0xC/sizeof(unsigned)] = data; // reg[4]
 reg[0x8/sizeof(unsigned)] = adrs; // reg[3]
 setbit(&reg[0], 6); // start
 while (!getbit(&reg[0x14/sizeof(unsigned)], 1) && --limit>0)
 ; // wait done
 //printf("write %d,%X -> %X\n",len,data,reg[0x14/sizeof(unsigned)]);
 if (limit <=0) {
 //printf("timeout\n");
 } else if (getbit(&reg[0x14/sizeof(unsigned)], 0)) {
 print_TWI_error();
 }
 }



 int detect_LM73() {
 unsigned data;
 write_TWI_data(0x92, 1, 0x7);
 data = read_TWI_data(0x92, 2);
 if (((data & 0xFF) != 0x01) || (((data >> 8) & 0xFF) != 0x90)) {
 return 0;
 }
 return 1;
 }

 short read_LM73_temp() {
 unsigned data;

 //	 // uncomment if we want to wait for temp by polling
 //	 int limit = 10;
 //	 write_TWI_data(0x92,1,0x4);
 //	 data = read_TWI_data(0x92,1);
 //	 while (!(data & 1) && --limit > 0) {
 //	 usleep(10000);
 //	 data = read_TWI_data(taddr,1);
 //	 }
 

 write_TWI_data(0x92, 1, 0x0);
 data = read_TWI_data(0x92, 2);
 return ((data & 0xFF) << 8) + ((data >> 8) & 0xFF);
 }

 void LM73_power(int on) {
 write_TWI_data(0x92, 2, 0x4001 | (((on==0) ? 1 : 0) << 15));
 }


 */

