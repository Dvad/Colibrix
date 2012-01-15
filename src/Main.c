#include "Misc.h"
#include "Maths.h"
#include "Params.h"
//#include "Pilote.h"
#include "Sonar.h"
#include "Centrale.h"
#include "Asservissement.h"
#include "Controlleur.h"
//#include "CommWifi.h"
#include <stdio.h>
#include <sys/resource.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

/* ANCIEN MAIN
   // manipulation des chaînes de caractères
 #include <strings.h> // manipulation des chaînes de caractères
 // convention : les chaines de caractère se terminent par le caractère nul '\0'.
 #include <sys/types.h>
 #include <sys/stat.h>
 #include <sys/time.h> // pour calculer le temps (gettimeofday(), struct timeval )
 */

int system(const char *string);
int atoi(const char * str);
float atof(const char * str);
//void usleep(int us);
int getpid();



void videBufferSonar() {
	printf("Vidage buffer sonar...");
	fflush(stdout);
	for (i = 0; i < 100; i++) {
		Sonar_CheckData(0);
	}
	printf(" OK\n");
}


int main(int argc, char **argv) {

	//
	// Machine d'état
	//
	// int state
	// -1: erreur
	//  0: non initialisé
	//  1: initialisé, pret rotation
	//  2: rotation, pret décollage
	//  3: décollage
	//  4: vol normal
	//  5: atterissage
	//
	state = 0;

	printf("\n");
	printf("\n================================================");
	printf("\n=====    XMicroDrone - Controlleur V1.1    =====");
	printf("\n================================================");
	printf("\n");

	//
	// Récupérons les numéros de devices séries (paramètre optionnel)
	//
	int premierDevice = 0; // La valeur par défaut, qu'on utilisera s'il n'y a pas d'arguments
	if (argc >= 2) {
		int arg = atoi(argv[1]);
		if (arg >= 0 && arg <= 2) {
			premierDevice = arg;
		} else {
			printf("Argument devices hors de portée.\n");
		}
	}

	//---------------------------------------------------------------------------
	//
	//
	//                        INITIALISATION DU PROGRAMME
	//
	//
	//---------------------------------------------------------------------------

	//
	// Rend le fgets sur stdin non bloquant; FONCTIONNE !
	//
	int flags = fcntl(0, F_GETFL, 0);
	flags |= O_NONBLOCK;
	fcntl(0, F_SETFL, flags);

	printf("Initialisation des différents modules...");
	fflush(stdout);

	Misc_Initialise(); // Ne dépend de rien d'autre // Pas de blabla
	Maths_Initialise(); // Ne dépend de rien d'autre // Pas de blabla
	Params_Initialise(); // Après Maths_Initialise(); (peut utiliser des maths) // Pas de blabla
	Asservissement_Initialise(); // Après Params_Initialise();
	Pilote_Initialise(); // Après Params, // Pas de blabla

	printf(" OK\n");

	printf("Fermeture des serveurs port séries...");
	fflush(stdout);
	system("pkill -9 xuartctl"); // kill tous les process utilisant xuartctl
	usleep(100 * 1000);
	printf(" OK\n");

	// La fonction suivante émet son propre blabla
	Controlleur_Initialise(0); // Après Params_Initialise(); // Argument: 1 pour allumer les moteurs successivement


	int grnLedOn = 0;
	int redLedOn = 0;
	Misc_SetRedLed(0);
	Misc_SetGrnLed(0);

	if (0) {
		CommWiFi_Initialise(); // Emet son propre blabla
	}

	if (0) {
		printf("Fermeture des process inutiles...");
		fflush(stdout);
		system("pkill -9 dioctl");
		usleep(100000);
		system("pkill -9 ts7500ctl");
		usleep(100000);
		printf(" OK\n");
	}

	// On se donne temporairement la priorité -15 pour que les xuart l'aient
	//
	// -15 pour les xuartctl (Port séries) et ts7500ctl (I2C)
	// -10 pour nous et ts7500ctl
	// -5 pour les trucs systemes importants
	// 0 pour le reste
	// Plage: -20 (haute priorité) à +20 (faible priorité)
	int usePrio = 0;

	if (usePrio && setpriority(PRIO_PROCESS, getpid(), -15)) {
		printf("ERREUR: setpriority\n");
	}

	Centrale_Initialise(premierDevice); // Argument: le dev par défaut, -1 pour laisser l'utilisateur choisir

	Sonar_Initialise(premierDevice + 1); // Argument: le dev par défaut, -1 pour laisser l'utilisateur choisir

	// On se replace a -10
	if (usePrio && setpriority(PRIO_PROCESS, getpid(), -10)) {
		printf("ERREUR: setpriority !\n");
	}

	printf("Merci de vérifier les numéros de devices ci-dessus\n");
	if (usePrio) {
		printf("ET de vous assurer que la priorité du process ts5700ctl est strictement négative !\n");
	}

	if (0) {
		Misc_WaitEnter();
	}

	float tempsSCum = 0;
	int dispType = 1; // ----------------------------------
	int attenteSonar = 0;
	int attenteCentrale = 0;
	int appelsCentrale = 0;
	int appelsSonar = 0;
	int dernierTestSonar = 0;
	int dispUneFois = 0;

	int toursAvantVerInputConsole = VERIFICATION_INPUT_CONSOLE_TOUT_LES - 1; // Initialisé ainsi pour vérifier une commande 
	//   dès le 1er tour de boucle

	int dispFreqOnce = 0;

	//
	// Liste des commandes
	//
	int MAX_COMMAND_LENGTH = 7 + 2; // Prendre la longueur réelle + 2 pour le \n et un cran de sécurité !!!


	char chSETPR[] = "set ep"; // Pas d'estimation
	//---Réglages des gains
	char chSETLP[] = "set lp"; // Régler le gain lacet proportionnel
	char chSETLI[] = "set li"; // Régler le gain lacet intégral
	char chSETLD[] = "set ld"; // Régler le gain lacet dérivé
	char chSETRP[] = "set rp"; // Régler le gain roulis proportionnel
	char chSETRI[] = "set ri"; // Régler le gain roulis intégral
	char chSETRD[] = "set rd"; // Régler le gain roulis dérivé
	char chSETTP[] = "set tp"; // Régler le gain tangage proportionnel
	char chSETTI[] = "set ti"; // Régler le gain tangage intégral
	char chSETTD[] = "set td"; // Régler le gain tangage dérivé
	char chSETAP[] = "set ap"; // Régler le gain altitude proportionnel
	char chSETAI[] = "set ai"; // Régler le gain altitude intégral
	char chSETAD[] = "set ad"; // Régler le gain altitude dérivé


	videBufferSonar();
	

	//---------------------------------------------------------------------------
	//
	//
	//                    FONCTIONNEMENT NORMAL: BOUCLE INFINIE
	//
	//
	//---------------------------------------------------------------------------
	printf("Initialisation terminée !\n");
	fflush(stdout);
	Misc_ResetElapsedUs();

	for (;;) { // La boucle infinie du programme

		//
		// Vérification de la liaison
		//
		if (state > 1 && modeI2C > 0) {
			attenteSignal++;
			if (attenteSignal == 300) { // 5 sec avant warning
				printf("\nATTENTION: communication inactive, merci d'appuyer sur entrée !\n");
				fflush(stdout);
			} else if (attenteSignal == 500) { // 10 sec avant désactivation

				if (state == 3 || state == 4) {
					state = 5; // Lance un atterissage si en vol
					printf("\nERREUR: Aucun signal wifi, atterissage d'urgence !\n");
				} else {
					state = -1; // Arret d'urgence sinon
					printf("\nERREUR: Aucun signal wifi, arret d'urgence !\n");
				}
				fflush(stdout);
			}
		}

		//////////////////////////////////////////////////////////////////
		//
		// Regardons si une commande a été tapé dans la console 
		//
		//////////////////////////////////////////////////////////////////
		toursAvantVerInputConsole++;
		if (toursAvantVerInputConsole >= VERIFICATION_INPUT_CONSOLE_TOUT_LES) {
			toursAvantVerInputConsole = 0;

			if (1 || Misc_HasData(0)) { // Misc_HasData(0) fonctionne bien en ethernet
				char *cptr;
				char buffer[MAX_COMMAND_LENGTH]; // Ex 256 au lieu de MAX_COMMAND_LENGTH
				cptr = fgets(buffer, MAX_COMMAND_LENGTH, stdin); // Ex 256 au lieu de MAX_COMMAND_LENGTH
				if (cptr != NULL) {

					attenteSignal = 0; // Watchdog: la communication est active !


					//
					// Convertissons le char* en char[] pour pouvoir le comparer a l'aide de strncmp
					//
					char dst[MAX_COMMAND_LENGTH];
					strncpy(dst, cptr, MAX_COMMAND_LENGTH - 1);
					dst[MAX_COMMAND_LENGTH - 1] = '\0';

					if (dispType > 0) {
						printf("\n"); // Le dispCommande affiche une ligne puis l'enleve en permanence
					}

					//
					// Comparons le a la liste des commandes implémentés
					//
					//------------------------------------------------
					//--------------- TOUCHE "ENTREE" ----------------
					//------------------------------------------------
					if ((int)dst[0] == 10) {
						dispUneFois = 1;
						// Nous poursuivons l'éxécution tranquillement
					}

					//---------------- Traitons en premier les commandes longues -------------


					//------------------------------------------------
					//-------------- COMMANDES "SET **" --------------
					//------------------------------------------------
					else if (strncmp(chSETLP, dst, sizeof chSETLP - 1) == 0) {
						GAINS_PRO[0]
								= askFloatValue("Gain proportionnel lacet");
					} else if (strncmp(chSETLI, dst, sizeof chSETLI - 1) == 0) {
						GAINS_INT[0] = askFloatValue("Gain intégral lacet");
					} else if (strncmp(chSETLD, dst, sizeof chSETLD - 1) == 0) {
						GAINS_DER[0] = askFloatValue("Gain dérivé lacet");
					} else if (strncmp(chSETRP, dst, sizeof chSETRP - 1) == 0) {
						GAINS_PRO[1]
								= askFloatValue("Gain proportionnel roulis");
					} else if (strncmp(chSETRI, dst, sizeof chSETRI - 1) == 0) {
						GAINS_INT[1] = askFloatValue("Gain intégral roulis");
					} else if (strncmp(chSETRD, dst, sizeof chSETRD - 1) == 0) {
						GAINS_DER[1] = askFloatValue("Gain dérivé roulis");
					} else if (strncmp(chSETTP, dst, sizeof chSETTP - 1) == 0) {
						GAINS_PRO[2]
								= askFloatValue("Gain proportionnel tangage");
					} else if (strncmp(chSETTI, dst, sizeof chSETTI - 1) == 0) {
						GAINS_INT[2] = askFloatValue("Gain intégral tangage");
					} else if (strncmp(chSETTD, dst, sizeof chSETTD - 1) == 0) {
						GAINS_DER[2] = askFloatValue("gain dérivé tangage");
					} else if (strncmp(chSETAP, dst, sizeof chSETAP - 1) == 0) {
						GAINS_PRO[3]
								= askFloatValue("GAIN proportionnel altitude");
					} else if (strncmp(chSETAI, dst, sizeof chSETAI - 1) == 0) {
						GAINS_INT[3] = askFloatValue("GAIN intégral altitude");
					} else if (strncmp(chSETAD, dst, sizeof chSETAD - 1) == 0) {
						GAINS_DER[3] = askFloatValue("GAIN dérivé altitude");
					}

					//------------------------------------------------
					//--------------- COMMANDE "SET PR" -----------------
					//------------------------------------------------
					else if (strncmp(chSETPR, dst, sizeof chSETPR - 1) == 0) {
						if (state == 0 || state == 1) {
							printf(
									"\nTemps d'anticipation actuel de l'estimateur: %i\n",
									ESTIMATEUR_TEMPS_DE_REACTION);
							int value = askIntValue("TDR");
							if (value >= 1 && value <= 5) {
								printf("Nouvelle valeur acceptée !");
								ESTIMATEUR_TEMPS_DE_REACTION = value;
								Asservissement_Initialise();
							} else {
								printf("ERREUR: Nouvelle valeure refusée !\n");
								fflush(stdout);
							}
						} else {
							printf("Commande inaccessible dans cet état !\n");
							fflush(stdout);
						}
					}

					//---------------- Traitons en second les commandes courtes -------------

					//------------------------------------------------
					//-------------- COMMANDE NAVIG. -----------------
					//------------------------------------------------
					else if (dst[0] == 'a') {
						ConsLRTA[0] += PAS_L;
					} else if (dst[0] == 'e') {
						ConsLRTA[0] -= PAS_L;
					} else if (dst[0] == 'q') {
						ConsLRTA[1] += PAS_RT;
					} else if (dst[0] == 'd') {
						ConsLRTA[1] -= PAS_RT;
					} else if (dst[0] == 'z') {
						ConsLRTA[2] -= PAS_RT;
					} else if (dst[0] == 's') {
						ConsLRTA[2] += PAS_RT;
					} else if (dst[0] == 'r') {
						ConsLRTA[3] += PAS_A;
					} else if (dst[0] == 'f') {
						ConsLRTA[3] -= PAS_A;
					}

					//------------------------------------------------
					//------------- COMMANDES VEQ --------------------
					//------------------------------------------------
					else if (dst[0] == 'p') {
						VEqui += 50.0F;
						printf("Nouvelle vitesse d'équilibre: %f\n", VEqui);
						fflush(stdout);
						if (state == 4) {
							Vactu = VEqui;
						}
					} else if (dst[0] == 'm') {
						VEqui -= 50.0F;
						printf("Nouvelle vitesse d'équilibre: %f\n", VEqui);
						fflush(stdout);
						if (state == 4) {
							Vactu = VEqui;
						}
					}
					
					//------------------------------------------------
					//------------- COMMANDES A_VOL --------------------
					//------------------------------------------------
					else if (dst[0] == '$') {
						ALTITUDE_FIN_DECOLLAGE += 0.05F;
						if(ALTITUDE_FIN_DECOLLAGE > 0.8F) {
							ALTITUDE_FIN_DECOLLAGE = 0.8F;
						}
						printf("Nouvelle altitude cible au décollage: %icm\n", (int)(100.0F * ALTITUDE_FIN_DECOLLAGE));
						fflush(stdout);
					} else if (dst[0] == '*') {
						ALTITUDE_FIN_DECOLLAGE -= 0.05F;
						if(ALTITUDE_FIN_DECOLLAGE < 0.3F) {
							ALTITUDE_FIN_DECOLLAGE = 0.3F;
						}
						printf("Nouvelle altitude cible au décollage: %icm\n", (int)(100.0F * ALTITUDE_FIN_DECOLLAGE));
						fflush(stdout);
					}

					
					//------------------------------------- MACHINE ETAT -------------------------------
					//------------------------------------------------
					//--------------- COMMANDE "w" -----------------
					//------------------------------------------------
					else if (dst[0] == 'w') {
						if (state == -1) {
							printf("Erreur ignoré !\n");
							state = 0;
						} else {
							printf("ERREUR: La commande ne peut être appliqué dans ce contexte.\n");
						}
						fflush(stdout);
					}
					//------------------------------------------------
					//--------------- COMMANDE "x" -----------------
					//------------------------------------------------
					else if (dst[0] == 'x') {
						if (state == -1 || state == 0 || state == 1) {
							printf("Initialisation... OK\n");
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
							printf("ERREUR: La commande ne peut être appliqué dans ce contexte.\n");
						}
						fflush(stdout);
					}
					//------------------------------------------------
					//--------------- COMMANDE "c" -----------------
					//------------------------------------------------
					else if (dst[0] == 'c') {
						if (state == 1) {
							printf("Mise en rotation...\n");
							state = 2;
						} else {
							printf("ERREUR: La commande ne peut être appliqué dans ce contexte.\n");
						}
						fflush(stdout);
					}
					//------------------------------------------------
					//--------------- COMMANDE "v" -----------------
					//------------------------------------------------
					else if (dst[0] == 'v') {
						if (state == 2) {
							printf("Décollage...\n");
							Vactu = 0.0F;
							state = 3;
						} else {
							printf("ERREUR: La commande ne peut être appliqué dans ce contexte.\n");
						}
						fflush(stdout);
					}
					//------------------------------------------------
					//--------------- COMMANDE "b" -----------------
					//------------------------------------------------
					else if (dst[0] == 'b') {
						if (state == 4 || state == 3) {
							printf("Atterissage...\n");
							state = 5;
						} else {
							printf("ERREUR: La commande ne peut être appliqué dans ce contexte.\n");
						}
						fflush(stdout);
					}

					//------------------------------------------------
					//--------------- COMMANDE "STOP" ----------------
					//------------------------------------------------
					else if (dst[0] == ' ') {
						state = -1;
						printf("\n !! ARRET D'URGENCE !! \n");
						fflush(stdout);
					}
					
					
					//------------------------------------- Divers ---------------
					
					//------------------------------------------------
					//--------------- COMMANDE "k" ----------------
					//------------------------------------------------
					else if (dst[0] == 'k') {
						printf("Arret du programme...\n");
						fflush(stdout);
						break; // Termine la boucle et entraine la fermeture du programme
					}

					//------------------------------------------------
					//--------------- COMMANDE "n" -----------------
					//------------------------------------------------
					else if (dst[0] == 'n') {
						dispType++;
						if (dispType == 3) {
							dispType = 0;
						}
						printf("Mode d'affichage changé. (mode actuel: %i)\n",
								dispType);
						fflush(stdout);
					}
					//------------------------------------------------
					//--------------- COMMANDE "j" -----------------
					//------------------------------------------------
					else if (dst[0] == 'j') {
						modeI2C++;
						if (modeI2C == 2) {
							modeI2C = 0;
						}
						printf(
								"Mode de controle I2C changé. (mode actuel: %i)\n",
								modeI2C);
						fflush(stdout);
					}
					//------------------------------------------------
					//--------------- COMMANDE "u" -----------------
					//------------------------------------------------
					else if (dst[0] == 'u') {
						BUF_CTR++;
						if (BUF_CTR == 3) {
							BUF_CTR = 1;
						}
						printf(
								"Buffer centrale changé. (facteur actuel: %i)\n",
								BUF_CTR);
						fflush(stdout);
					}
					//------------------------------------------------
					//--------------- COMMANDE "y" -----------------
					//------------------------------------------------
					else if (dst[0] == 'y') {
						if (readSonar == 1) {
							readSonar = 0;
							printf("Lecture sonar désactivée !\n");
						} else {
							readSonar = 1;
							printf("Lecture sonar activée !\n");
						}
						fflush(stdout);
					}

					//------------------------------------------------
					//--------------- COMMANDE "i" ----------------
					//------------------------------------------------
					else if (dst[0] == 'i') {
						dispFreqOnce = 1;
					}

					//------------------------------------------------
					//-------------- COMMANDE "l" ---------------
					//------------------------------------------------
					else if (dst[0] == 'l') {
						// Listons les réglages PID actuels
						printf("\n               Proportionnel      Intégral         Dérivé\n");
						printf("Lacet    : %15f %15f %15f\n", GAINS_PRO[0],
								GAINS_INT[0], GAINS_DER[0]);
						printf("Roulis   : %15f %15f %15f\n", GAINS_PRO[1],
								GAINS_INT[1], GAINS_DER[1]);
						printf("Tangage  : %15f %15f %15f\n", GAINS_PRO[2],
								GAINS_INT[2], GAINS_DER[2]);
						printf("Altitude : %15f %15f %15f\n", GAINS_PRO[3],
								GAINS_INT[3], GAINS_DER[3]);
						printf("Vequi    : %15f", VEqui);
						fflush(stdout);
					}
					//------------------------------------------------
					//---------------- COMMANDE "HELP" ---------------
					//------------------------------------------------
					else if (dst[0] == 'h') {
						Misc_PrintHelp();
						fflush(stdout);
					}
					//------------------------------------------------
					//------------- COMMANDE NON RECONNUE ------------
					//------------------------------------------------
					else {
						printf("\nCommande non reconnue: %s", cptr); // Pas de \n car cptr en contient un !
						printf("\nAppuyez sur 'h' pour obtenir de l'aide.\n");
						fflush(stdout);
					}
				}
			}
		}

		//////////////////////////////////////////////////////////////////
		//
		// Récupération de l'état du système 
		//
		//////////////////////////////////////////////////////////////////


		if (state != -1) {

			//
			// On attend une nouvelle trame centrale
			//
			attenteCentrale = 0;
			for (;;) {

				appelsCentrale++;
				Centrale_CheckData(1);

				if (NouvelleTrameCentrale) {
					NouvelleTrameCentrale = 0;
					break;
					//--La trame a été récupéré et analysé, nous avons alors la nouvelle attitude du drone
					// La variable trameRate a été mise a 1 si on a raté une trame entre temps !
				} else {
					attenteCentrale++;

					if (attenteCentrale > 1000 * 100) { // La valeur de 100 convient parfaitement en lecture bloquante
						printf("\nERREUR: Pas de données centrale !\n");
						fflush(stdout);
						state = -1;
						break;
					}

				}
			}
		}

		dernierTestSonar++;
		if (readSonar && state != -1 && dernierTestSonar >= TEST_SONAR_TOUT_LES
				&& !trameRate) {
			dernierTestSonar = 0;

			//
			// On regarde si on a une trame sonar
			//
			appelsSonar++;
			Sonar_CheckData(1);
			if (NouvelleTrameSonar) {
				NouvelleTrameSonar = 0;
				attenteSonar = 0;

				//--Tant mieux, l'altitude a été mis-à-jour
				if (SonarTropBas) {
					if (state == 4) {
						printf("\nDANGER: Trop bas");
						fflush(stdout);
					}
				} else if (SonarTropHaut) {
					if (state == 4) {
						printf("\nDANGER: Trop haut, baisse consigne");
						fflush(stdout);
						ConsLRTA[3] -= 0.005F; // 0.5cm 20 fois par seconde => 10cm / s
					}
				} else {
					// On est dans la plage du sonar !
				}

			} else {
				//--Tant pis, nous avons toujours l'ancienne valeur
				attenteSonar++;
				if (attenteSonar > ATTENTE_SONAR_MAXI) {
					printf("\nERREUR: Pas de données sonar !\n");
					fflush(stdout);
					state = -1;
				}
			}
		}

		//////////////////////////////////////////////////////////////////
		//
		// Calcul de l'intervalle de temps
		//
		//////////////////////////////////////////////////////////////////

		//
		// Important:
		//
		// A ce stade de la boucle, nous avons récupéré une trame centrale, et en avons extrait l'information
		// temporelle qui nous a permis de calculer l'intervalle de temps réel entre les deux trames.
		// Cette intervalle est stocké dans "IntervalleTemps" et est utilisé par Asservissement_Controle()
		// et puis par Pilote_CalculConsignes();
		//


		//
		// Calcul de l'intervalle de temps avec les trames centrale
		//
		tempsSCum += IntervalleTemps;
		float TOUT_LES = 0.25F;
		if (tempsSCum >= TOUT_LES) {
			tempsSCum -= TOUT_LES;
			dispUneFois = 1;
		}

		if (1) { // Clignottement LED verte
			if (tempsSCum > 0.5F && !grnLedOn) {
				grnLedOn = 1;
				Misc_SetGrnLed(grnLedOn);
			} else if (tempsSCum < 0.5F && grnLedOn) {
				grnLedOn = 0;
				Misc_SetGrnLed(grnLedOn);
			}
		}

		//////////////////////////////////////////////////////////////////
		//
		// Transitions d'état automatiques et sécurités
		//
		//////////////////////////////////////////////////////////////////

		// 3 -> 4 (Fin du décollage)
		if (state == 3) {

			if (Vactu < VEqui) {
				// Premiere phase du décollage: amener Vactu a VEqui
				Vactu += RAMPE_ROTATION * IntervalleTemps;
				if (Vactu >= VEqui) {
					Vactu = VEqui;
					printf("\nPuissance de vol atteinte ! Ascension verticale...\n");
					fflush(stdout);
				}
			} else {

				// Seconde phase: ascension verticale
				ConsLRTA[3] += RAMPE_ASCENSION * IntervalleTemps;

				if (ConsLRTA[3] >= ALTITUDE_FIN_DECOLLAGE) {
					ConsLRTA[3] = ALTITUDE_FIN_DECOLLAGE;
					state = 4;
					printf("\nDécollage terminé ! Vol de croisière.\n");
					fflush(stdout);
				}
			}
		}

		// 5 -> 1 (Fin de l'atterissage)
		if (state == 5) {
			if (ConsLRTA[3] > 0.0F) {
				// Premiere phase: Descente verticale
				ConsLRTA[3] -= RAMPE_DESCENTE * IntervalleTemps;
				if (ConsLRTA[3] <= 0.0F) {
					ConsLRTA[3] = 0.0F;
					printf("\nDescente terminée !\n");
					fflush(stdout);
				}
			} else {
				// Seconde phase: extinction des moteurs
				Vactu -= RAMPE_ROTATION * IntervalleTemps;
				if (Vactu <= 0.0F) { // Sera limité par la valeur minimale de toute façon
					Vactu = 0.0F;
					state = 1;
					printf("\nAtterissage terminé !\n");
					fflush(stdout);
				}
			}
		}

		//
		// Vérification des plages de fonctionnement
		//
		if (state >= 1) { // Initialisé ou en vol

			//--Angle de lacet
			if (Misc_Abs(ConsLRTA[0] - PosLRTA[0]) > ECART_MAXI_L) {
				printf(
						"\nERREUR: Angle de lacet hors de porté (Cons=%5f°, Pos=%5f°)!\n",
						57 * ConsLRTA[0], 57 * PosLRTA[0]);
				fflush(stdout);
				state = -1;
			}

			//--Angle de roulis
			if (Misc_Abs(ConsLRTA[1] - PosLRTA[1]) > ECART_MAXI_RT) {
				printf(
						"\nERREUR: Angle de roulis hors de porté (Cons=%5f°, Pos=%5f°)!\n",
						57 * ConsLRTA[1], 57 * PosLRTA[1]);
				fflush(stdout);
				state = -1;
			}

			//--Angle de tangage
			if (Misc_Abs(ConsLRTA[2] - PosLRTA[2]) > ECART_MAXI_RT) {
				printf(
						"\nERREUR: Angle de tangage hors de porté (Cons=%5f°, Pos=%5f°)!\n",
						57 * ConsLRTA[2], 57 * PosLRTA[2]);
				fflush(stdout);
				state = -1;
			}

			//--Altitude
			if (Misc_Abs(ConsLRTA[3] - PosLRTA[3]) > ECART_MAXI_A) {
				printf(
						"\nERREUR: Altitude hors de porté (Cons=%5fcm, Pos=%5fcm)!\n",
						100 * ConsLRTA[3], 100 * PosLRTA[3]);
				fflush(stdout);
				state = -1;
			}
		}

		//////////////////////////////////////////////////////////////////
		//
		// Calcul et envoi de la commande 
		//
		//////////////////////////////////////////////////////////////////


		if (state == -1 || state == 0 || state == 1) {

			// Aucune rotation
			Controlleur_Envoi(0);

		} else if (state == 2) {

			// Rotation lente
			Controlleur_Envoi(1);

		} else if (state == 3 || state == 4 || state == 5) {

			//Pilote_CalculConsignes();

			Asservissement_Controle();

			Controlleur_Envoi(2);
		}

		//////////////////////////////////////////////////////////////////
		//
		// Affichage de quelques infos 
		//
		//////////////////////////////////////////////////////////////////

		if (dispUneFois) {
			dispUneFois = 0;

			//
			// Mise a jour LED rouge
			//
			if (state == -1 && !redLedOn) {
				redLedOn = 1;
				Misc_SetRedLed(redLedOn);
			} else if (state >= 0 && redLedOn) {
				redLedOn = 0;
				Misc_SetRedLed(redLedOn);
			}

			//
			// dispType :
			//   0 : aucun affichage régulier
			//   1 : affichage ligne d'état
			//   2 : affichage ligne d'état + freqs
			if (dispType > 0) {

				//
				// Mode I2C
				//
				printf("\n[M=%i]", modeI2C);

				//
				// Etat
				//
				if (state == -1) {
					printf("[ERR]");
				} else if (state == 0) {
					printf("[N.I]");
				} else if (state == 1) {
					printf("[PRT]");
				} else if (state == 2) {
					printf("[ROT]");
				} else if (state == 3) {
					printf("[DEC]");
				} else if (state == 4) {
					printf("[VOL]");
				} else if (state == 5) {
					printf("[ATT]");
				}

				//
				// Vmoyen et equi
				//
				printf("[%i/%i/%i]", Controlleur_VMoyen(), (int)(VEqui),
						Ass_CorAlti());

				//
				// Sonar
				//
				printf("A: ");
				if (SonarTropBas) {
					printf("T.BAS");
				} else if (SonarTropHaut) {
					printf("T.HAUT");
				} else {
					printf("%3i", (int)(100 * PosLRTA[3]));
				}
				printf("/%3icm", (int)(100 * ConsLRTA[3]));

				//
				// Lacet, roulis, tangage
				//
				printf(" L: %3i/%3i°  R: %2i/%2i°  T: %2i/%2i°", (int)(57
						* PosLRTA[0]), (int)(57 * ConsLRTA[0]), (int)(57
						* PosLRTA[1]), (int)(57 * ConsLRTA[1]), (int)(57
						* PosLRTA[2]), (int)(57 * ConsLRTA[2]));

				//
				// Commande
				//
				//	if (dispType == 3) {
				//		Controlleur_PrintCmd();
				//	}

				if (dispType == 2 || dispFreqOnce) {

					printf(
							"\nCENTRALE: Ap: %5i  Tr: %4i  Er: %4i  Re: %4i  Ef: %4i",
							appelsCentrale, freqCentrale, erreursCentrale,
							recherchesCentrale, tramesEffacees);

					printf(
							"\nSONAR   : Ap: %5i  Tr: %4i  Er: %4i  Re: %4i  Ef: %4i",
							appelsSonar, freqSonar, erreursSonar,
							recherchesSonar, 0);

					dispFreqOnce = 0;
				}
				fflush(stdout);
			}

			freqSonar = 0;
			appelsSonar = 0;
			erreursSonar = 0;
			recherchesSonar = 0;

			freqCentrale = 0;
			appelsCentrale = 0;
			erreursCentrale = 0;
			recherchesCentrale = 0;
			tramesEffacees = 0;

		}

		//////////////////////////////////////////////////////////////////
		//
		// Fin de la boucle, temporisation si nécessaire 
		//
		//////////////////////////////////////////////////////////////////

		if (state == -1) {
			usleep(10*1000);
		}

	} // Fin de la boucle infinie


	//---------------------------------------------------------------------------
	//
	//
	//                    FERMETURE DU PROGRAMME: LIBERATION MEMOIRE
	//
	//
	//---------------------------------------------------------------------------

	//--Envoi d'une consigne zero
	Controlleur_Envoi(0);

	printf("Fermeture du programme...\n");
	fflush(stdout);

	Sonar_Termine();
	Centrale_Termine();
	Controlleur_Termine();

	Pilote_Termine();
	CommWiFi_Termine();

	Misc_SetGrnLed(0);
	Misc_SetRedLed(0);

	printf("Programme terminé !\n");
	fflush(stdout);
	return 1;
}



