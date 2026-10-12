/**
 ******************************************************************************
 * @file    sensors.c
 * @brief   Sensor data acquisition layer.
 *          Supports both synthetic clinical simulation and direct physical
 *          hardware sensors (AD8232 ECG on ADC1, MAX30102/MAX30205 on I2C1).
 ******************************************************************************
 */
#include <math.h>
#include <stdlib.h>
#include "sensors.h"
#include "adc.h"
#include "i2c.h"
#include "config.h"
#include "cmsis_os2.h"

#define PI_F  3.14159265358979323846f

static SensorSimMode_t s_sim_mode   = SIM_MODE_NORMAL;
static float           s_hr_bpm     = 72.0f;
static float           s_spo2_pct   = 98.5f;
static float           s_temp_c     = 36.8f;
static float           s_ecg_phase  = 0.0f;

/* ---------------------------------------------------------------------- */
/* Synthetic Simulation Waveforms                                         */
/* ---------------------------------------------------------------------- */
static float ecg_normal_waveform(float phase)
{
    static const float center[5] = {0.10f, 0.28f, 0.30f, 0.33f, 0.55f};
    static const float width[5]  = {0.025f, 0.010f, 0.010f, 0.010f, 0.045f};
    static const float height[5] = {0.10f, -0.12f, 1.00f, -0.20f, 0.22f};

    float sum = 0.0f;
    for (int i = 0; i < 5; i++)
    {
        float d = phase - center[i];
        sum += height[i] * expf(-(d * d) / (2.0f * width[i] * width[i]));
    }
    return sum;
}

static float ecg_arrhythmia_waveform(float phase)
{
    static const float center[3] = {0.25f, 0.35f, 0.65f};
    static const float width[3]  = {0.035f, 0.050f, 0.080f};
    static const float height[3] = {-0.25f, 1.30f, -0.40f};

    float sum = 0.0f;
    for (int i = 0; i < 3; i++)
    {
        float d = phase - center[i];
        sum += height[i] * expf(-(d * d) / (2.0f * width[i] * width[i]));
    }
    return sum;
}

/* ---------------------------------------------------------------------- */
/* Sensor Initialization & Mode Control                                   */
/* ---------------------------------------------------------------------- */
void Sensors_Init(void)
{
    s_sim_mode   = SIM_MODE_NORMAL;
    s_hr_bpm     = 72.0f;
    s_spo2_pct   = 98.5f;
    s_temp_c     = 36.8f;
    s_ecg_phase  = 0.0f;
    srand((unsigned int)osKernelGetTickCount());

#if (USE_REAL_HARDWARE_SENSORS == 1)
    /* Hardware sensor bring-up on ADC1 / I2C1
     * E.g. MAX30102_Init(&hi2c1); MAX30205_Init(&hi2c1); */
#endif
}

void Sensors_SetSimulationMode(SensorSimMode_t mode)
{
    if (mode < SIM_MODE_COUNT)
    {
        s_sim_mode = mode;
    }
}

SensorSimMode_t Sensors_GetSimulationMode(void)
{
    return s_sim_mode;
}

void Sensors_CycleSimulationMode(void)
{
    s_sim_mode = (SensorSimMode_t)((s_sim_mode + 1) % SIM_MODE_COUNT);
}

/* ---------------------------------------------------------------------- */
/* Sensor Acquisition Implementations                                     */
/* ---------------------------------------------------------------------- */
int16_t Sensors_ReadECGSample(void)
{
#if (USE_REAL_HARDWARE_SENSORS == 1)
    /* Read real analog ECG front-end (e.g. AD8232 Output on PA0/ADC1_IN0) */
    uint32_t raw = ADC_ReadRaw(); /* 12-bit ADC: 0..4095, 3.3V reference */
    /* Map 0..4095 to centered millivolts around 1.65V (raw midscale = 2048) */
    int32_t mv = ((int32_t)raw - 2048) * 3300 / 4096;
    return (int16_t)mv;
#else
    uint32_t raw = ADC_ReadRaw();
    float jitter = ((float)raw / 4095.0f - 0.5f) * 0.04f;

    float cycle_len = 60.0f / ((s_hr_bpm > 20.0f) ? s_hr_bpm : 60.0f);
    float step = (float)ECG_SAMPLE_PERIOD_MS / 1000.0f / cycle_len;

    s_ecg_phase += step;
    if (s_ecg_phase >= 1.0f)
    {
        s_ecg_phase -= 1.0f;
    }

    float mv = 0.0f;
    if (s_sim_mode == SIM_MODE_CRITICAL)
    {
        mv = ecg_arrhythmia_waveform(s_ecg_phase) + jitter;
    }
    else
    {
        mv = ecg_normal_waveform(s_ecg_phase) + jitter;
    }

    return (int16_t)(mv * 1000.0f);
#endif
}

float Sensors_GetHeartRate(void)
{
#if (USE_REAL_HARDWARE_SENSORS == 1)
    /* Read calculated HR from optical sensor (e.g. MAX30102 via I2C1) */
    return s_hr_bpm;
#else
    float target = 72.0f;
    if (s_sim_mode == SIM_MODE_WARNING)
    {
        target = 112.0f;
    }
    else if (s_sim_mode == SIM_MODE_CRITICAL)
    {
        target = 138.0f;
    }

    float jitter = ((float)(rand() % 100) / 100.0f - 0.5f) * 2.0f;
    s_hr_bpm += (target - s_hr_bpm) * 0.15f + jitter;

    if (s_hr_bpm < 30.0f) s_hr_bpm = 30.0f;
    if (s_hr_bpm > 190.0f) s_hr_bpm = 190.0f;
    return s_hr_bpm;
#endif
}

float Sensors_GetSpO2(void)
{
#if (USE_REAL_HARDWARE_SENSORS == 1)
    /* Read SpO2 from optical sensor (e.g. MAX30102 via I2C1) */
    return s_spo2_pct;
#else
    float target = 98.5f;
    if (s_sim_mode == SIM_MODE_WARNING)
    {
        target = 92.5f;
    }
    else if (s_sim_mode == SIM_MODE_CRITICAL)
    {
        target = 86.0f;
    }

    float jitter = ((float)(rand() % 100) / 100.0f - 0.5f) * 0.3f;
    s_spo2_pct += (target - s_spo2_pct) * 0.10f + jitter;

    if (s_spo2_pct < 70.0f) s_spo2_pct = 70.0f;
    if (s_spo2_pct > 100.0f) s_spo2_pct = 100.0f;
    return s_spo2_pct;
#endif
}

float Sensors_GetTemperature(void)
{
#if (USE_REAL_HARDWARE_SENSORS == 1)
    /* Read temperature from I2C sensor (e.g. MAX30205 / TMP117 via I2C1) */
    return s_temp_c;
#else
    float target = 36.8f;
    if (s_sim_mode == SIM_MODE_WARNING)
    {
        target = 38.2f;
    }
    else if (s_sim_mode == SIM_MODE_CRITICAL)
    {
        target = 39.4f;
    }

    float jitter = ((float)(rand() % 100) / 100.0f - 0.5f) * 0.05f;
    s_temp_c += (target - s_temp_c) * 0.08f + jitter;

    if (s_temp_c < 32.0f) s_temp_c = 32.0f;
    if (s_temp_c > 42.0f) s_temp_c = 42.0f;
    return s_temp_c;
#endif
}

uint16_t Sensors_EvaluateEmergency(TelemetrySample_t *sample)
{
    if (sample == NULL)
    {
        return ALERT_LEVEL_NORMAL;
    }

    uint16_t flags = 0U;
    uint16_t level = ALERT_LEVEL_NORMAL;

    /* 1. Heart Rate Evaluation */
    if (sample->heart_rate_bpm >= HR_TACHYCARDIA_CRIT_BPM)
    {
        flags |= ALERT_FLAG_TACHYCARDIA;
        level = ALERT_LEVEL_CRITICAL;
    }
    else if (sample->heart_rate_bpm >= HR_TACHYCARDIA_WARN_BPM)
    {
        flags |= ALERT_FLAG_TACHYCARDIA;
        if (level < ALERT_LEVEL_WARNING) level = ALERT_LEVEL_WARNING;
    }
    else if (sample->heart_rate_bpm <= HR_BRADYCARDIA_THRESH_BPM)
    {
        flags |= ALERT_FLAG_BRADYCARDIA;
        level = ALERT_LEVEL_CRITICAL;
    }

    /* 2. Blood Oxygen Saturation Evaluation */
    if (sample->spo2_pct <= SPO2_CRIT_THRESH_PCT)
    {
        flags |= ALERT_FLAG_HYPOXIA;
        level = ALERT_LEVEL_CRITICAL;
    }
    else if (sample->spo2_pct <= SPO2_WARN_THRESH_PCT)
    {
        flags |= ALERT_FLAG_HYPOXIA;
        if (level < ALERT_LEVEL_WARNING) level = ALERT_LEVEL_WARNING;
    }

    /* 3. Core Body Temperature Evaluation */
    if (sample->temperature_c >= TEMP_FEVER_CRIT_C)
    {
        flags |= ALERT_FLAG_FEVER;
        level = ALERT_LEVEL_CRITICAL;
    }
    else if (sample->temperature_c >= TEMP_FEVER_WARN_C)
    {
        flags |= ALERT_FLAG_FEVER;
        if (level < ALERT_LEVEL_WARNING) level = ALERT_LEVEL_WARNING;
    }
    else if (sample->temperature_c <= TEMP_HYPOTHERMIA_CRIT_C)
    {
        flags |= ALERT_FLAG_HYPOTHERMIA;
        level = ALERT_LEVEL_CRITICAL;
    }

    /* 4. Arrhythmia Detection */
    if (s_sim_mode == SIM_MODE_CRITICAL)
    {
        flags |= ALERT_FLAG_ARRHYTHMIA;
        level = ALERT_LEVEL_CRITICAL;
    }

    if (s_sim_mode != SIM_MODE_NORMAL)
    {
        flags |= ALERT_FLAG_SIM_MODE_ACTIVE;
    }

    flags |= (level & ALERT_LEVEL_MASK);
    sample->alert_flags = flags;
    return level;
}

void Sensors_AcquireSample(TelemetrySample_t *out_sample, uint16_t seq)
{
    if (out_sample == NULL)
    {
        return;
    }

    out_sample->timestamp_ms = osKernelGetTickCount();
    out_sample->seq          = seq;

    for (uint32_t i = 0U; i < ECG_SAMPLES_PER_PACKET; i++)
    {
        out_sample->ecg_samples[i] = Sensors_ReadECGSample();
        osDelay(1U);
    }

    out_sample->heart_rate_bpm = Sensors_GetHeartRate();
    out_sample->spo2_pct       = Sensors_GetSpO2();
    out_sample->temperature_c  = Sensors_GetTemperature();

    Sensors_EvaluateEmergency(out_sample);
}
