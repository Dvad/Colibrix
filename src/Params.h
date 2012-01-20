#ifndef PARAMS_H_
#define PARAMS_H_


#define DEG_EN_RAD 0.01745F   // Le facteur de conversion pour convertir des degrées en radians
#define CM_EN_M 0.01F         // Idem de centimetres en métres
 
int i;

//
// Constantes mécaniques
//
float I_ROUL;
float I_TANG;
float I_LACT;
float MASSE_TOTALE;
float BRAS_DE_LEVIER;
float TRAINEE_MOTEUR;
float POUSEE_MOTEUR;


float UA_MAX;
float UA_MIN;
float UL_MAX;
float UL_MIN;
float URT_MAX;
float URT_MIN;


float ECART_MAXI_A;
float ECART_MAXI_L;
float ECART_MAXI_RT;

float PAS_A;
float PAS_L;
float PAS_RT;

int modeI2C;
int BUF_CTR;
int readSonar;

//
// Constantes logicielles
//
int TEST_SONAR_TOUT_LES;

int CENTRALE_TAILLE_TRAME;
int CENTRALE_TAILLE_BUFFER;

int SONAR_TAILLE_BUFFER;
int SONAR_TAILLE_TRAME;

int SONAR_SAUT_MAXI;
float SONAR_ALTI_MINI;
float INVERSE_INTERVALLE_TEMPS_DERIVATION_SONAR;

int ATTENTE_SONAR_MAXI;


float COEF_GPS_POS;
float COEF_GPS_VIT;


float RAMPE_ROTATION;
float RAMPE_DESCENTE;
float RAMPE_ASCENSION;


int ESTIMATEUR_TEMPS_DE_REACTION;
float ESTIMATEUR_GAIN_POUSEE;
float ESTIMATEUR_GAIN_INTEGRATION;
float ESTIMATEUR_PAS_DE_TEMPS_MOYEN;

int VERIFICATION_INPUT_CONSOLE_TOUT_LES;
float ALTITUDE_FIN_DECOLLAGE;

int SonarTropHaut;
int SonarTropBas;

int freqSonar;
	int erreursSonar;
	int recherchesSonar;
	int freqCentrale;
	int erreursCentrale;
	int recherchesCentrale;
	

int tramesEffacees;
		
		
//
// Variables d'état
//
int attenteSignal;
float OrdreLRTA[4];
		
int NouvelleTrameSonar;
int NouvelleTrameCentrale;
float PosLRTA[4];  // Positions LRTA
float VitLRTA[4];  // Vitesses LRTA
float ConsLRTA[4]; // Consignes LRTA
float ULRTA[4];    // Commandes axes
float Commande[4]; // Commandes moteur
int CONTROLE_LRTA[4];   // Si oui ou non le controle de cet axe est actif
float IntervalleTemps; // L'intervalle de temps entre la trame actuelle et la précédente
float VEqui;           // La commande moteur d'équilibre
float Vactu;
int state;
float AltitudeBaro;

int trameRate;
int MAX_WAIT_BEF_SEND;
int ROTATION_MAXI; // <= 255 pour garantir le protocole I2C
int ROTATION_LENTE; // La vitesse de rotation pour les tests & co

int SONAR_IRQ;
int CENTRALE_IRQ;

int DUREE_MOYENNAGE_ALTITUDE;
int DUREE_MOYENNAGE_BARO;

//
// Paramétres
//
float GAINS_PRO[4]; // Les gains proportionnels pour la commande en LRTA
float GAINS_INT[4]; // intégraux
float GAINS_DER[4]; // dérivées
//
//Paramètres du Sonar SRF08
unsigned char GAIN_MAX_SONAR;
unsigned char RANGE_SONAR;
//
// Liste des fonctions
//
void Params_Initialise();

#endif /*PARAMS_H_*/
