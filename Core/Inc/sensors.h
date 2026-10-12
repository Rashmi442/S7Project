/**
 ******************************************************************************
 * @file    sensors.h
 * @brief   Simulated physiological sensor front-end (ECG, HR, SpO2, Temp),
 *          Emergency Simulation Control, and Autonomous Clinical Evaluation.
 ******************************************************************************
 */
#ifndef __SENSORS_H
#define __SENSORS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "packet.h"

typedef enum
{
    SIM_MODE_NORMAL = 0,    /* Normal physiological baseline             */
    SIM_MODE_WARNING,       /* Warning level vitals (Tachycardia/Fever)  */
    SIM_MODE_CRITICAL,      /* Critical Arrhythmia / Severe Hypoxia      */
    SIM_MODE_COUNT
} SensorSimMode_t;

void            Sensors_Init(void);

/* Sensor simulation control (demonstration / testing) */
void            Sensors_SetSimulationMode(SensorSimMode_t mode);
SensorSimMode_t Sensors_GetSimulationMode(void);
void            Sensors_CycleSimulationMode(void);

/* Raw sensor readings */
int16_t         Sensors_ReadECGSample(void);
float           Sensors_GetHeartRate(void);
float           Sensors_GetSpO2(void);
float           Sensors_GetTemperature(void);

/* Evaluates vital signs against clinical limits and sets alert_flags */
uint16_t        Sensors_EvaluateEmergency(TelemetrySample_t *sample);

/* Full sample acquisition (10-sample ECG burst + vitals + evaluation) */
void            Sensors_AcquireSample(TelemetrySample_t *out_sample, uint16_t seq);

#ifdef __cplusplus
}
#endif

#endif /* __SENSORS_H */
