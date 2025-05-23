/****************************************************************************************************************************
  ESP8266_ISR_Timer-Impl.h
  For ESP8266 boards
  Written by Khoi Hoang

  Built by Khoi Hoang https://github.com/khoih-prog/ESP8266TimerInterrupt
  Licensed under MIT license

  The ESP8266 timers are badly designed, using only 23-bit counter along with maximum 256 prescaler. They're only better than UNO / Mega.
  The ESP8266 has two hardware timers, but timer0 has been used for WiFi and it's not advisable to use. Only timer1 is available.
  The timer1's 23-bit counter terribly can count only up to 8,388,607. So the timer1 maximum interval is very short.
  Using 256 prescaler, maximum timer1 interval is only 26.843542 seconds !!!

  Now with these new 16 ISR-based timers, the maximum interval is practically unlimited (limited only by unsigned long miliseconds)
  The accuracy is nearly perfect compared to software timers. The most important feature is they're ISR-based timers
  Therefore, their executions are not blocked by bad-behaving functions / tasks.
  This important feature is absolutely necessary for mission-critical tasks.

  Based on SimpleTimer - A timer library for Arduino.
  Author: mromani@ottotecnica.com
  Copyright (c) 2010 OTTOTECNICA Italy

  Based on BlynkTimer.h
  Author: Volodymyr Shymanskyy

  Version: 1.6.0

  Version Modified By   Date      Comments
  ------- -----------  ---------- -----------
  1.0.0   K Hoang      23/11/2019 Initial coding
  1.0.1   K Hoang      25/11/2019 New release fixing compiler error
  1.0.2   K.Hoang      26/11/2019 Permit up to 16 super-long-time, super-accurate ISR-based timers to avoid being blocked
  1.0.3   K.Hoang      17/05/2020 Restructure code. Fix example. Enhance README.
  1.1.0   K.Hoang      27/10/2020 Restore cpp code besides Impl.h code to use if Multiple-Definition linker error.
  1.1.1   K.Hoang      06/12/2020 Add Version String and Change_Interval example to show how to change TimerInterval
  1.2.0   K.Hoang      08/01/2021 Add better debug feature. Optimize code and examples to reduce RAM usage
  1.3.0   K.Hoang      18/05/2021 Update to match new ESP8266 core v3.0.0
  1.4.0   K.Hoang      01/06/2021 Add complex examples. Fix compiler errors due to conflict to some libraries.
  1.4.1   K.Hoang      22/11/2021 Tested with core v3.0.2. Add instructions in README.md
  1.5.0   K.Hoang      18/01/2022 Fix `multiple-definitions` linker error. Fix bug and add more accurate but shorter timer
  1.6.0   K.Hoang      13/02/2022 Add example to demo how to use one-shot ISR-based timers. Optimize code
  1.6.1   Nineday      29/12/2024 Switch time function from millis() to micros() for more accuracy.
*****************************************************************************************************************************/

#pragma once

#if !defined(ESP8266)
  #error This code is designed to run on ESP8266 and ESP8266-based boards! Please check your Tools->Board setting.
#endif

#ifndef ISR_TIMER_GENERIC_IMPL_H
#define ISR_TIMER_GENERIC_IMPL_H

//#include "ESP8266_ISR_Timer.h"
#include <string.h>

// Select time function:
static inline unsigned long elapsed()
{
  return micros();
}
// static inline unsigned long elapsed() 
// {
//   return millis();
// }


ESP8266_ISR_Timer::ESP8266_ISR_Timer()
  : numTimers (-1)
{
}

void IRAM_ATTR ESP8266_ISR_Timer::init() 
{
  unsigned long current_millis = micros();   //elapsed();

  for (uint8_t i = 0; i < MAX_NUMBER_TIMERS; i++) 
  {
    memset((void*) &timer[i], 0, sizeof (timer_t));
    timer[i].prev_millis = current_millis;
  }

  numTimers = 0;
}


void IRAM_ATTR ESP8266_ISR_Timer::run() 
{
  uint8_t i;
  unsigned long current_millis;

  // get current time
  current_millis = micros();   //elapsed();

  for (i = 0; i < MAX_NUMBER_TIMERS; i++) 
  {

    timer[i].toBeCalled = TIMER_DEFCALL_DONTRUN;

    // no callback == no timer, i.e. jump over empty slots
    if (timer[i].callback != NULL) 
    {
      // is it time to process this timer ?
      // see http://arduino.cc/forum/index.php/topic,124048.msg932592.html#msg932592

      if ((current_millis - timer[i].prev_millis) >= timer[i].delay) 
      {
        unsigned long skipTimes = (current_millis - timer[i].prev_millis) / timer[i].delay;
        // update time
        timer[i].prev_millis += timer[i].delay * skipTimes;

        // check if the timer callback has to be executed
        if (timer[i].enabled) 
        {
          // "run forever" timers must always be executed
          if (timer[i].maxNumRuns == TIMER_RUN_FOREVER) 
          {
            timer[i].toBeCalled = TIMER_DEFCALL_RUNONLY;
          }
          // other timers get executed the specified number of times
          else if (timer[i].numRuns < timer[i].maxNumRuns) 
          {
            timer[i].toBeCalled = TIMER_DEFCALL_RUNONLY;
            timer[i].numRuns++;

            // after the last run, delete the timer
            if (timer[i].numRuns >= timer[i].maxNumRuns) 
            {
              timer[i].toBeCalled = TIMER_DEFCALL_RUNANDDEL;
            }
          }
        }
      }
    }
  }

  for (i = 0; i < MAX_NUMBER_TIMERS; i++) 
  {
    if (timer[i].toBeCalled == TIMER_DEFCALL_DONTRUN)
      continue;

    if (timer[i].hasParam)
      (*(timer_callback_p)timer[i].callback)(timer[i].param);
    else
      (*(timer_callback)timer[i].callback)();

    if (timer[i].toBeCalled == TIMER_DEFCALL_RUNANDDEL)
      deleteTimer(i);
  }
}


// find the first available slot
// return -1 if none found
int8_t IRAM_ATTR ESP8266_ISR_Timer::findFirstFreeSlot() 
{
  // all slots are used
  if (numTimers >= MAX_NUMBER_TIMERS) 
{
    return -1;
  }

  // return the first slot with no callback (i.e. free)
  for (uint8_t i = 0; i < MAX_NUMBER_TIMERS; i++) 
{
    if (timer[i].callback == NULL) 
{
      return i;
    }
  }

  // no free slots found
  return -1;
}


int8_t IRAM_ATTR ESP8266_ISR_Timer::setupTimer(const unsigned long& d, void* f, void* p, bool h, const unsigned& n) 
{
  int freeTimer;

  if (numTimers < 0) 
  {
    init();
  }

  freeTimer = findFirstFreeSlot();
  
  if (freeTimer < 0) 
  {
    return -1;
  }

  if (f == NULL) 
  {
    return -1;
  }

  timer[freeTimer].delay = d;
  timer[freeTimer].callback = f;
  timer[freeTimer].param = p;
  timer[freeTimer].hasParam = h;
  timer[freeTimer].maxNumRuns = n;
  timer[freeTimer].enabled = true;
  timer[freeTimer].prev_millis = elapsed();

  numTimers++;

  return freeTimer;
}


int IRAM_ATTR ESP8266_ISR_Timer::setTimer(const unsigned long& d, const timer_callback& f, const unsigned& n) 
{
  return setupTimer(d, (void *)f, NULL, false, n);
}

int IRAM_ATTR ESP8266_ISR_Timer::setTimer(const unsigned long& d, const timer_callback_p& f, void* p, const unsigned& n) 
{
  return setupTimer(d, (void *)f, p, true, n);
}

int IRAM_ATTR ESP8266_ISR_Timer::setInterval(const unsigned long& d, const timer_callback& f) 
{
  return setupTimer(d, (void *)f, NULL, false, TIMER_RUN_FOREVER);
}

int IRAM_ATTR ESP8266_ISR_Timer::setInterval(const unsigned long& d, const timer_callback_p& f, void* p) 
{
  return setupTimer(d, (void *)f, p, true, TIMER_RUN_FOREVER);
}

int IRAM_ATTR ESP8266_ISR_Timer::setTimeout(const unsigned long& d, const timer_callback& f) 
{
  return setupTimer(d, (void *)f, NULL, false, TIMER_RUN_ONCE);
}

int IRAM_ATTR ESP8266_ISR_Timer::setTimeout(const unsigned long& d, const timer_callback_p& f, void* p) 
{
  return setupTimer(d, (void *)f, p, true, TIMER_RUN_ONCE);
}

bool IRAM_ATTR ESP8266_ISR_Timer::changeInterval(const unsigned& numTimer, const unsigned long& d) 
{
  if (numTimer >= MAX_NUMBER_TIMERS) 
  {
    return false;
  }

  // Updates interval of existing specified timer
  if (timer[numTimer].callback != NULL) 
  {
    timer[numTimer].delay = d;
    timer[numTimer].prev_millis = elapsed();
    return true;
  }
  // false return for non-used numTimer, no callback
  return false;
}

void IRAM_ATTR ESP8266_ISR_Timer::deleteTimer(const unsigned& timerId) 
{
  if (timerId >= MAX_NUMBER_TIMERS) 
  {
    return;
  }

  // nothing to delete if no timers are in use
  if (numTimers == 0) 
  {
    return;
  }

  // don't decrease the number of timers if the
  // specified slot is already empty
  if (timer[timerId].callback != NULL) 
  {
    memset((void*) &timer[timerId], 0, sizeof (timer_t));
    timer[timerId].prev_millis = elapsed();

    // update number of timers
    numTimers--;
  }
}


// function contributed by code@rowansimms.com
void IRAM_ATTR ESP8266_ISR_Timer::restartTimer(const unsigned& numTimer) 
{
  if (numTimer >= MAX_NUMBER_TIMERS) 
  {
    return;
  }

  timer[numTimer].prev_millis = elapsed();
}


bool IRAM_ATTR ESP8266_ISR_Timer::isEnabled(const unsigned& numTimer) 
{
  if (numTimer >= MAX_NUMBER_TIMERS) 
  {
    return false;
  }

  return timer[numTimer].enabled;
}


void IRAM_ATTR ESP8266_ISR_Timer::enable(const unsigned& numTimer) 
{
  if (numTimer >= MAX_NUMBER_TIMERS) 
  {
    return;
  }

  timer[numTimer].enabled = true;
}


void IRAM_ATTR ESP8266_ISR_Timer::disable(const unsigned& numTimer) 
{
  if (numTimer >= MAX_NUMBER_TIMERS) 
  {
    return;
  }

  timer[numTimer].enabled = false;
}

void IRAM_ATTR ESP8266_ISR_Timer::enableAll() 
{
  // Enable all timers with a callback assigned (used)
  for (uint8_t i = 0; i < MAX_NUMBER_TIMERS; i++) 
  {
    if (timer[i].callback != NULL && timer[i].numRuns == TIMER_RUN_FOREVER) 
    {
      timer[i].enabled = true;
    }
  }
}

void IRAM_ATTR ESP8266_ISR_Timer::disableAll() 
{
  // Disable all timers with a callback assigned (used)
  for (uint8_t i = 0; i < MAX_NUMBER_TIMERS; i++) 
  {
    if (timer[i].callback != NULL && timer[i].numRuns == TIMER_RUN_FOREVER) 
    {
      timer[i].enabled = false;
    }
  }
}

void IRAM_ATTR ESP8266_ISR_Timer::toggle(const unsigned& numTimer) 
{
  if (numTimer >= MAX_NUMBER_TIMERS) 
  {
    return;
  }

  timer[numTimer].enabled = !timer[numTimer].enabled;
}


int8_t IRAM_ATTR ESP8266_ISR_Timer::getNumTimers() 
{
  return numTimers;
}

#endif    // ISR_TIMER_GENERIC_IMPL_H
