#ifndef __ULTRA_H
#define __ULTRA_H	
#include "sys.h"

void UltrasonicWave_StartMeasure(void);
void UltrasonicWave_Init(void);
float UltrasonicWave_Measure(void);
#endif
