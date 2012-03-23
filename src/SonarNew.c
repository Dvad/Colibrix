#include "Params.h"
#include "Misc.h"
#include "SonarNew.h"
#include "i2C.h"
#include <errno.h> // gestion minimale des erreurs
#include <fcntl.h> // Pour le flag NON_BLOCK
#include <stdio.h>
#include <time.h>
int atoi(const char * str);
float atof(const char * str);
void close(int id);
int system(const char *string);
void usleep(int us);
char *strerror(int errnum);
int read(int id, unsigned char* buf, int length);
void lireTrameSonar();

//
// Variables statiques du sonar
//

int derniereAltitudeMoyenneEstValable;
int nbValeursValables;
int indexDerniereAltitude;
int fdSonar;
int indexEcritureSonar;
int indexLectureSonar;
int estInitialiseSonar;
unsigned char bufferCircuSonar[32]; // > SONAR_TAILLE_BUFFER
unsigned char bufferPortSonar[32]; // > SONAR_TAILLE_TRAME

float derniereAltitudeMoyenne;
float dernieresAltitudes[32]; // >= DUREE_MOYENNAGE_ALTITUDE
int LectureSonarEnCours;

struct timeval datationSonar;
long long int endTimeSonar=0;
long long int startTimeSonar=0;
//
// Variables prédéclarés pour éviter les temps d'allocation, elles sont initialisées avant chaque utilisation
//
int alti;
char valChar[4]; // Stocke temporairement la valeur sous forme char
int ret;
int donneesDisponibles;
int ancAlti;
void Sonar_Initialise() {

	indexEcritureSonar = 0;
	indexLectureSonar = 0;
	freqSonar = 0;

	valChar[3] = '\0';


	ancAlti = 0;

	nbValeursValables = 0;
	indexDerniereAltitude = 0;

	derniereAltitudeMoyenne = 0.0;
	derniereAltitudeMoyenneEstValable = 0;

	LectureSonarEnCours=0;
	I2C_Envoyer_Pulse_Sonar();

	estInitialiseSonar = 1;
}

//
//Vérifie que la mesure du sonar est fini, si ce n'est pas le cas attend le prochain coup
//Si c'est le cas, on extrait la valeur et on la moyenne pour remplir LRTA et VitLRTA
//Enfin, met la variable nouvelleTrameSonar à 1 si besoin
//
void lireTrameSonar() {

	if (NouvelleTrameSonar) {
		erreursSonar++;
	}
	gettimeofday(&datationSonar,NULL);
	startTimeSonar=datationSonar.tv_sec *1000000 + (datationSonar.tv_usec);
	ret=I2C_Envoyer_Lecture_Sonar();
	if (ret==-1) {
		return;
	}
	else {
		alti = ret; //alti est en centimètre pour le garder en int!
		if (DEBUG_SONAR==1)	printf("Altitude Instantanée %i:\n",alti);
	}
	gettimeofday(&datationSonar,NULL);
	endTimeSonar=datationSonar.tv_sec *1000000 + (datationSonar.tv_usec);
	//printf("\n Temps de lecture: %lli \n",endTimeSonar-startTimeSonar);
	// 
	// Saut maxi: MAX_SONAR_JUMP 
	//	
	if (alti > ancAlti + SONAR_SAUT_MAXI) {
		alti = ancAlti + SONAR_SAUT_MAXI;
	} else if (alti < ancAlti - SONAR_SAUT_MAXI) {
		alti = ancAlti - SONAR_SAUT_MAXI;
	}
	ancAlti = alti;

	SonarTropBas = 0;
	SonarTropHaut = 0;
	
	if (alti < 3) {
		// Le sonar est trop proche du sol
		SonarTropBas = 1;
		nbValeursValables = 0;
		VitLRTA[3] = 0.0;
		PosLRTA[3] = 0.0;
		derniereAltitudeMoyenneEstValable = 0;
		
	} else if (alti >= (int) RANGE_SONAR_EN_METRE * 100) {
		// Le sonar est trop loin du sol
		SonarTropHaut = 1;
		nbValeursValables = 0;
		VitLRTA[3] = 0.0;
		PosLRTA[3] = RANGE_SONAR_EN_METRE;
		derniereAltitudeMoyenneEstValable = 0;
		
	} else {
		// Le sonar est dans sa plage de fonctionnement
			
		//
		//
		//--Sytéme de moyenne sur l'altitude
		//
		nbValeursValables++;
		dernieresAltitudes[indexDerniereAltitude] = (float)alti * 0.01F;
		indexDerniereAltitude++;
		if (indexDerniereAltitude == DUREE_MOYENNAGE_ALTITUDE) {
			indexDerniereAltitude = 0;
		}

		if (nbValeursValables >= DUREE_MOYENNAGE_ALTITUDE) {
			// Obtention de l'altitude par moyennage
			PosLRTA[3] = 0.0;
			for (i = 0; i < DUREE_MOYENNAGE_ALTITUDE; i++) {
				PosLRTA[3] += dernieresAltitudes[i];
			}
			PosLRTA[3] /= (float)DUREE_MOYENNAGE_ALTITUDE;
			//PosLRTA[3] -= SONAR_ALTI_MINI; // pourquoi faire ca?
			if (DEBUG_SONAR==1) printf("Altitude filtrée: %f\n",PosLRTA[3]);
			if (derniereAltitudeMoyenneEstValable) {
				// Obtention de la vitesse par dérivation sur les 2 dernieres moyennes
				VitLRTA[3] = INVERSE_INTERVALLE_TEMPS_DERIVATION_SONAR //TODO Vérifier cette constante On SAIT qu'on a un résultat sonar toute les "tant" ms
						* (PosLRTA[3] - derniereAltitudeMoyenne);
			} else {
				VitLRTA[3] = 0.0F;
			}
			derniereAltitudeMoyenne = PosLRTA[3];
			derniereAltitudeMoyenneEstValable = 1;
			
			nbValeursValables = DUREE_MOYENNAGE_ALTITUDE; // Empeche le compteur d'exploser
		} else {
			
			// On a pas encore assez de valeurs valables, attendons...
			// On garde l'ancienne valeur de la position !! 
			//  (Sinon on remettrai a zéro en cas de trop haut !!)
			VitLRTA[3] = 0.0;
		}
	}
	//dateTrameSonar=datation.tv_sec *1e6 + (datation.tv_usec);
	I2C_Envoyer_Pulse_Sonar();
	NouvelleTrameSonar = 1;
}

