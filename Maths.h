#ifndef MATHS_H_
#define MATHS_H_


float PI;
float DEUX_PI;
float PI_SUR_DEUX;


void Maths_Initialise();
float Sin(float angleRad);
float Cos(float angleRad);
int Modulo(int nn, int mod); // Nécessaire pour les nombres négatifs uniquement, sinon utiliser "%".


#endif /*MATHS_H_*/
