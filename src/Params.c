#include "Params.h"



//
// Constantes
//
void Params_Initialise()
{
	
	
	//
	// Constantes m�caniques
	//
	I_ROUL = 0.038F; // Moment d'inertie en roulis   // Valeur � recalculer
	I_TANG = 0.038F; // Moment d'inertie en tangage  // Valeur � recalculer
	I_LACT = 0.068F; // Moment d'inertie en lacet    // Valeur � recalculer
	MASSE_TOTALE = 1.000F;
	BRAS_DE_LEVIER = 0.20F;
	TRAINEE_MOTEUR = 0.0005F;
	POUSEE_MOTEUR  = 0.00285;
	
	//
	// Constantes logicielles
	//
	CENTRALE_TAILLE_TRAME = 196;   // !! Modifier la taille tableau si modif
	CENTRALE_TAILLE_BUFFER = 196*3; // !! Modifier la taille tableau si modif 
	SONAR_TAILLE_TRAME = 5;   // !! Modifier la taille tableau si modif
	SONAR_TAILLE_BUFFER = 5 * 3; // !! Modifier la taille tableau si modif
	
	//
	// Constantes d'optimisation logicielle
	SONAR_SAUT_MAXI = 1; // (JUMP = 1 <-> 0.6 m/s)
	TEST_SONAR_TOUT_LES = 4; // En nombre de trame. freq centrale = 100, freq sonar = 20 => tout les 5
	DUREE_MOYENNAGE_ALTITUDE = 4; // En unit� de pas sonar (50ms) | 10==0.5s
	VERIFICATION_INPUT_CONSOLE_TOUT_LES = 20; // 20 <=> 100/20 = 5 Hz
	SONAR_ALTI_MINI = 0.15F;
	INVERSE_INTERVALLE_TEMPS_DERIVATION_SONAR = 20.0F; // La fr�quence du sonar
	
	SONAR_IRQ = 300; // 0 (hardware), 200, 300, 400
	CENTRALE_IRQ = 300; // 0 (hardware), 200, 300, 400
	BUF_CTR = 2; // 1 ou 2, en facteur de la taille buffer
		
	
	
	
	//
	// Estimateur
	//
	// Le produit ESTIMATEUR_GAIN_INTEGRATION * ESTIMATEUR_TEMPS_DE_REACTION * ESTIMATEUR_PAS_DE_TEMPS_MOYEN
	// repr�sente le temps d'anticipation total du syst�me
	//
	ESTIMATEUR_GAIN_POUSEE = 0.0F; //1.0F;      // Le gain de la pouss�e vers l'acc�l�ration
	ESTIMATEUR_GAIN_INTEGRATION = 0.90F; // Le gain a l'int�gration de la vitesse en position
	ESTIMATEUR_TEMPS_DE_REACTION = 3; // Le nombre de tour de boucle d'anticipation
	ESTIMATEUR_PAS_DE_TEMPS_MOYEN = 1.0F / 102.0F; // 102 la fr�quence r�elle de la centrale
	
	
	
	//
	// S�curit�s
	//
	ATTENTE_SONAR_MAXI = 100; // En unit� de trames centrales (10ms)
	
	//
	// Variables d'�tats
	//
	for(i = 0; i < 4; i++) {
		PosLRTA[i]  = 0.0F;  // RAZ positions LRTA
		VitLRTA[i]  = 0.0F;  // RAZ vitesses LRTA
		ConsLRTA[i] = 0.0F;  // RAZ consignes LRTA
		ULRTA[i]    = 0.0F;  // RAZ commandes axes
		Commande[i] = 0.0F;  // RAZ commandes moteur
		OrdreLRTA[i]= 0.0F;  // RAZ ordres du Wifi au Pilote
	}
	
	attenteSignal = 0;
	modeI2C = 1; // 0=rien, 1=normal
	
	readSonar = 1;	


	NouvelleTrameSonar = 0;
	NouvelleTrameCentrale = 0;
	
	SonarTropHaut = 0;
	SonarTropBas = 0;
	
	
	MAX_WAIT_BEF_SEND = 8; // Le nombre de pas avant le r�envoi d'une consigne I2C
	// doit etre suffisament petit pour pas qu'il se mette en mode s�curit�
	// exprim� en pas de dizaines de ms
	// VALEUR CRITIQUE EXPERIMENTALE: 15
	
	
	PAS_A = 0.05f; // 5cm
	PAS_L = DEG_EN_RAD * 3.0F;
	PAS_RT = DEG_EN_RAD * 3.0F;
	
	//
	// Dynamique de vol
	//
	VEqui = 900.0F; // A CHANGER APRES RECHARGE BATTERIE !!!!!
	Vactu = 0.0F;
	
	// Les deux valeurs suivantes sont en protocoles I2C !!!
	// Il y a un facteur 10: Ci2c = 0.1 * Creel
	ROTATION_MAXI = 200; // <= 255 pour garantir le protocole I2C
	ROTATION_LENTE = 3; // La vitesse de rotation pour les tests & co
				
	RAMPE_ROTATION = 700; // La rampe d'acc�l�ration de rotation des moteurs en tr/s^2.
	RAMPE_DESCENTE = 0.2F; // m/s^2
	RAMPE_ASCENSION = 0.1F; // m/s^2
	ALTITUDE_FIN_DECOLLAGE = 0.30F; // m

	//
	// Param�tres
	//
	//--Lacet
	CONTROLE_LRTA[0] = 1;
	GAINS_PRO[0] = 20.0F; //40.0F;
	GAINS_INT[0] = 0.0F;
	GAINS_DER[0] = 7.0F; //15.0F;
	
	//--Roulis
	CONTROLE_LRTA[1] = 1;
	GAINS_PRO[1] = 10.0F; //15
	GAINS_INT[1] = 0.0F;
	GAINS_DER[1] = 4.0F; //7

	//--Tangage
	CONTROLE_LRTA[2] = 1;
	GAINS_PRO[2] = 10.0F; //15
	GAINS_INT[2] = 0.0F;
	GAINS_DER[2] = 4.0F; //7

	//--Altitude
	CONTROLE_LRTA[3] = 1;
	GAINS_PRO[3] = 10.0F;
	GAINS_INT[3] = 0.0F;
	GAINS_DER[3] = 3.0F;
	
	
	//
	// Les �carts suivants entrainent un arret d'urgence total !!
	//   En vol, c'est donc catastrophique
	//   Ces param�tres peuvent etre s�rr�s sur le banc d'essai,
	//   MAIS DOIVENT ETRE RELACHES EN VOL REEL !!!!!!!!!!
	ECART_MAXI_A  = CM_EN_M * 50.0F;
	ECART_MAXI_L  = DEG_EN_RAD * 40.0F;
	ECART_MAXI_RT = DEG_EN_RAD * 30.0F;
	

	//
	// Les param�tres suivant saturent la consigne envoy� par le PID
	//   Il doivent donc etre:
	//     - lache en RT
	//     - moyen en A
	//     - rel. s�rr� en L
	UA_MAX = GAINS_PRO[3] * CM_EN_M * 60.0F; // En centim�tres
	UA_MIN = - UA_MAX;
	
	UL_MAX = GAINS_PRO[0] * DEG_EN_RAD * 30.0F; // Degr�es
	UL_MIN = - UL_MAX;
		
	URT_MAX = GAINS_PRO[1] * DEG_EN_RAD * 50.0F; // Degr�es
	URT_MIN = - URT_MAX;
	
}




