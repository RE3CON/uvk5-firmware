/* Copyright 2023 Dual Tachyon
 * https://github.com/DualTachyon
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */

#include <string.h>

#include "app/scanner.h"
#ifdef ENABLE_FMRADIO
	#include "app/fm.h"
#endif
#include "bitmaps.h"
#include "driver/keyboard.h"
#include "driver/st7565.h"
#include "external/printf/printf.h"
#include "functions.h"
#include "helper/battery.h"
#include "misc.h"
#include "settings.h"
#include "ui/helper.h"
#include "ui/ui.h"
#include "ui/status.h"

void UI_DisplayStatus(const bool test_display)
{
	uint8_t     *line = gStatusLine;
	unsigned int x    = 0;
	unsigned int x1   = 0;
	
	gUpdateStatus = false;
	
	memset(gStatusLine, 0, sizeof(gStatusLine));

	// **************

	// POWER-SAVE indicator
	if (gCurrentFunction == FUNCTION_TRANSMIT)
	{
		memmove(line + x, BITMAP_TX, sizeof(BITMAP_TX));
		x1 = x + sizeof(BITMAP_TX);
	}
	else
	if (gCurrentFunction == FUNCTION_RECEIVE ||
	    gCurrentFunction == FUNCTION_MONITOR ||
	    gCurrentFunction == FUNCTION_INCOMING)
	{
		memmove(line + x, BITMAP_RX, sizeof(BITMAP_RX));
		x1 = x + sizeof(BITMAP_RX);
	}
	else
	if (gCurrentFunction == FUNCTION_POWER_SAVE || test_display)
	{
		memmove(line + x, BITMAP_POWERSAVE, sizeof(BITMAP_POWERSAVE));
		x1 = x + sizeof(BITMAP_POWERSAVE);
	}
	x += sizeof(BITMAP_POWERSAVE);

	#ifdef ENABLE_NOAA
		// NOASS SCAN indicator
		if (gIsNoaaMode || test_display)
		{
			memmove(line + x, BITMAP_NOAA, sizeof(BITMAP_NOAA));
			x1 = x + sizeof(BITMAP_NOAA);
		}
		x += sizeof(BITMAP_NOAA);
	#else
		// hmmm, what to put in it's place
	#endif
	
	if (gSetting_KILLED)
	{
		memset(line + x, 0xFF, 10);
		x1 = x + 10;
	}
	else
	#ifdef ENABLE_FMRADIO
		// FM indicator
		if (gFmRadioMode || test_display)
		{
			memmove(line + x, BITMAP_FM, sizeof(BITMAP_FM));
			x1 = x + sizeof(BITMAP_FM);
		}
		else
	#endif
		// SCAN indicator
		if (gScanStateDir != SCAN_OFF || gScreenToDisplay == DISPLAY_SCANNER || test_display)
		{
			if (gNextMrChannel <= MR_CHANNEL_LAST)
			{	// channel mode
				if (gEeprom.SCAN_LIST_DEFAULT == 0)
					UI_PrintStringSmallBuffer("1", line + x);
				else
				if (gEeprom.SCAN_LIST_DEFAULT == 1)
					UI_PrintStringSmallBuffer("2", line + x);
				else
				if (gEeprom.SCAN_LIST_DEFAULT == 2)
					UI_PrintStringSmallBuffer("*", line + x);
			}
			else
			{	// frequency mode
				UI_PrintStringSmallBuffer("S", line + x);
			}
			x1 = x + 7;
		}
	x += 7;  // font character width

	#ifdef ENABLE_VOICE
		// VOICE indicator
		if (gEeprom.VOICE_PROMPT != VOICE_PROMPT_OFF || test_display)
		{
			memmove(line + x, BITMAP_VoicePrompt, sizeof(BITMAP_VoicePrompt));
			x1 = x + sizeof(BITMAP_VoicePrompt);
		}
		x += sizeof(BITMAP_VoicePrompt);
	#else
		// hmmm, what to put in it's place
	#endif

	// DUAL-WATCH indicator
	if (gEeprom.DUAL_WATCH != DUAL_WATCH_OFF || test_display)
	{
		if (gDualWatchActive || test_display)
			memmove(line + x, BITMAP_TDR1, sizeof(BITMAP_TDR1));
		else
			memmove(line + x, BITMAP_TDR2, sizeof(BITMAP_TDR2));
	}
	x += sizeof(BITMAP_TDR1);

	// CROSS-VFO indicator
	if (gEeprom.CROSS_BAND_RX_TX != CROSS_BAND_OFF || test_display)
	{
		memmove(line + x, BITMAP_XB, sizeof(BITMAP_XB));
		x1 = x + sizeof(BITMAP_XB);
	}
	x += sizeof(BITMAP_XB);
	
	#ifdef ENABLE_VOX
		// VOX indicator
		if (gEeprom.VOX_SWITCH || test_display)
		{
			memmove(line + x, BITMAP_VOX, sizeof(BITMAP_VOX));
			x1 = x + sizeof(BITMAP_VOX);
		}
		x += sizeof(BITMAP_VOX);
	#endif

	// KEY-LOCK indicator
	if (gEeprom.KEY_LOCK || test_display)
	{
		memmove(line + x, BITMAP_KeyLock, sizeof(BITMAP_KeyLock));
		x += sizeof(BITMAP_KeyLock);
		x1 = x;
	}
	else
	if (gWasFKeyPressed)
	{
		memmove(line + x, BITMAP_F_Key, sizeof(BITMAP_F_Key));
		x += sizeof(BITMAP_F_Key);
		x1 = x;
	}

	{	// battery voltage or percentage
		char         s[8];
		unsigned int space_needed;
		
		unsigned int x2 = LCD_WIDTH - sizeof(BITMAP_BatteryLevel5) - 3;

		if (gChargingWithTypeC)
			x2 -= sizeof(BITMAP_USB_C);  // the radio is on charge

		switch (gSetting_battery_text)
		{
			default:
			case 0:
				break;
	
			case 1:		// voltage
			{
				const uint16_t voltage = (gBatteryVoltageAverage <= 999) ? gBatteryVoltageAverage : 999; // limit to 9.99V
				sprintf(s, "%u.%02uV", voltage / 100, voltage % 100);
				space_needed = (7 * strlen(s));
				if (x2 >= (x1 + space_needed))
				{
					UI_PrintStringSmallBuffer(s, line + x2 - space_needed);
				}
				break;
			}
			
			case 2:		// percentage
			{
				sprintf(s, "%u%%", BATTERY_VoltsToPercent(gBatteryVoltageAverage));
				space_needed = (7 * strlen(s));
				if (x2 >= (x1 + space_needed))
					UI_PrintStringSmallBuffer(s, line + x2 - space_needed);
				break;
			}
		}
	}
		
	// move to right side of the screen
	x = LCD_WIDTH - sizeof(BITMAP_BatteryLevel1) - sizeof(BITMAP_USB_C);
	
	// USB-C charge indicator
	if (gChargingWithTypeC || test_display)
		memmove(line + x, BITMAP_USB_C, sizeof(BITMAP_USB_C));
	x += sizeof(BITMAP_USB_C);

	// BATTERY LEVEL indicator
	if (gBatteryDisplayLevel >= 2 && !gLowBattery)
	{
		line += x;
		memmove(line, BITMAP_BatteryLevel1, sizeof(BITMAP_BatteryLevel1));

		#ifndef ENABLE_REVERSE_BAT_SYMBOL
			line += sizeof(BITMAP_BatteryLevel1);
			{
				const int8_t pos[] = {-4, -7, -10, 13};
				for (unsigned int i = 0; i < ARRAY_SIZE(pos); i++)
					if (gBatteryDisplayLevel >= (2 + i))
						memmove(line + pos[i], BITMAP_BatteryLevel, sizeof(BITMAP_BatteryLevel));
	
/*				switch (gBatteryDisplayLevel)
				{
					default:
					case 5: memmove(line - 13, BITMAP_BatteryLevel, sizeof(BITMAP_BatteryLevel));
					case 4: memmove(line - 10, BITMAP_BatteryLevel, sizeof(BITMAP_BatteryLevel));
					case 3: memmove(line -  7, BITMAP_BatteryLevel, sizeof(BITMAP_BatteryLevel));
					case 2: memmove(line -  4, BITMAP_BatteryLevel, sizeof(BITMAP_BatteryLevel));
				}
*/			}
		#else
		{
			const int8_t pos[] = {3, 6, 9, 12};
			for (unsigned int i = 0; i < ARRAY_SIZE(pos); i++)
				if (gBatteryDisplayLevel >= (2 + i))
					memmove(line + pos[i], BITMAP_BatteryLevel, sizeof(BITMAP_BatteryLevel));
/*
			switch (gBatteryDisplayLevel)
			{
				default:
				case 5: memmove(line + 12, BITMAP_BatteryLevel, sizeof(BITMAP_BatteryLevel));
				case 4: memmove(line +  9, BITMAP_BatteryLevel, sizeof(BITMAP_BatteryLevel));
				case 3: memmove(line +  6, BITMAP_BatteryLevel, sizeof(BITMAP_BatteryLevel));
				case 2: memmove(line +  3, BITMAP_BatteryLevel, sizeof(BITMAP_BatteryLevel));
			}
*/		}
		#endif
	}
	else
	if (gLowBatteryBlink == 1)
		memmove(line + x, BITMAP_BatteryLevel1, sizeof(BITMAP_BatteryLevel1));

	// **************

	ST7565_BlitStatusLine();
}
