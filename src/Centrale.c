#include "Centrale.h"
#include "Params.h"
#include "Maths.h"
#include "Misc.h"
#include <errno.h> // gestion minimale des erreurs
#include <fcntl.h> // Pour le flag NON_BLOCK
#include <stdio.h>
#include <math.h>
#include <stdint.h> //pour size_t


typedef unsigned int size_t;

void readFrame();
char *strerror(int errnum);
void *memcpy(void *dest, const void *src, size_t n); //avant int était size_t
void close(int id);
int system(const char *string);
float getFloatAtOffset(int offset);
int getInt32AtOffset(int offset);
void usleep(int ns);
int read(int id, unsigned char* buf, int length);

//
// Variables statiques de la centrale
//
unsigned char bufferCircuCentrale[1024]; // >= CENTRALE_TAILLE_BUFFER
unsigned char bufferPortCentrale[1024];  // > 3 * CENTRALE_TAILLE_TRAME
int fdCentrale;
int indexLectureCentrale;
int indexEcritureCentrale;
int estInitialiseCentrale;

//
// Variables prédéclarés pour éviter les temps d'allocation, elles sont initialisé avant chaque utilisation
//
int j;
double tempDouble;
char tempEightBytes[8];
int ret;
int donneesDisponibles;


float tempsPic;
float ancTempsPic;
float ancLRT[3]; // Anciens angles pour obtention de la vitesse par dérivation
float ancVitLRT[3];
float insVitLRT[3];
float zeroPosLRT[3];


//float PosGPS[3];
//float zeroPosGPS[3];
//float TempsGPS;

//--Baro
 int indexHistBaro;
 float *histAltBaro;
 float lastBaro;
 float zeroBaro;
 float AltitudeBaro;
 








void Centrale_Initialise() {
	char* adresse="/dev/ttyO0";
	printf("Création serveur centrale...\n");
	//ATTENTION S'ASSURER AU BOOT QUE TOUT EST BIEN CONFIGURER AVEC STTY
	//stty 460800 cstopb</dev/ttyO0 bauds
	if(adresse==NULL){
		printf("Veuillez fournir l'adresse de la centrale en argument");
		return;
	}
	//On choisit l'adresse qu'on hardcodera une fois détérminé

	printf("Device centrale: %s\n", adresse);
	printf("Ouverture port centrale...");
	fdCentrale = open(adresse, O_RDONLY );//| O_NONBLOCK | O_NOCTTY); // O_RDWR // En vision datation il faudra la rendre bloquante!
	if (fdCentrale != NULL){
		printf(" OK\n");
	}
	else{
		printf("Erreur lors de l'ouverture du port centrale");
	}
	indexEcritureCentrale = 0;
	indexLectureCentrale = 0;
	donneesDisponibles = 0;
	freqCentrale = 0;
	recherchesCentrale = 0;
	erreursCentrale = 0;
	estInitialiseCentrale = 1;
	//BARO
	indexHistBaro=0;
	zeroBaro=0;
	for(i = 0; i < 3; i++) {
		ancVitLRT[i] = 0;
	}
}

void Centrale_Termine() {
	if (estInitialiseCentrale) {
		printf("Fermeture port centrale...");
		close(fdCentrale);
		printf(" OK\n");
		estInitialiseCentrale = 0;
	}
}

void Centrale_RAZAttitude() {
	for (i = 0; i < 3; i++) {
		zeroPosLRT[i] = PosLRTA[i] + zeroPosLRT[i];
	}
}

/*
void Centrale_RAZPosition() {
	for (i = 0; i < 3; i++) {
		zeroPosGPS[i] = PosGPS[i] + zeroPosGPS[i];
	}
}
*/

void RAZBaro() {
	zeroBaro = AltitudeBaro + zeroBaro;
}

//
// Lit les données, et si une trame est disponible l'utilise et met "NouvelleTrameCentrale" é 1.
//
void Centrale_CheckData(int useData) {
	
	int debug = 0;

	if (!estInitialiseCentrale) {
		return;
	}

	if (!Misc_HasData(fdCentrale)) {
		if(debug)printf("w");		
		return;
	} 

	ret = read(fdCentrale, bufferPortCentrale, BUF_CTR * CENTRALE_TAILLE_TRAME);
	if (!useData) {
		return;
	}
		
	if (ret<0 && ret != EAGAIN && errno != 11) { // errno==11 correspond a "ressource indisponible" = pas de données !
		//
		// ERREUR GRAVE - Gestion é faire
		//
		printf("\nERREUR LECTURE CENTRALE: %s (errno=%i)\n", strerror(errno),
				errno);
		erreursCentrale++;
		return;
		//abort();
	} else if (ret==EAGAIN) {
		//
		// Pas de données, pas grave on retourne
		//
		if(debug)printf("_NODATA_");

	} else {
		if (debug) {
			if (0) { //ret == maxBytesRead) {
				printf("_OVERFLOW_");
			} else if (ret > CENTRALE_TAILLE_TRAME) {
				printf("+%i ", ret - CENTRALE_TAILLE_TRAME);
				//	printf("\nCENTRALE: Il reste des données !\n");
				//	Misc_SetRedLed(1);
			} else if (ret == CENTRALE_TAILLE_TRAME) {
				
				printf("=");
				int ii = 0;
				if (bufferPortCentrale[ii] == 255 // 0x255 = frame header
						&& bufferPortCentrale[(ii + 1) % CENTRALE_TAILLE_BUFFER] == 1 // 0x1 = frame type
						&& bufferPortCentrale[(ii + CENTRALE_TAILLE_TRAME) % CENTRALE_TAILLE_BUFFER]
								== 255 // next frame header
						&& bufferPortCentrale[(ii + CENTRALE_TAILLE_TRAME + 1) % CENTRALE_TAILLE_BUFFER]
								== 1 // Next frame type
				) {
					
				}
			} else if (ret < CENTRALE_TAILLE_TRAME) {
				printf("-%i ", CENTRALE_TAILLE_TRAME - ret);
			}
		}
		
		
		//
		// On a des données
		//
		//--Etape 1: Recopions les dans bufferCircuCentrale
		if(ret > 0) {
				for (i = 0; i < ret; i++) {
					bufferCircuCentrale[indexEcritureCentrale] = bufferPortCentrale[i];
					indexEcritureCentrale++;
					if (indexEcritureCentrale == CENTRALE_TAILLE_BUFFER)
					indexEcritureCentrale = 0;
				}
		}
		//--Etape 2: Calculons le nombre de données disponibles
		donneesDisponibles = indexEcritureCentrale - indexLectureCentrale;
		if (donneesDisponibles < 0)
			donneesDisponibles += CENTRALE_TAILLE_BUFFER;

		//--Etape 3: Recherchons la trame
		while (donneesDisponibles >= CENTRALE_TAILLE_TRAME) {

			if (bufferCircuCentrale[indexLectureCentrale] == 255 // 0x255 = frame header
					&& bufferCircuCentrale[(indexLectureCentrale + 1) % CENTRALE_TAILLE_BUFFER] == 1 // 0x1 = frame type
					&& bufferCircuCentrale[(indexLectureCentrale + CENTRALE_TAILLE_TRAME) % CENTRALE_TAILLE_BUFFER]
							== 255 // next frame header
					&& bufferCircuCentrale[(indexLectureCentrale + CENTRALE_TAILLE_TRAME + 1) % CENTRALE_TAILLE_BUFFER]
							== 1 // Next frame type
			) {
				
				//--Début d'une trame
				readFrame();
				freqCentrale++;
				indexLectureCentrale += CENTRALE_TAILLE_TRAME; // Avanéons l'index d'une trame	
				
				if(debug)printf("T");
			} else {
				//--Pas le début d'une trame
				indexLectureCentrale++; // Avanéons l'index d'un cran pour trouver le début
				recherchesCentrale++;
				
			}

			//
			// Mise a jour des index
			//
			if (indexLectureCentrale >= CENTRALE_TAILLE_BUFFER) {
				indexLectureCentrale -= CENTRALE_TAILLE_BUFFER;
			}

			if (0 && NouvelleTrameCentrale) { // La présence du zéro force a aller jusqu'au bout
											  // Et donc de prendre la derniere trame si on récupére 2 d'un coup

				// Arretons ici la boucle
				donneesDisponibles = - 1;

			} else {
				// Recalculons le nombre de données disponibles
				donneesDisponibles = indexEcritureCentrale
						- indexLectureCentrale;
				if (donneesDisponibles < 0) {
					donneesDisponibles += CENTRALE_TAILLE_BUFFER;
				}
			}

		} // WHILE il y a de quoi faire une trame
	} // If il y avait des données
} // Fin de la fonction


void readFrame() {

	if (NouvelleTrameCentrale) {
		tramesEffacees += 1;
		IntervalleTemps = 2.0F / 102.0F;
	} else {
		IntervalleTemps = 1.0F / 102.0F;
	}

	// 
	// Checksum: TESTE, FONCTIONNE
	//
	long val1[CENTRALE_TAILLE_TRAME - 4];
	long val2[CENTRALE_TAILLE_TRAME - 4];
	val1[0] = bufferCircuCentrale[(indexLectureCentrale + 2) % CENTRALE_TAILLE_BUFFER];
	val2[0] = bufferCircuCentrale[(indexLectureCentrale + 2) % CENTRALE_TAILLE_BUFFER];
	for (i = 2; i < CENTRALE_TAILLE_TRAME - 3; i++) // Tout sauf deux premier (255, 1) et deux derniers bytes (les 2 checksums)
	{
		val1[i - 1] = val1[i - 2] + bufferCircuCentrale[(indexLectureCentrale + 1 + i) % CENTRALE_TAILLE_BUFFER];
		val2[i - 1] = val2[i - 2] + val1[i - 1];
	}
	char check1 =(char) (val1[CENTRALE_TAILLE_TRAME - 5] % 256L);
	char check2 =(char) (val2[CENTRALE_TAILLE_TRAME - 5] % 256L);
	char cible1 = bufferCircuCentrale[(indexLectureCentrale + CENTRALE_TAILLE_TRAME - 2) % CENTRALE_TAILLE_BUFFER];
	char cible2 = bufferCircuCentrale[(indexLectureCentrale + CENTRALE_TAILLE_TRAME - 1) % CENTRALE_TAILLE_BUFFER];
	int passesCheckSum = 0;
	if (check1 == cible1 && check2 == cible2) {
		passesCheckSum = 1;
	}

	if (!passesCheckSum) {
		printf("\nWARNING: CENTRALE: check sum raté !");
		erreursCentrale++;
		return;
	}

	//
	//--Date temps PIC et calcul de l'intervalle de temps
	//
	if(0) {
		tempsPic = getFloatAtOffset(2);
		IntervalleTemps = tempsPic - ancTempsPic;
		ancTempsPic = tempsPic;
	}
	

	//
	// Vérifions si des trames ont été ratées
	//
	if (0 && IntervalleTemps > 1.5F / 102.0F) { // Intervalle théorique: 1/102
		
		//--Oui, déterminons le nombre de trames ratées
		int COMPTAGE_MAXI = 50;
		int nbTramesRatees;
		for(nbTramesRatees = 1; nbTramesRatees < COMPTAGE_MAXI; nbTramesRatees++) {
			if(IntervalleTemps < ((float)nbTramesRatees + 1.5F) / 102.0F) {
				break;
			}
		}
		
		if (1) { // Affiche les warnings pour manque de trames ?
			if (nbTramesRatees >= 2 && nbTramesRatees != COMPTAGE_MAXI - 1) {
				printf("\nWARNING: CENTRALE: Raté %i trames d'un coup !",
						nbTramesRatees);
			} else if (nbTramesRatees == COMPTAGE_MAXI) {
				printf("\nWARNING: CENTRALE: Raté plus de %i trames !",
						COMPTAGE_MAXI - 1);
			}
		}
		
		//printf("\nWARNING: CENTRALE: Intervalle entre trames trop long  : dt=%fms\n", 1000 * IntervalleTemps);
		erreursCentrale += nbTramesRatees;
		trameRate = 1; // Si oui ou non il y a eu des trames ratés ce tour ci
	} else {
		trameRate = 0;
	}
	
	if (0) {
		if (IntervalleTemps > 1.5F / 102.0F) { // Raté plus de deux trames
			IntervalleTemps = 2.0F / 102.0F;
		} else {
			IntervalleTemps = 1.0F / 102.0F; //-----------------------Assez débile comme méthode !
		}
	}

	//
	//--Angles (L,R,T)
	//
	PosLRTA[0] = -getFloatAtOffset(82);
	PosLRTA[2] = -getFloatAtOffset(82 + 8);
	PosLRTA[1] = +getFloatAtOffset(82 + 16);

	for (i = 0; i < 3; i++) {
		//--Recentrage
		PosLRTA[i] -= zeroPosLRT[i];
		
		//--Vérfication ordre de grandeur
		if (PosLRTA[i] > 3.0F * PI || PosLRTA[i] < -3.0F * PI) {
			erreursCentrale += 1;
			printf("\nERREUR: CENTRALE: Angles hors de porté!\n");
			return;
		}
		
		//--Application du modulo choisi
		if (PosLRTA[i] > PI)
			PosLRTA[i] -= DEUX_PI;
		if (PosLRTA[i] <= -PI)
			PosLRTA[i] += DEUX_PI;

		//--Obtention de la vitesse par dérivation
		
		//---Vitesse actuelle
		if(0) { // Utiliser vitese instantanée
			VitLRTA[i] = 0.5F * (PosLRTA[i] - ancLRT[i]) / IntervalleTemps;
		} else {
			ancVitLRT[i] = insVitLRT[i];
			insVitLRT[i] = (PosLRTA[i] - ancLRT[i]) / IntervalleTemps;
			VitLRTA[i] = 0.5F * (ancVitLRT[i] + insVitLRT[i]);
		}
		ancLRT[i] = PosLRTA[i];
	}

	//
	// Accéléros
	//
	/*
	 for (i = 0; i < 3; i++) {
	 Acc[i] = getFloatAtOffset(34 + i * 8) - CorAcc[i];
	 Vit[i] += IntervalleTemps * Acc[i];
	 Pos[i] += IntervalleTemps * Vit[i];
	 }*/

	//
	//--Baro
	//
	/*
	 long tempLong = 265L * bufferCircuCentrale[(indexLectureCentrale + 162) % CENTRALE_TAILLE_BUFFER];
	 tempLong += bufferCircuCentrale[(indexLectureCentrale + 162 + 1) % CENTRALE_TAILLE_BUFFER];
	 histAltBaro[indexHistBaro] = 44330.7692308 * (1 - pow(((((float)tempLong) / 10) / 1013.25), 0.1901976));
	 lastBaro = histAltBaro[indexHistBaro];
	 indexHistBaro++;
	 if (indexHistBaro == DUREE_MOYENNAGE_BARO)
	 indexHistBaro = 0;

	 AltitudeBaro = 0.0;
	 for ( i = 0; i < DUREE_MOYENNAGE_BARO; i++) {
	 AltitudeBaro += histAltBaro[i];
	 }
	 AltitudeBaro /= (float)DUREE_MOYENNAGE_BARO;
	 AltitudeBaro -= zeroBaro;
	 */

	//if (Params.GET_GPS_VALUES) {
	/*

	 //
	 //--Position et vitesse GPS
	 //
	 PosGPS[0] = (float)getInt32AtOffset(164);
	 PosGPS[1] = (float)getInt32AtOffset(164 + 4);
	 PosGPS[2] = (float)getInt32AtOffset(164 + 8);

	 VitGPS[0] = (float)getInt32AtOffset(164 + 12);
	 VitGPS[1] = (float)getInt32AtOffset(164 + 16);
	 VitGPS[2] = (float)getInt32AtOffset(164 + 20);

	 for ( i = 0; i < 3; i++) {
	 PosGPS[i] *= COEF_GPS_POS;
	 PosGPS[i] -= zeroPosGPS[i];
	 VitGPS[i] *= COEF_GPS_VIT;
	 }

	 //
	 //--Temps GPS
	 //
	 tempLong = 0;
	 for ( i = 0; i < 4; i++) {
	 tempLong
	 += pow256[i]
	 * bufferCircuCentrale[(indexLectureCentrale + 164 + 24 + i) % CENTRALE_CIRCULAR_BUFFER_SIZE];
	 }
	 TempsGPS = (float)tempLong;
	 }*/

	//
	// La trame est dispo !
	//
	NouvelleTrameCentrale = 1;
}


float getFloatAtOffset(int offset) {
	//
	// Optimisation: rajouter au pointeur dans le memcpy plutot que de copier des données !!!!

	// 2 difficultés a traiter ici:
	// -> Il faut placer les 4 derniers octets en premiers en gardant leur ordre et sans inverser les bits !
	// -> On est dans un buffer circulaire, attention aux indices trop grand..

	for (i = 0; i < 4; i++) {
		//tempEightBytes[i] = bufferCircuCentrale[(indexLectureCentrale + offset + i + 4) % CENTRALE_TAILLE_BUFFER];
		//tempEightBytes[i+4] = bufferCircuCentrale[(indexLectureCentrale + offset + i) % CENTRALE_TAILLE_BUFFER];
		//devient donc (David
		tempEightBytes[i] = bufferCircuCentrale[(indexLectureCentrale + offset + i) % CENTRALE_TAILLE_BUFFER];
		tempEightBytes[i+4] = bufferCircuCentrale[(indexLectureCentrale + offset + i + 4) % CENTRALE_TAILLE_BUFFER];
	}
	memcpy((void*)&tempDouble, (void*)tempEightBytes, 8);
	return ((float)tempDouble);
	/*
	 if (indexLectureCentrale + offset + 3 < CENTRALE_TAILLE_BUFFER) {
	 //--Les 4 premiers octets sont alignés
	 memcpy((void*)(&result + 4), (void*)(&bufferCircuCentrale + indexLectureCentrale
	 + offset), 4);

	 } else {
	 // Si on n'est pas dans un bloc adjacent du buffer, on applique un modulo pour créer un tableau
	 char tempFourBytes[4];
	 for (; i < 4; i++) {
	 tempFourBytes[i] = bufferCircuCentrale[(indexLectureCentrale + offset + i) % CENTRALE_TAILLE_BUFFER];
	 }
	 memcpy((void*)(&result + 4), (void*)(&tempFourBytes), 4);
	 }

	 if (indexLectureCentrale + offset + 7 < CENTRALE_TAILLE_BUFFER) {
	 // Les 4 derniers octets sont alignés
	 memcpy((void*)(&result), (void*)(&bufferCircuCentrale + indexLectureCentrale + offset
	 + 4), 4);
	 } else {// Si on n'est pas dans un bloc adjacent du buffer, on applique un modulo pour créer un tableau
	 char tempFourBytes[4];
	 for (i = 0; i < 4; i++) {
	 tempFourBytes[i] = bufferCircuCentrale[(indexLectureCentrale + offset + i + 4) % CENTRALE_TAILLE_BUFFER];
	 }
	 memcpy((void*)(&result), (void*)(&tempFourBytes), 4);
	 }
	 */

	/*
	 for(i = 0; i < 4; i++) {
	 char temp = bytes[i + 4];
	 bytes[i + 4] = bytes[i];
	 bytes[i] = temp;
	 }
	 */

	//double result;
	//memcpy((void*)&result, (void*)bytes, sizeof(bytes));
}
