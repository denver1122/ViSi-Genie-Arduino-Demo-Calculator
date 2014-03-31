/*
 * calculator.ino:
 *      Original Version by Gordon Henderson, February 2013, <projects@drogon.net>
 *      Copyright (c) 2012-2013 4D Systems PTY Ltd, Sydney, Australia
 *      Updated March 2014 by 4D Systems Pty Ltd
 ***********************************************************************
 * This file is part of genieArduino:
 *    genieArduino is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Lesser General Public License as
 *    published by the Free Software Foundation, either version 3 of the
 *    License, or (at your option) any later version.
 *
 *    genieArduino is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public
 *    License along with genieArduino.
 *    If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************
 */

// This demo is a calculator. Display interface talks to the Arduino, and arduino does
// all the calculations. Uses Serial0 to talk to the Display.

#include <genieArduino.h>  // MODIFIED new genieArduino library

#include <stdio.h>
#include <stdint.h>
#include <ctype.h>

#ifndef	TRUE
#define	TRUE	(1==1)
#define	FALSE	(!TRUE)
#endif

// Calculator globals

double acc ;
double memory ;
double display = 0.0 ;
int    lastOperator ;
int    errorCondition  ;

Genie genie;
#define RESETLINE 4  // Change this if you are not using an Arduino Adaptor Shield Version 2 (see code below)

/*
 *********************************************************************************
 * main:
 *	Run our little demo
 *********************************************************************************
 */

void setup ()
{
  pinMode      (13, OUTPUT); // For debug purposes, LED on D13 is set if something 'wrong' happens
  digitalWrite (13, 0);      // Turn off LED to start with

  Serial.begin(115200);  // Serial0 @ 115200 Baud
  genie.Begin(Serial);   // Use Serial0 for talking to the Genie Library, and to the 4D Systems display

  genie.AttachEventHandler(myGenieEventHandler); // Attach the user function Event Handler for processing events

  // Reset the Display (change D4 to D2 if you have original 4D Arduino Adaptor)
  // If NOT using a 4D Arduino Adaptor, digitalWrites must be reversed as Display Reset is Active Low, and
  // the 4D Arduino Adaptors invert this signal so must be Active High.
  pinMode(RESETLINE, OUTPUT);  // Set D4 on Arduino to Output (4D Arduino Adaptor V2 - Display Reset)
  digitalWrite(RESETLINE, 1);  // Reset the Display via D4
  delay(100);
  digitalWrite(RESETLINE, 0);  // unReset the Display via D4

  delay (3500); //let the display start up after the reset (This is important)

  genie.WriteObject (GENIE_OBJ_FORM, 0, 0); // Select form 0 (the calculator)

  calculatorKey ('a') ;	// Clear the calculator
}

void loop (void)
{
  // Big loop - just wait for events from the display now

  genie.DoEvents();

  // Do other things here if required
}

/*
 * updateDisplay:
 *	Do just that.
 *********************************************************************************
 */

void updateDisplay(void)
{
  char *p ;
  char buf [32] ;

  if (errorCondition)
    sprintf (buf, "%s", "ERROR") ;
  else
    dtostrf (display, 11, 5, buf) ;

  genie.WriteStr (0, buf) ;	// Text box number 0

  sprintf (buf, "%c %c",
           memory != 0.0 ? 'M' : ' ',
           lastOperator == 0 ? ' ' : lastOperator) ;

  genie.WriteStr (1, buf) ;	// Text box number 1
}


/*
 * processOperator:
 *	Take our operator and apply it to the accumulator and display registers
 *********************************************************************************
 */

static void processOperator(int opsKey)
{
  /**/ if (opsKey == '+')
    acc += display ;
  else if (opsKey == '-')
    acc -= display ;
  else if (opsKey == '*')
    acc *= display ;
  else if (opsKey == '/')
  {
    if (display == 0.0)
      errorCondition = TRUE ;
    acc /= display ;
  }
  display = acc ;

  if (fabs (display) > 999999999.0)
    errorCondition = TRUE ;

  if (fabs (display) < 0.00000001)
    display = 0.0 ;
}


/*
 * calculatorKey:
 *	We've pressed a key on our calculator.
 *********************************************************************************
 */

void calculatorKey(int key)
{
  static int gotDecimal     = FALSE ;
  static int startNewNumber = TRUE ;
  static double multiplier  = 1.0 ;
  float digit ;

  // Eeeeee...

  if (errorCondition && key != 'a')
    return ;

  if (isdigit(key))
  {
    if (startNewNumber)
    {
      startNewNumber = FALSE ;
      multiplier     = 1.0 ;
      display        = 0.0 ;
    }

    digit = (double)(key - '0') ;
    if (multiplier == 1.0)
      display = display * 10 + (double)digit ;
    else
    {
      display     = display + (multiplier * digit) ;
      multiplier /= 10.0 ;
    }
    updateDisplay () ;
    return ;
  }

  switch (key)
  {
    case 'a':			// AC - All Clear
      lastOperator   = 0 ;
      acc            = 0.0 ;
      memory         = 0.0 ;
      display        = 0.0 ;
      gotDecimal     = FALSE ;
      errorCondition = FALSE ;
      startNewNumber = TRUE ;
      break ;

    case 'c':			// Clear entry or operator
      if (lastOperator != 0)
        lastOperator = 0 ;
      else
      {
        display        = 0.0 ;
        gotDecimal     = FALSE ;
        startNewNumber = TRUE ;
      }
      break ;

      // Memory keys

    case 128:		// Mem Store
      memory = display ;
      break ;

    case 129:		// M+
      memory += display ;
      break ;

    case 130:		// M-
      memory -= display ;
      break ;

    case 131:		// MR - Memory Recall
      display = memory ;
      break ;

    case 132:		// MC - Memory Clear
      memory = 0.0 ;
      break ;

      // Other functions

    case 140:		// +/-
      display = -display ;
      break ;

    case 's':	// Square root
      if (display < 0.0)
        errorCondition = TRUE ;
      else
      {
        display        = sqrt (display) ;
        gotDecimal     = FALSE ;
        startNewNumber = TRUE ;
      }
      break ;

      // Operators

    case '+': case '-': case '*': case '/':
      if (lastOperator == 0)
        acc = display ;
      else
        processOperator (lastOperator) ;
      lastOperator    = key ;
      startNewNumber = TRUE ;
      gotDecimal     = FALSE ;
      break ;

    case '=':
      if (lastOperator != 0)
        processOperator (lastOperator) ;
      lastOperator    = 0 ;
      gotDecimal     = FALSE ;
      startNewNumber = TRUE ;
      acc            = 0.0 ;
      break ;

    case '.':
      if (!gotDecimal)
      {
        if (startNewNumber)
        {
          startNewNumber = FALSE ;
          display        = 0.0 ;
        }
        multiplier = 0.1 ;
        gotDecimal = TRUE ;
        break ;
      }

    default:
      printf ("*** Unknown key from display: 0x%02X\n", key) ;
      break ;
  }

  updateDisplay() ;
}


/*
 * myGenieEventHandler:
 *	Take a reply off the display and call the appropriate handler for it.
 *********************************************************************************
 */

void myGenieEventHandler (void)
{
  int keyboardValue;

  genieFrame Event;
  genie.DequeueEvent(&Event); // Remove this event from the queue

  if (Event.reportObject.cmd != GENIE_REPORT_EVENT) // If this event is NOT a Reported Message
  {
    digitalWrite (13, 1); // Set the LED to show a different message has been received (invalid to this demo)
    return ; // Leave the handler
  }

  if (Event.reportObject.object == GENIE_OBJ_KEYBOARD) // If this event is from a Keyboard
  {
    if (Event.reportObject.index == 0)	// If from Keyboard0
    {
      keyboardValue = genie.GetEventData(&Event); // Get data from Keyboard0
      calculatorKey(keyboardValue); // pass data to the calculatorKey function for processing
    }
    else
      digitalWrite (13, 1) ; // If this event is from a different keyboard, set LED as its invalid for this demo
  }
  else
    digitalWrite (13, 1); // If this event is from a different object, set LED as its invalid for this demo
}

