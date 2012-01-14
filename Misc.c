#include <C:\CSD\workspace\helloworld-oabi\src\sbus.h>
#include <C:\CSD\workspace\helloworld-oabi\src\Misc.h>
#include <stdio.h>
#include <fcntl.h> // to add non blocking flag to read input
#include <sys/time.h>


int atoi(const char * str);
float atof(const char * str);

struct timeval tv;
fd_set fds;

int lastUsTimer;
int lastUsElapsed;
int lastUsElapsed2;
int lastUsElapsed3;
int timer;

int us, dt; // Variables prédéclarés pour éviter l'allocation uniquement

void Misc_Initialise() {
	tv.tv_sec = 0;
	tv.tv_usec = 0;
}


void Misc_ResetTimer() {
	
	gettimeofday(&tv, NULL);
	lastUsTimer = tv.tv_usec;
	timer = 0;
}

void Misc_ResetElapsedUs() {
	
	gettimeofday(&tv, NULL);
	lastUsElapsed = tv.tv_usec;
	lastUsElapsed2 = tv.tv_usec;
}

void Misc_ResetElapsedUs2() {
	
	gettimeofday(&tv, NULL);
	lastUsElapsed2 = tv.tv_usec;
}

void Misc_ResetElapsedUs3() {
	
	gettimeofday(&tv, NULL);
	lastUsElapsed3 = tv.tv_usec;
}

float Misc_Abs(float value) {
	if(value < 0) {
		return -value;
	}
	return value;
}

int UMisc_GetTimerUs() {

	gettimeofday(&tv, NULL);
	us = tv.tv_usec; // Renvoie les us de la seconde en cours (retombe donc à zéro toutes les secondes !
	dt = us - lastUsTimer;
	lastUsTimer = us;
	if (dt < 0) {
		dt += 1000000; // Une seconde
	}
	timer += dt;
	return timer;
}

int UMisc_GetElapsedUs() {
	
	gettimeofday(&tv, NULL);
	us = tv.tv_usec; // Renvoie les us de la seconde en cours (retombe donc à zéro toutes les secondes !
	dt = us - lastUsElapsed;
		
		if (-200 * 1000 < dt && dt < 0) {
			//--Valeur fausse
			//---On recommence
			printf("E");
			gettimeofday(&tv, NULL);
			us = tv.tv_usec; // Renvoie les us de la seconde en cours (retombe donc à zéro toutes les secondes !
			dt = us - lastUsElapsed;

			
			
		} else {
			//lastUsElapsed = us;
		}
		lastUsElapsed = us;
		if (dt < 0) {
			dt += 1000000;
		}
		return dt;
}

int UMisc_GetElapsedUs2() {
	
	gettimeofday(&tv, NULL);
	us = tv.tv_usec; // Renvoie les us de la seconde en cours (retombe donc à zéro toutes les secondes !
	dt = us - lastUsElapsed2;
	lastUsElapsed2 = us;
	
	if (-500 * 1000 < dt && dt < 0) {
		//--Valeur fausse
		//---On recommence
		printf("E");
		gettimeofday(&tv, NULL);
		us = tv.tv_usec; // Renvoie les us de la seconde en cours (retombe donc à zéro toutes les secondes !
		dt = us - lastUsElapsed2;
	}
	if (dt < 0) {
		dt += 1000000;
	}
	return dt;
}

int UMisc_GetElapsedUs3() {
	
	gettimeofday(&tv, NULL);
	us = tv.tv_usec; // Renvoie les us de la seconde en cours (retombe donc à zéro toutes les secondes !
	dt = us - lastUsElapsed3;
	lastUsElapsed3 = us;
	if (dt < 0) {
		dt += 1000000;
	}
	return dt;
}



void Misc_SetGrnLed(int state) {
	if (state) {
		// Green led on
		sbuslock();
		sbus_poke16(0x62, sbus_peek16(0x62) | 0x8000);
		sbusunlock();
	} else {
		// Green led off
		sbuslock();
		sbus_poke16(0x62, sbus_peek16(0x62) & ~(0x8000));
		sbusunlock();
	}
}

void Misc_SetRedLed(int state) {
	if (state) {
		// Red led on
		sbuslock();
		sbus_poke16(0x62, sbus_peek16(0x62) | 0x4000);
		sbusunlock();
	} else {
		// Red led off
		sbuslock();
		sbus_poke16(0x62, sbus_peek16(0x62) & ~(0x4000));
		sbusunlock();
	}
}

void Misc_TglGrnLed()
{
	sbuslock();
	sbus_poke16(0x62, sbus_peek16(0x62) ^ 0x8000); // Tgl green led
	sbusunlock();
}

void Misc_TglRedLed()
{
	sbuslock();
	sbus_poke16(0x62, sbus_peek16(0x62) ^ 0x4000); // Tgl red led
	sbusunlock();
}


int askIntValue(const char* theValueName) {
	printf("GET: %s = ", theValueName);
	fflush(stdout);

	char *cptr;
	char buffer[256];
	for (;;) {
		cptr = fgets(buffer, 256, stdin);
		if (cptr != NULL) {
			int val = atoi(cptr);
			printf("SET: %s = %i\n", theValueName, val);
			fflush(stdout);
			return val;
		}
	}
}

float askFloatValue(const char* theValueName) {
	printf("GET: %s = ", theValueName);
	fflush(stdout);

	char *cptr;
	char buffer[256];
	for (;;) {
		cptr = fgets(buffer, 256, stdin);
		if (cptr != NULL) {
			float val = atof(cptr);
			printf("SET: %s = %f\n", theValueName, val);
			fflush(stdout);
			return val;
		}
	}
}

void Misc_WaitEnter() {
	printf("Appuyer sur entrée pour poursuivre...\n");
	fflush(stdout);

	char *cptr;
	char buffer[256];
	for (;;) {
		cptr = fgets(buffer, 256, stdin);
		if (cptr != NULL) {
			return;
		}
	}
}

//
// Fonction kbhit()
//
// Retourne 1 si des données sont dispo sur le stream
// Retourne 0 sinon
//
// Permet d'utiliser la fonction read() en non bloquant !
//
int Misc_HasData(STDIN_FILENO)
{
	FD_ZERO(&fds);
	FD_SET(STDIN_FILENO, &fds);
	select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
	return FD_ISSET(STDIN_FILENO, &fds);
}

void Misc_PrintHelp() {
	printf("Commandes disponibles:\n");
	printf("  Gestion d'état:\n");
	printf("    w: Clear\n");
	printf("    x: Initialisation\n");
	printf("    c: Rotation\n");
	printf("    v: Décollage\n");
	printf("    b: Atterissage\n");
	printf("  Réglages manuels:\n");
	printf("    z/s: Tangage\n");
	printf("    q/d: Roulis\n");
	printf("    a/e: Lacet\n");
	printf("    r/f: Altitude\n");
	printf("    p/m: Réglage de Vequi\n");
	printf("    $/*: Réglage de l'altitude après décollage\n");
	printf("  Divers:\n");
	printf("    y: Active/désactive le sonar\n");
	printf("    u: Buffer centrale\n");
	printf("    j: Mode I2C\n");
	printf("    n: Affichage\n");
	printf("    i: Infos fréquences\n");
	printf("    l: Gains PID\n");
	printf("    h: Affiche l'aide\n");
	printf("    k: Termine le programme\n");
	printf("   [espace]: Arret d'urgence\n");
	printf("   set {l/r/t/a}{p/i/d}: Réglage d'un gain\n");
	printf("Fin des commandes disponibles\n");
	fflush(stdout);
}

//---------------------------------------------------------------------------------
//
//               Code inutilisé
//
//
//

/*
 char invByte(char b)
 {
 char i = 0;
 char mask = 1;
 char inv = 0;
 for (; i < 7; i++) {
 if ((b & mask) != 0) {
 inv = (inv + 1);
 }
 inv = (inv << 1);
 mask = mask << 1;
 }
 if ((b & mask) != 0) {
 inv = (inv + 1);
 }
 return inv;
 }
 */

/*
 void getDoubleFromBytesDemo() {
 //
 // Démonstration de comment transformer un double en octets
 // puis a nouveau en double grace a memcpy et des pointeurs
 //
 double d = 1.0; // Le double de départ
 char bytes[8]; // Les 64 bits pour stocker le double

 memcpy((void*)bytes, (void*)&d, sizeof(bytes));
 
 int i, j;
 
 char invBytes[8]; // Inverse all bytes
 for(i = 0; i < 7; i++) {
 invBytes[7-i] = invByte(bytes[i]);
 }
 
 //--Affichage des 64 bits
 
 for (i = 0; i < 8; i++) {
 for (j = 0; j < 8; j++) {
 int k = 0;
 if ((bytes[i] & (1 << j)) != 0)
 k = 1;
 printf("%i", k);
 if(j == 7)printf(" ");
 }
 }
 
 printf("\n");
 for (i = 0; i < 8; i++) {
 for (j = 0; j < 8; j++) {
 int k = 0;
 if ((invBytes[i] & (1 << j)) != 0)
 k = 1;
 printf("%i", k);
 if(j == 7)printf(" ");
 }
 }
 
 
 

 double s;
 memcpy((void*)&s, (void*)bytes, sizeof(bytes));
 printf("\n%f\n", s);


 }
 */

