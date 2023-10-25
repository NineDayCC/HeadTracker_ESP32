#pragma once
#include "ht.h"

#define GPIO_PPM_OUT_SET GPIO_PPM_OUT

int PPMinit(void);
void buildChannels(void);
void PpmOut_setChannel(int chan, uint16_t val);
int PpmOut_getChnCount();

void printPPMdata(void);    //test

