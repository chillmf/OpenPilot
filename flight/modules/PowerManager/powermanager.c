/**
 ******************************************************************************
 * @addtogroup OpenPilotModules OpenPilot Modules
 * @{
 * @addtogroup PowerManagerModule PowerManager Module
 * @brief Measures the battery level and controls the charging circuit.
 * Updates the FlightBatteryState object
 * @{
 *
 * @file       powermanager.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2013.
 * @brief      Module to monitor an on-board power manager and control battery charging.
 *
 * @see        The GNU Public License (GPL) Version 3
 *
 *****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/**
 * Output object: FlightBatteryState
 *
 * This module will periodically generate information on the battery state.
 *
 * UAVObjects are automatically generated by the UAVObjectGenerator from
 * the object definition XML file.
 *
 * Modules have no API, all communication to other modules is done through UAVObjects.
 * However modules may use the API exposed by shared libraries.
 * See the OpenPilot wiki for more details.
 * http://www.openpilot.org/OpenPilot_Application_Architecture
 *
 */

#include <openpilot.h>

#include <pios_bq24075.h>
#include <flightbatterystate.h>

//
// Configuration
//
#define SAMPLE_PERIOD_MS   500
#define ADC_INTERNAL_VREF  1.20f
#define PM_BAT_DIVIDER     3.0f

// Private types
static const float charge_perc[10] =
{
  3.00f, // 00%
  3.78f, // 10%
  3.83f, // 20%
  3.87f, // 30%
  3.89f, // 40%
  3.92f, // 50%
  3.96f, // 60%
  4.00f, // 70%
  4.04f, // 80%
  4.10f  // 90%
};

// Private variables

// Private functions
static void onTimer(UAVObjEvent *ev);
static float adcConvertToVoltageFloat(uint16_t v, uint16_t vref);

/**
 * Initialise the module, called on startup
 * \returns 0 on success or -1 if initialisation failed
 */
int32_t PowerManagerInitialize(void)
{

    // Start module
    FlightBatteryStateInitialize();

    static UAVObjEvent ev;
    memset(&ev, 0, sizeof(UAVObjEvent));
    EventPeriodicCallbackCreate(&ev, onTimer, SAMPLE_PERIOD_MS / portTICK_RATE_MS);

    return 0;
}

int32_t PowerManagerStart(void)
{
    PIOS_BQ24075_SetChargeState(PIOS_BQ24075_ID, PIOS_BQ24075_CHARGE_500MA);
    return 0;
}

MODULE_INITCALL(PowerManagerInitialize, PowerManagerStart);

/**
 * Returns a number from 0 to 9 where 0 is completely discharged
 * and 9 is 90% charged.
 */
static int32_t pmBatteryChargeFromVoltage(float voltage)
{
  int charge = 0;

  if (voltage < charge_perc[0]) {
      charge = 0;
  } else if (voltage > charge_perc[9]) {
      charge = 9;
  } else {
      while (voltage >  charge_perc[charge]) {
          charge++;
      }
  }

  return charge;
}

static float adcConvertToVoltageFloat(uint16_t v, uint16_t vref)
{
  return (v / (vref / ADC_INTERNAL_VREF)) * PM_BAT_DIVIDER;
}

static void onTimer(__attribute__((unused)) UAVObjEvent *ev)
{
    //const float dT = SAMPLE_PERIOD_MS / 1000.0f;
    static FlightBatteryStateData flightBatteryData;
    uint16_t vref_adc = PIOS_ADC_PinGet(PIOS_ADC_VREF_ADC_PIN);
    uint16_t bat_adc = PIOS_ADC_PinGet(PIOS_ADC_BAT_ADC_PIN);
    float voltage = adcConvertToVoltageFloat(bat_adc, vref_adc);
    int32_t charge = pmBatteryChargeFromVoltage(flightBatteryData.Voltage);

    // Set the battery voltage.
    FlightBatteryStateGet(&flightBatteryData);
    flightBatteryData.Voltage = voltage;
    flightBatteryData.Current = PIOS_BQ24075_GetChargingState(PIOS_BQ24075_ID);
    flightBatteryData.EstimatedFlightTime = charge;
    FlightBatteryStateSet(&flightBatteryData);

    if (charge == 1) {
        AlarmsSet(SYSTEMALARMS_ALARM_BATTERY, SYSTEMALARMS_ALARM_WARNING);
    } else if (charge == 0) {
        AlarmsSet(SYSTEMALARMS_ALARM_BATTERY, SYSTEMALARMS_ALARM_ERROR);
    } else {
        AlarmsSet(SYSTEMALARMS_ALARM_BATTERY, SYSTEMALARMS_ALARM_OK);
    }
}

/**
 * @}
 */
