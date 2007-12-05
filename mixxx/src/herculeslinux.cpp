/***************************************************************************
                          herculeslinux.cpp  -  description
                             -------------------
    begin                : Tue Feb 22 2005
    copyright            : (C) 2005 by Tue Haste Andersen
    email                : haste@diku.dk
***************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

/*
 * =========Version History=============
 * Version 1.6.0 --- Dec 5 2007 --- Garth Dahlstrom <ironstorm@users.sf.net>
 * - Developed on Hercule DJ Console MK2 - MK1 may also work, untested
 * - Fixed control scale to be 0.0 to 4.0 on bass, mid, treb, etc
 * - Fixed pitch knob to be from -1.0 to 1.0
 * - Fixed volume sliders to be from 0.0 to 1.0 
 * - Fixed crossfader to be -1.0 to 1.0
 * - Fixed Play buttons to send the NOT (!) value whatever the current playback status is
 * - Button LEDs light up when pressed
 * - Thanks to Thomas' patch we have a working headphone switch on the Mk2
 *
 * - REMOVED legacy dev/file based support used in Mixxx 1.5.0 (Alberts MP3 Control won't work unless Mel can get it going with libdjconsole) 
 *	The last straw why this code has now been removed is with the recent engine changes to the meanings of all of the control values, 
 *	this block of code would need a large update and a bit of testing to get everything working in the manner it used to...
 *
 * - TODO: fix the JogDials to do something like a scratch (they do a temporary pitch bend now :\)
 * - TODO: reset m_iPitchOffsetLeft or m_iPitchOffsetRight value to -9999 when mouse/keyboard adjusts pitch slider (see PitchChange method header)
 */

#define __THOMAS_HERC__

#include "herculeslinux.h"
#include <string.h>
#include <QtDebug>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include "controlobject.h"
#include "controlobjectthread.h"
#include "mathstuff.h"
#include "rotary.h"
//Added by qt3to4:
#include <Q3ValueList> // used by the old herc code
#include <QTimer>

#include <QDebug>

#include <math.h>

//#define __HERCULES_STUB__ // Define __HERCULES_STUB__ disable Herc USB mode to be able to test Hercules in MIDI mode.

#ifndef __HERCULES_STUB__
// If DJCONSOLE=1 -> use libDJConsole implementation
// If DJCONSOLE_LEGACY_CODE -> use legacy devfile based implementation
// Otherwise stub-out HerculesLinux object
 #ifndef __LIBDJCONSOLE__
  #ifdef __DJCONSOLE_LEGACY_CODE__
   // include devfile based djconsole legacy code
  #else 
   #define __HERCULES_STUB__ // stub is implied because neither libDJConsole nor Legacy code implementations are specified.
  #endif
 #endif
#endif

#ifdef __HERCULES_STUB__
/************** Stub ***********/
// Stub of HerculesLinux object.
HerculesLinux::HerculesLinux() : Hercules() {}
HerculesLinux::~HerculesLinux() {}
void HerculesLinux::closedev() {}
void HerculesLinux::run() {}
bool HerculesLinux::opendev(){ return 1; }
int HerculesLinux::opendev(int iId) { return 1; }
void HerculesLinux::getNextEvent(){}
void HerculesLinux::led_write(int iLed, bool bOn){}
void HerculesLinux::selectMapping(QString qMapping) {}
double HerculesLinux::PitchChange(const QString ControlSide, const int ev_value, int &m_iPitchPrevious, int &m_iPitchOffset) { return 0; }
/************** End Stub ***********/
#else //__HERCULES_STUB__

#ifdef __LIBDJCONSOLE__
static void console_event(void * c, int code, int value)
{
    HerculesLinux * f=(HerculesLinux *)c;
    f->consoleEvent(code, value);
}

HerculesLinux::HerculesLinux() : Hercules()
{
    djc = 0; // set to zero to force detection on the first run through.
    m_iPitchLeft = -1;
    m_iPitchRight = -1;

    qDebug("HerculesLinux: Constructor called");

    // m_iFd = -1; // still needed?
    m_iId = -1;
    m_iJogLeft = 0.;
    m_iJogRight = 0.;

    m_dJogLeftOld = -1;
    m_dJogRightOld = -1;

    m_bHeadphoneLeft = false;
    m_bHeadphoneRight = false;

#ifdef __THOMAS_HERC__
    m_iHerculesHeadphonesSelection = 1;
#endif
}

HerculesLinux::~HerculesLinux() {
}
void HerculesLinux::closedev() {
}

void HerculesLinux::run() 
{
#ifdef __THOMAS_HERC__
	double l;
	double r;
	bool leftJogProcessing = false;
	bool rightJogProcessing = false;
	m_pRotaryLeft->setFilterLength(4);
	m_pRotaryRight->setFilterLength(4);
	m_pRotaryLeft->setCalibration(64);
	m_pRotaryRight->setCalibration(64);
	djc->Leds.setBit(LEFT_FX, false);
	djc->Leds.setBit(LEFT_FX_CUE, false);
	djc->Leds.setBit(LEFT_LOOP, true);
	djc->Leds.setBit(RIGHT_FX, false);
	djc->Leds.setBit(RIGHT_FX_CUE, false);
	djc->Leds.setBit(RIGHT_LOOP, true);
	while( 1 )
	{
		if (m_iJogLeft != 0)
		{
			l = m_pRotaryLeft->fillBuffer(m_iJogLeft);
			m_iJogLeft = 0;
			leftJogProcessing = true;
		}
		else 
		{
			l = m_pRotaryLeft->filter(m_iJogLeft);
		}
		if (m_iJogRight != 0)
		{
			r = m_pRotaryRight->fillBuffer(m_iJogRight);
			m_iJogRight = 0;
			rightJogProcessing = true;
		}
		else 
		{
			r = m_pRotaryRight->filter(m_iJogRight);
		}
		if ( l != 0 || leftJogProcessing)
		{
			//qDebug("sendEvent(%e, m_pControlObjectLeftJog)",l);
			sendEvent(l, m_pControlObjectLeftJog);
			if ( l == 0 ) leftJogProcessing = false;
		}
		if ( r != 0 || rightJogProcessing)
		{
			//qDebug("sendEvent(%e, m_pControlObjectRightJog)",r);
			sendEvent(r, m_pControlObjectRightJog);
			if ( r == 0 ) rightJogProcessing = false;
		}
		msleep (64);
	}
#endif // __THOMAS_HERC__
}

bool HerculesLinux::opendev()
{
    qDebug("Starting Hercules DJ Console detection");
    if (djc == 0) {
        djc = new DJConsole();
        if(djc && djc->detected()) {
            qDebug("A Hercules DJ Console was detected.");
        } else {
            qDebug("Sorry, no love.");
        }

        djc->loadData();
#ifdef __THOMAS_HERC__
        start();
        m_pControlObjectLeftBtnCueAndStop = ControlObject::getControl(ConfigKey("[Channel1]","cue_gotoandstop"));
        m_pControlObjectRightBtnCueAndStop = ControlObject::getControl(ConfigKey("[Channel2]","cue_gotoandstop"));
#endif
        djc->setCallback(console_event, this);

        return djc->ready();

    } else {
        qDebug("Already completed detection.");
        return 1;
    }
}

int HerculesLinux::opendev(int iId)
{
    return opendev();
}

void HerculesLinux::consoleEvent(int first, int second) {
    if (second == 0) {
        djc->Leds.setBit(first, false);
        return;
    }

    //qDebug("x Button %i = %i", first, second);
    if(first != 0) {
        bool ledIsOn = (second == 0 ? false : true);
        int led = 0;
#ifdef __THOMAS_HERC__
	int iDiff = 0;
#endif
        switch(first) {
        case LEFT_PLAY:
        case LEFT_CUE:
        case LEFT_MASTER_TEMPO:
        case LEFT_AUTO_BEAT:
        case LEFT_MONITOR:
        case RIGHT_PLAY:
        case RIGHT_CUE:
        case RIGHT_MASTER_TEMPO:
        case RIGHT_AUTO_BEAT:
        case RIGHT_MONITOR:
            led = first;
            break;
#ifndef __THOMAS_HERC__  // Old behaviour - LEDs only
        case LEFT_1:  led = LEFT_FX;      break;
        case LEFT_2:  led = LEFT_FX_CUE;  break;
        case LEFT_3:  led = LEFT_LOOP;    break;
        case RIGHT_1: led = RIGHT_FX;     break;
        case RIGHT_2: led = RIGHT_FX_CUE; break;
        case RIGHT_3: led = RIGHT_LOOP;   break;
#else
		case LEFT_1:  
			m_pRotaryLeft->setCalibration(512);
		        djc->Leds.setBit(LEFT_FX, true);
		        djc->Leds.setBit(LEFT_FX_CUE, false);
		        djc->Leds.setBit(LEFT_LOOP, false);
			break;
		case LEFT_2:
			m_pRotaryLeft->setCalibration(256);
		        djc->Leds.setBit(LEFT_FX, false);
		        djc->Leds.setBit(LEFT_FX_CUE, true);
		        djc->Leds.setBit(LEFT_LOOP, false);
			break;
		case LEFT_3:  
			m_pRotaryLeft->setCalibration(64);
		        djc->Leds.setBit(LEFT_FX, false);
		        djc->Leds.setBit(LEFT_FX_CUE, false);
		        djc->Leds.setBit(LEFT_LOOP, true);
			break;
		case RIGHT_1: 
			m_pRotaryRight->setCalibration(512);
		        djc->Leds.setBit(RIGHT_FX, true);
		        djc->Leds.setBit(RIGHT_FX_CUE, false);
		        djc->Leds.setBit(RIGHT_LOOP, false);
			break;
		case RIGHT_2: 
			m_pRotaryRight->setCalibration(256);
		        djc->Leds.setBit(RIGHT_FX, false);
		        djc->Leds.setBit(RIGHT_FX_CUE, true);
		        djc->Leds.setBit(RIGHT_LOOP, false);
			break;
		case RIGHT_3: 	
			m_pRotaryRight->setCalibration(64);
		        djc->Leds.setBit(RIGHT_FX, false);
		        djc->Leds.setBit(RIGHT_FX_CUE, false);
		        djc->Leds.setBit(RIGHT_LOOP, true);
            break;
#endif __THOMAS_HERC__  
		default: break;
        }

	// GED's magic formula -- no longer used.
	// double v = ((second+1)/(4.- ((second>((7/8.)*256))*((second-((7/8.)*256))*1/16.)))); 

	// Albert's http://zunzun.com/ site saves the day by solving our data points to this new magical formula...
	double magic = (0.733835252488 * tan((0.00863901501308 * second) - 4.00513109039)) + 0.887988233294; 

	// double divisor = 2.;
	double divisor = 256.;
	double d1 = divisor-1; 
	double d2 = (divisor/2)-1;
	double d4 = (divisor/4)-1;
	
	// qDebug() << "second: " << second << "magic: " << magic << " v: " << v << " sd1:" << QString::number(second/d1) << " sd2:" << QString::number(second/d2) <<" sd4:" << QString::number(second/d4);

	// qDebug() << "m_pControlObjectLeftPitch:" << QString::number(m_pControlObjectLeftPitch->get()) << "m_pControlObjectRightPitch:" << QString::number(m_pControlObjectRightPitch->get());

        switch(first) {
        case LEFT_VOL: sendEvent(second/d1, m_pControlObjectLeftVolume); break;
        case RIGHT_VOL: sendEvent(second/d1, m_pControlObjectRightVolume); break;
        case LEFT_PLAY: sendButtonEvent(!m_pControlObjectLeftBtnPlay->get(), m_pControlObjectLeftBtnPlay); break;
        case RIGHT_PLAY: sendButtonEvent(!m_pControlObjectRightBtnPlay->get(), m_pControlObjectRightBtnPlay); break;
        case XFADER: sendEvent((second-d2)/d2, m_pControlObjectCrossfade); break;
        case LEFT_PITCH_DOWN: sendButtonEvent(true, m_pControlObjectLeftBtnPitchBendMinus); break;
        case LEFT_PITCH_UP: sendButtonEvent(true, m_pControlObjectLeftBtnPitchBendPlus); break;
        case RIGHT_PITCH_DOWN: sendButtonEvent(true, m_pControlObjectRightBtnPitchBendMinus); break;
        case RIGHT_PITCH_UP: sendButtonEvent(true, m_pControlObjectRightBtnPitchBendPlus); break;
        case LEFT_SKIP_BACK: sendButtonEvent(true, m_pControlObjectLeftBtnTrackPrev); break;
        case LEFT_SKIP_FORWARD: sendButtonEvent(true, m_pControlObjectLeftBtnTrackNext); break;
        case RIGHT_SKIP_BACK: sendButtonEvent(true, m_pControlObjectRightBtnTrackPrev); break;
        case RIGHT_SKIP_FORWARD: sendButtonEvent(true, m_pControlObjectRightBtnTrackNext); break;
        case RIGHT_HIGH: sendEvent(magic, m_pControlObjectRightTreble); break;
        case RIGHT_MID: sendEvent(magic, m_pControlObjectRightMiddle); break;
        case RIGHT_BASS: sendEvent(magic, m_pControlObjectRightBass); break;
        case LEFT_HIGH: sendEvent(magic, m_pControlObjectLeftTreble); break;
        case LEFT_MID: sendEvent(magic, m_pControlObjectLeftMiddle); break;
        case LEFT_BASS:	sendEvent(magic, m_pControlObjectLeftBass); break;

#ifndef __THOMAS_HERC__  // Old behaviour + Headphone Deck Pseudocode
        case LEFT_CUE: sendButtonEvent(true, m_pControlObjectLeftBtnCue); break;
        case RIGHT_CUE: sendButtonEvent(true, m_pControlObjectRightBtnCue); break;
        case LEFT_MASTER_TEMPO: sendEvent(0, m_pControlObjectLeftBtnMasterTempo); m_bMasterTempoLeft = !m_bMasterTempoLeft; break;
        case RIGHT_MASTER_TEMPO: sendEvent(0, m_pControlObjectRightBtnMasterTempo); m_bMasterTempoRight = !m_bMasterTempoRight; break;

        case RIGHT_MONITOR: sendButtonEvent(true, m_pControlObjectRightBtnHeadphone); m_bHeadphoneRight = !m_bHeadphoneRight; break;
        case LEFT_MONITOR: sendButtonEvent(true, m_pControlObjectLeftBtnHeadphone); m_bHeadphoneLeft = !m_bHeadphoneLeft; break;

/*
        case HEADPHONE_DECK_A:
                qDebug("Deck A");
                if (m_bHeadphoneRight) {
                   sendButtonEvent(true, m_pControlObjectRightBtnHeadphone); m_bHeadphoneRight = !m_bHeadphoneRight;
                }
                if (!m_bHeadphoneLeft) {
                   sendButtonEvent(true, m_pControlObjectLeftBtnHeadphone); m_bHeadphoneLeft = !m_bHeadphoneLeft;
                }
        break;

        case HEADPHONE_DECK_B:

                qDebug("Deck B");
                if (!m_bHeadphoneRight) {
                   sendButtonEvent(true, m_pControlObjectRightBtnHeadphone); m_bHeadphoneRight = !m_bHeadphoneRight;
                }
                if (m_bHeadphoneLeft) {
                   sendButtonEvent(true, m_pControlObjectLeftBtnHeadphone); m_bHeadphoneLeft = !m_bHeadphoneLeft ;
                }

        break;

        case HEADPHONE_MIX:
                qDebug("Deck MIX");
                if (!m_bHeadphoneRight) {
                   sendButtonEvent(true, m_pControlObjectRightBtnHeadphone); m_bHeadphoneRight = !m_bHeadphoneRight;
                }
                if (!m_bHeadphoneLeft) {
                   sendButtonEvent(true, m_pControlObjectLeftBtnHeadphone); m_bHeadphoneLeft = !m_bHeadphoneLeft;
                }
        break;
 */
#else
		case LEFT_CUE: if (m_pControlObjectLeftBtnPlayProxy->get())
			{
				/* CUE do GotoAndStop */
				sendButtonEvent(true, m_pControlObjectLeftBtnCueAndStop); 
			}
			else
			{
				sendButtonEvent(true, m_pControlObjectLeftBtnCue); 
			}
			break;
		case RIGHT_CUE: 
			if (m_pControlObjectRightBtnPlayProxy->get())
			{
				/* CUE do GotoAndStop */
				sendButtonEvent(true, m_pControlObjectRightBtnCueAndStop); 
			}
			else
			{
				sendButtonEvent(true, m_pControlObjectRightBtnCue); 
			}
			break;
		case LEFT_MASTER_TEMPO: 
			sendEvent(0, m_pControlObjectLeftBtnMasterTempo); 
			m_bMasterTempoLeft = !m_bMasterTempoLeft; 
			break;
		case RIGHT_MASTER_TEMPO: 
			sendEvent(0, m_pControlObjectRightBtnMasterTempo); 
			m_bMasterTempoRight = !m_bMasterTempoRight; 
			break;
		case RIGHT_MONITOR: 
			sendButtonEvent(true, m_pControlObjectRightBtnHeadphone); 
			m_bHeadphoneRight = !m_bHeadphoneRight; 
			break;
		case LEFT_MONITOR: 
			sendButtonEvent(true, m_pControlObjectLeftBtnHeadphone); 
			m_bHeadphoneLeft = !m_bHeadphoneLeft; 
			break;
		/* for the headphone select if have mesured something like this on my hercules mk2 
		*
		*	from state	to state	value(s)
		*	split		mix		first=102, second=8
		*	mix		split		first=103, second=4 most significant
		*	mix		split		first=100, second=1
		*	mix		split		first=101, second=2
		*	mix		deck b		first=101, second=2
		*	deck b		mix		first=102, second=8
		*	deck b		deck a		first=100, second=1
		*	deck a		deck b		first=101, second=2
		*
		*	you will see only one unique value: first=103,seconnd=4
		*	so lets try what we learned about: (sorry, we realy need a var for tracking this)
		*/
		case 103:
			if (second == 4)
			{
				m_iHerculesHeadphonesSelection = kiHerculesHeadphoneSplit;
				qDebug("Deck SPLIT (mute both)");
				if (m_bHeadphoneRight) 
				{
					sendButtonEvent(true, m_pControlObjectRightBtnHeadphone); m_bHeadphoneRight = !m_bHeadphoneRight;
				}
				if (m_bHeadphoneLeft) 
				{
					sendButtonEvent(true, m_pControlObjectLeftBtnHeadphone); m_bHeadphoneLeft = !m_bHeadphoneLeft;
				}
			}
			break;
		case 102:
			if (second == 8)
			{
				m_iHerculesHeadphonesSelection = kiHerculesHeadphoneMix;
				qDebug("Deck MIX");
				if (!m_bHeadphoneRight) 
				{
					sendButtonEvent(true, m_pControlObjectRightBtnHeadphone); m_bHeadphoneRight = !m_bHeadphoneRight;
				}
				if (!m_bHeadphoneLeft) 
				{
					sendButtonEvent(true, m_pControlObjectLeftBtnHeadphone); m_bHeadphoneLeft = !m_bHeadphoneLeft;
				}
			}
			break;
		case 101:
			if (second == 2 && ( m_iHerculesHeadphonesSelection == kiHerculesHeadphoneDeckA || m_iHerculesHeadphonesSelection == kiHerculesHeadphoneMix ) )
			{
				/* now we shouldn't get here if 101/2 follows straight to 103/4 */
				m_iHerculesHeadphonesSelection = kiHerculesHeadphoneDeckB;
				qDebug("Deck B");
				if (!m_bHeadphoneRight) 
				{
					sendButtonEvent(true, m_pControlObjectRightBtnHeadphone); m_bHeadphoneRight = !m_bHeadphoneRight;
				}
				if (m_bHeadphoneLeft) 
				{
					sendButtonEvent(true, m_pControlObjectLeftBtnHeadphone); m_bHeadphoneLeft = !m_bHeadphoneLeft ;
				}
			}
			break;
		case 100:
			if (second == 1 && m_iHerculesHeadphonesSelection == kiHerculesHeadphoneDeckB )
			{
				m_iHerculesHeadphonesSelection = kiHerculesHeadphoneDeckA;
				qDebug("Deck A");
				if (m_bHeadphoneRight) 
				{
					sendButtonEvent(true, m_pControlObjectRightBtnHeadphone); m_bHeadphoneRight = !m_bHeadphoneRight;
				}
				if (!m_bHeadphoneLeft) 
				{
					sendButtonEvent(true, m_pControlObjectLeftBtnHeadphone); m_bHeadphoneLeft = !m_bHeadphoneLeft;
				}
			}
			break;
		case LEFT_JOG:
			iDiff = 0;
			if (m_dJogLeftOld>=0)
			{
				iDiff = second-m_dJogLeftOld;
			}
			if (iDiff<-200)
			{
				iDiff += 256;
			}
			else if (iDiff>200)
			{
				iDiff -= 256;
			}
			m_dJogLeftOld = second;
			m_iJogLeft += (double)iDiff; /* here goes the magic */
			break;
		case RIGHT_JOG:
			iDiff = 0;
			if (m_dJogRightOld>=0)
			{
				iDiff = second-m_dJogRightOld;
			}
			if (iDiff<-200)
			{
				iDiff += 256;
			}
			else if (iDiff>200)
			{
				iDiff -= 256;
			}
			m_dJogRightOld = second;
			m_iJogRight += (double)iDiff;
			break;
#endif // __THOMAS_HERC__

        case LEFT_PITCH: sendEvent(PitchChange("Left", second, m_iPitchLeft, m_iPitchOffsetLeft), m_pControlObjectLeftPitch); break;
        case RIGHT_PITCH: sendEvent(PitchChange("Right", second, m_iPitchRight, m_iPitchOffsetRight), m_pControlObjectRightPitch); break;

        case LEFT_AUTO_BEAT: sendButtonEvent(false, m_pControlObjectLeftBtnAutobeat); break;
        case RIGHT_AUTO_BEAT: sendButtonEvent(false, m_pControlObjectRightBtnAutobeat); break;

        default:
            qDebug("Button %i = %i", first, second);
            break;
        }

        if(led != 0)
            djc->Leds.setBit(led, ledIsOn);
    }
}

void HerculesLinux::getNextEvent(){
}
void HerculesLinux::led_write(int iLed, bool bOn){
}
void HerculesLinux::selectMapping(QString qMapping) {
}

double HerculesLinux::PitchChange(const QString ControlSide, const int ev_value, int &m_iPitchPrevious, int &m_iPitchOffset) {
    // Handle the initial event from the Hercules and set pitch to default of 0% change
    if (m_iPitchPrevious < 0) {
        m_iPitchOffset = ev_value;
        m_iPitchPrevious = 64;
        return m_iPitchPrevious;
    }

    int delta = ev_value - m_iPitchOffset;
    if (delta >= 240) {
        delta = (255 - delta) * -1;
    }
    if (delta <= -240) {
        delta = 255 + delta;
    }
    m_iPitchOffset = ev_value;

#ifdef __THOMAS_HERC__
    int pitchAdjustStep = delta; // * 3; 
#else
    int pitchAdjustStep = delta * 3;
#endif

    if ((pitchAdjustStep > 0 && m_iPitchPrevious+pitchAdjustStep < 128) || (pitchAdjustStep < 0 && m_iPitchPrevious+pitchAdjustStep > 0)) {
        m_iPitchPrevious = m_iPitchPrevious+pitchAdjustStep;
    } else if (pitchAdjustStep > 0) {
        m_iPitchPrevious = 127;
    } else if (pitchAdjustStep < 0) {
        m_iPitchPrevious = 0;
    }

    // qDebug() << "PitchChange [" << ControlSide << "] PitchAdjust" << pitchAdjustStep <<"-> new Pitch:" << m_iPitchPrevious << " NewRangeAdjustedPitch:" <<  QString::number(((m_iPitchPrevious+1) - 64)/64.);

    // old range was 0..127
    // new range is -1.0 to 1.0
    return ((m_iPitchPrevious+1) - 64)/64.;
}

#endif

#endif //__HERCULES_STUB__
