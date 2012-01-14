#ifndef MISC_H_
#define MISC_H_


int askIntValue(const char *theValueName);
float askFloatValue(const char * theValueName);

int Misc_HasData();
void Misc_PrintHelp();
void Misc_WaitEnter();

void Misc_ResetTimer();
void Misc_ResetElapsedUs();
int Misc_GetElapsedUs();
int Misc_GetTimerUs();

float Misc_Abs(float value);

void Misc_Initialise();


#endif /*MISC_H_*/
