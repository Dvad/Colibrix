/*
 * i2C.h
 *
 *  Created on: 19 janv. 2012
 *      Author: dav
 */

#ifndef I2C_H_
#define I2C_H_
int I2C_Initialise();
int I2C_Envoyer_Commande_Moteur();
int I2C_Envoyer_Commande_Tout_Moteur();
int I2C_Envoyer_Pulse_Sonar();
int I2C_Envoyer_Lecture_Sonar();
int estInitialiseI2C;


#endif /* I2C_H_ */
