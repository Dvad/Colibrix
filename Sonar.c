#include <C:\CSD\workspace\helloworld-oabi\src\Params.h>
#include <C:\CSD\workspace\helloworld-oabi\src\Misc.h>
#include <C:\CSD\workspace\helloworld-oabi\src\Sonar.h>
#include <errno.h> // gestion minimale des erreurs
#include <fcntl.h> // Pour le flag NON_BLOCK
#include <stdio.h>

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


//
// Variables prédéclarés pour éviter les temps d'allocation, elles sont initialisé avant chaque utilisation
//
float alti;
char valChar[4]; // Stocke temporairement la valeur sous forme char
int ret;
int donneesDisponibles;
int ancAltiPouces;
int altiPouces;

void Sonar_Initialise(int dev) {

	printf("Création serveur sonar...\n");
	// /usr/local/sbin/
	
	
	if (SONAR_IRQ == 200) {
		system("xuartctl --port=1 --speed=9600 --server --irq=200hz");
	} else if (SONAR_IRQ == 300) {
		system("xuartctl --port=1 --speed=9600 --server --irq=300hz");
	} else if (SONAR_IRQ == 400) {
		system("xuartctl --port=1 --speed=9600 --server --irq=400hz");
	} else {
		system("xuartctl --port=1 --speed=9600 --server");
	}
	usleep(100000);

	if (dev == -1) { // Laisser l'utilisateur choisir
		dev = askIntValue("Numéro device ci dessus");
	}

	char* adresse;
	if (dev == 0) {
		adresse = "/dev/pts/0";
	} else if (dev == 1) {
		adresse = "/dev/pts/1";
	} else if (dev == 2) {
		adresse = "/dev/pts/2";
	} else if (dev == 3) {
		adresse = "/dev/pts/3";
	} else {
		printf("ERREUR GRAVE: DEVICE NON DANS LA LISTE");
		return;
	}

	printf("Device sonar: %i\n", dev);
	printf("Ouverture port sonar...");
	fdSonar = open(adresse, O_NONBLOCK); // O_RDWR |
	printf(" OK\n");
	fflush(stdout);

	indexEcritureSonar = 0;
	indexLectureSonar = 0;
	freqSonar = 0;

	valChar[3] = '\0';

	altiPouces = 0;
	ancAltiPouces = 0;

	nbValeursValables = 0;
	indexDerniereAltitude = 0;

	derniereAltitudeMoyenne = 0.0;
	derniereAltitudeMoyenneEstValable = 0;

	estInitialiseSonar = 1;
}

void Sonar_Termine() {
	if (estInitialiseSonar) {
		printf("Fermeture port sonar...");
		close(fdSonar);
		printf(" OK\n");
		estInitialiseSonar = 0;
	}
}

void Sonar_CheckData(int keepIt) {

	if (!estInitialiseSonar) {
		return;
	}

	if (!Misc_HasData(fdSonar)) {
		return;
	} // Pas optimal


	ret=read(fdSonar, bufferPortSonar, SONAR_TAILLE_TRAME);

	if (!keepIt) {
		return;
	}

	if (ret<0 && ret !=EAGAIN) {
		// errno==11 correspond a "ressource indisponible" = pas de données !
		//
		// ERREUR GRAVE - Gestion à faire
		//
		printf("\nERREUR LECTURE SONAR: %s (errno=%i)\n", strerror(errno),
				errno);

	} else if (ret==EAGAIN) {
		//
		// Pas de données, pas grave on retourne
		//

	} else {
		//
		// On a des données
		//
		//--Etape 1: Recopions les dans bufferCircuSonar
		for (i = 0; i < ret; i++) {
			bufferCircuSonar[indexEcritureSonar] = bufferPortSonar[i];

			indexEcritureSonar++;
			if (indexEcritureSonar == SONAR_TAILLE_BUFFER) {
				indexEcritureSonar = 0;
			}
		}
		//--Etape 2: Calculons le nombre de données disponibles
		donneesDisponibles = indexEcritureSonar - indexLectureSonar;
		if (donneesDisponibles < 0)
			donneesDisponibles += SONAR_TAILLE_BUFFER;

		//--Etape 3: Recherchons la trame
		while (donneesDisponibles >= SONAR_TAILLE_TRAME) {

			// La trame est de type 82, A, B, C, 13 
			// avec A,B,C l'écriture de la dist en ASCII(13 = "\n")
			if (bufferCircuSonar[indexLectureSonar] == 82) {
				// 82 ASCII == 'R'
				//--Début d'une trame
				lireTrameSonar();
				indexLectureSonar += SONAR_TAILLE_TRAME;
				freqSonar++;
			} else {
				//--Pas le début d'une trame
				indexLectureSonar++; // Avançons l'index d'un cran 
				recherchesSonar++; // pour trouver le début
			}

			//--Maj des index
			if (indexLectureSonar >= SONAR_TAILLE_BUFFER) {
				indexLectureSonar -= SONAR_TAILLE_BUFFER;
			}

			if (0 && NouvelleTrameSonar) {
				// Arretons ici la boucle
				donneesDisponibles = - 1;
			} else {
				// Recalculs le nombre de données disponibles
				donneesDisponibles = indexEcritureSonar - indexLectureSonar;
				if (donneesDisponibles < 0) {
					donneesDisponibles += SONAR_TAILLE_BUFFER;
				}
			}
		}
	}
}

void lireTrameSonar() {

	if (NouvelleTrameSonar) {
		erreursSonar++;
	}

	valChar[0] = bufferCircuSonar[(indexLectureSonar + 1) % SONAR_TAILLE_BUFFER];
	valChar[1] = bufferCircuSonar[(indexLectureSonar + 2) % SONAR_TAILLE_BUFFER];
	valChar[2] = bufferCircuSonar[(indexLectureSonar + 3) % SONAR_TAILLE_BUFFER];
	altiPouces = atoi(valChar);
	if (altiPouces == 0) {
		erreursSonar++;
	}

	// 
	// Saut maxi: MAX_SONAR_JUMP 
	//	
	if (altiPouces > ancAltiPouces + SONAR_SAUT_MAXI) {
		altiPouces = ancAltiPouces + SONAR_SAUT_MAXI;
	} else if (altiPouces < ancAltiPouces - SONAR_SAUT_MAXI) {
		altiPouces = ancAltiPouces - SONAR_SAUT_MAXI;
	}
	ancAltiPouces = altiPouces;

	SonarTropBas = 0;
	SonarTropHaut = 0;
	
	if (altiPouces <= 6) {
		// Le sonar est trop proche du sol
		SonarTropBas = 1;
		nbValeursValables = 0;
		VitLRTA[3] = 0.0;
		PosLRTA[3] = 0.0;
		derniereAltitudeMoyenneEstValable = 0;
		
	} else if (altiPouces >= 250) { // En réalité c'est 254, 
		// mais on prend 250 pour etre tranquille
		// Le sonar est trop loin du sol
		SonarTropHaut = 1;
		nbValeursValables = 0;
		VitLRTA[3] = 0.0;
		PosLRTA[3] = 0.01 * 2.54 * 250.0;
		derniereAltitudeMoyenneEstValable = 0;
		
	} else {
		// Le sonar est dans sa plage de fonctionnement
			
		//
		//
		//--Sytème de moyenne sur l'altitude
		//
		alti = 0.01F * 2.54F * (float)altiPouces; // Conversion de pouces en mètres
		nbValeursValables++;
		dernieresAltitudes[indexDerniereAltitude] = alti;
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
			PosLRTA[3] -= SONAR_ALTI_MINI;

			if (derniereAltitudeMoyenneEstValable) {
				// Obtention de la vitesse par dérivation sur les 2 dernieres moyennes
				VitLRTA[3] = INVERSE_INTERVALLE_TEMPS_DERIVATION_SONAR
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

	NouvelleTrameSonar = 1;
}

