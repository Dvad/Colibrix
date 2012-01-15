#include "Params.h"
#include "CommWifi.h"
#include <stdio.h>


float assietteZeroRT[2];


void Pilote_Initialise() {
	
	assietteZeroRT[0] = 0.0F;
	assietteZeroRT[0] = 0.0F;
}


void Pilote_Termine() {
	
	
}

// N'applique que l'ordre d'assiette pour l'instant
void Pilote_AppliqueOrdreLRTA() {
	
	//float BORNES_L = 
	//float BORNES_A = 
	float BORNES_RT = DEG_EN_RAD * 7.0F;

	
	if (OrdreLRTA[1] > -BORNES_RT && OrdreLRTA[1] < BORNES_RT) {
		assietteZeroRT[0] = OrdreLRTA[1];
	} else {
		printf("\nERREUR PILOTE: ordre roulis hors bornes !\n");
	}
	
	if (OrdreLRTA[2] > -BORNES_RT && OrdreLRTA[2] < BORNES_RT) {
		assietteZeroRT[1] = OrdreLRTA[2];
	} else {
		printf("\nERREUR PILOTE: ordre tangage hors bornes !\n");
	}
}


void Pilote_CalculConsignes() {
	
	//return;
	
	// L'intervalle de temps depuis le dernier appel (apart si c'est le premier)
	// est dans la variable "IntervalleTemps", exprim� en secondes flottantes.
	
	// Pour l'instant, le pilote automatique garde en m�moire une assiette
	// Et y ajoute des modifications temporaires
	
	
	
	//6------------------------------ ATTENTION, CE CODE EST INCOMPATIBLE AVEC LA FONCTION ACTUELLE ----------
	if(state == 3 || state == 5) {
		// DECOLLAGE
		ConsLRTA[0] = 0.0F; // Cap fixe lors du d�collage
		ConsLRTA[1] = 0.0F; // Roulis z�ro
		ConsLRTA[2] = 0.0F; // Tangage z�ro
		// ConsLRTA[3] <- g�r� par le noyau
	} else {
		// VOL DE CROISIERE
		//ConsLRTA[3] = ALTITUDE_DE_VOL; // Valeur a laquelle le d�collage am�ne le drone
		
		ConsLRTA[1] = assietteZeroRT[0];
		ConsLRTA[2] = assietteZeroRT[1];
	}
	
	
}
