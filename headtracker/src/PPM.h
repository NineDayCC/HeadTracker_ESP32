#ifndef PPM_H
#define PPM_H

#include <zephyr/kernel.h>

int PPMinit(void);
void buildChannels(void);
void PpmOut_setChannel(int chan, uint16_t val);
int PpmOut_getChnCount();

void printPPMdata(void);    //test

#endif