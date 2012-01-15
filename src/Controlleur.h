#ifndef CONTROLLEUR_H_
#define CONTROLLEUR_H_

void Controlleur_Initialise(int testMoteurs);
void Controlleur_Envoi(int mode);
void Controlleur_Termine();

void Controlleur_PrintCmd();
int Controlleur_VMoyen();


#endif /*CONTROLLEUR_H_*/
