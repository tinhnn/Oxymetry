#include "sensor_thread.h"
#include "ob1203_bio/ob1203_bio.h"
#include "usb_pcdc.h"

typedef enum e_demo_sequence
{
    DEMO_SEQUENCE_1 = (1),
    DEMO_SEQUENCE_2,
    DEMO_SEQUENCE_3,
    DEMO_SEQUENCE_4,
    DEMO_SEQUENCE_5,
    DEMO_SEQUENCE_6,
    DEMO_SEQUENCE_7,
    DEMO_SEQUENCE_8,
    DEMO_SEQUENCE_9,
    DEMO_SEQUENCE_10,
    DEMO_SEQUENCE_11,
    DEMO_SEQUENCE_12,
} demo_sequence_t;

static          spo2_t              gs_spo2;
static volatile ob1203_bio_data_t   gs_ob1203_bio_data;
extern TaskHandle_t sensor_thread;

//
static uint8_t TX_buf[8] = {'S', '_', 0x00, 0x00, 0x00, 0x00, 0x0D, 0x0A};

/* See Developer Assistance in the project */
void g_comms_i2c_bus0_quick_setup(void);

/* Quick setup for g_comms_i2c_bus0. */
void g_comms_i2c_bus0_quick_setup(void)
{
    fsp_err_t err;
    i2c_master_instance_t * p_driver_instance = (i2c_master_instance_t *) g_comms_i2c_bus0_extended_cfg.p_driver_instance;

    /* Open I2C driver, this must be done before calling any COMMS API */
    err = p_driver_instance->p_api->open(p_driver_instance->p_ctrl, p_driver_instance->p_cfg);
    assert(FSP_SUCCESS == err);

    /* Create a semaphore for blocking if a semaphore is not NULL */
    if (NULL != g_comms_i2c_bus0_extended_cfg.p_blocking_semaphore)
    {
        *(g_comms_i2c_bus0_extended_cfg.p_blocking_semaphore->p_semaphore_handle)
            = xSemaphoreCreateCountingStatic((UBaseType_t) 1, (UBaseType_t) 0, g_comms_i2c_bus0_extended_cfg.p_blocking_semaphore->p_semaphore_memory);
    }

    /* Create a recursive mutex for bus lock if a recursive mutex is not NULL */
    if (NULL != g_comms_i2c_bus0_extended_cfg.p_bus_recursive_mutex)
    {
        *(g_comms_i2c_bus0_extended_cfg.p_bus_recursive_mutex->p_mutex_handle)
            = xSemaphoreCreateRecursiveMutexStatic(g_comms_i2c_bus0_extended_cfg.p_bus_recursive_mutex->p_mutex_memory);
    }
}

/* OB1203 thread entry function */
/* pvParameters contains TaskHandle_t */
void sensor_thread_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED (pvParameters);

    /* TODO: add your own code here */
    bool result;
    rm_ob1203_raw_data_t raw_data;
    rm_ob1203_ppg_data_t ppg_data;
    ob1203_bio_t ob1203_bio;
    bool change = false;
    bool valid = false;
    bool update = false;
    bool ready = false;
    ob1203_bio_gain_currents_t gain_currents;
    demo_sequence_t sequence = DEMO_SEQUENCE_1;

    /* Set default gain and currents */
    gain_currents.gain.ppg_prox = g_ob1203_sensor1_extended_cfg.ppg_prox_gain;
    gain_currents.currents.ir_led = g_ob1203_sensor1_extended_cfg.ppg_ir_led_current;
    gain_currents.currents.red_led = g_ob1203_sensor1_extended_cfg.ppg_red_led_current;

    /* Open the Bus */
    g_comms_i2c_bus0_quick_setup();

    /* Open OB1203 Bio extension */
    result = ob1203_bio_open(&ob1203_bio,
                             (rm_ob1203_instance_t*)&g_ob1203_sensor0, // Proximity mode
                             (rm_ob1203_instance_t*)&g_ob1203_sensor1,  // PPG mode
                             &gs_spo2);
    if (false == result)
    {
        vTaskDelete(sensor_thread);
    }

    while (1)
    {
        switch (sequence)
        {
            case DEMO_SEQUENCE_1 :
            {
                /* Initialize an operation mode */
                result = ob1203_bio_operation_mode_init(&ob1203_bio);
                if (false == result)
                {
                    vTaskDelete(sensor_thread);
                }

                sequence = DEMO_SEQUENCE_2;
            }
            break;

            case DEMO_SEQUENCE_2 :
            {
                /* Start a measurement */
                result = ob1203_bio_measurement_start(&ob1203_bio);
                if (false == result)
                {
                    vTaskDelete(sensor_thread);
                }

                sequence = DEMO_SEQUENCE_3;
            }
            break;

            case DEMO_SEQUENCE_3:
            {
                /* Wait measurement period */
                result = ob1203_bio_measurement_period_wait(&ob1203_bio);
                if (false == result)
                {
                    vTaskDelete(sensor_thread);
                }

                sequence = DEMO_SEQUENCE_4;
            }
            break;

            case DEMO_SEQUENCE_4 :
            {
                /* Check if an operation mode needs to be changed */
                result = ob1203_bio_mode_change_check(&ob1203_bio, &change);
                if (false == result)
                {
                    vTaskDelete(sensor_thread);
                }

                if (false != change)
                {
                    /* Stop the measurement */
                    result = ob1203_bio_measurement_stop(&ob1203_bio);
                    if (false == result)
                    {
                        vTaskDelete(sensor_thread);
                    }

                    /* Change to another mode */
                    sequence = DEMO_SEQUENCE_1;
                }
                else
                {
                    /* No change */
                    sequence = DEMO_SEQUENCE_5;
                }
            }
            break;

            case DEMO_SEQUENCE_5:
            {
                /* Read raw data */
                result = ob1203_bio_ppg_raw_data_read(&ob1203_bio, &raw_data);
                if (false == result)
                {
                    vTaskDelete(sensor_thread);
                }

                sequence = DEMO_SEQUENCE_6;
            }
            break;

            case DEMO_SEQUENCE_6:
            {
                /* Calculate PPG data from raw data */
                result = ob1203_bio_ppg_data_calculate(&ob1203_bio, &raw_data, &ppg_data, &valid);
                if (false == result)
                {
                    vTaskDelete(sensor_thread);
                }

                if (false != valid)
                {
                    /* Valid data */
                    sequence = DEMO_SEQUENCE_7;
                }
                else
                {
                    /* Check if an operation mode needs to be changed */
                    result = ob1203_bio_mode_change_check(&ob1203_bio, &change);
                    if (false == result)
                    {
                        vTaskDelete(sensor_thread);
                    }

                    if (false != change)
                    {
                        /* Stop the measurement */
                        result = ob1203_bio_measurement_stop(&ob1203_bio);
                        if (false == result)
                        {
                            vTaskDelete(sensor_thread);
                        }

                        /* Change to another mode */
                        sequence = DEMO_SEQUENCE_1;
                    }
                    else
                    {
                        /* Invalid data */
                        sequence = DEMO_SEQUENCE_3;
                    }
                }
            }
            break;

            case DEMO_SEQUENCE_7:
            {
                /* Auto gain and currents control */
                result = ob1203_bio_auto_gain_currents_control(&ob1203_bio,
                                                               &ppg_data,
                                                               &gain_currents,
                                                               &update);
                if (false == result)
                {
                    vTaskDelete(sensor_thread);
                }

                if (false != update)
                {
                    /* Stop the measurement */
                    result = ob1203_bio_measurement_stop(&ob1203_bio);
                    if (false == result)
                    {
                        vTaskDelete(sensor_thread);
                    }

                    /* Reconfigure gain and currents */
                    result = ob1203_bio_gain_currents_reconfigure(&ob1203_bio, &gain_currents);
                    if (false == result)
                    {
                        vTaskDelete(sensor_thread);
                    }

                    sequence = DEMO_SEQUENCE_2;
                }
                else
                {
                    sequence = DEMO_SEQUENCE_8;
                }
            }
            break;

            case DEMO_SEQUENCE_8:
            {
                /* Check if the preparation for the algorithm is complete */
                result = ob1203_bio_algorithm_preparation_check(&ob1203_bio, &ready);
                if (false == result)
                {
                    vTaskDelete(sensor_thread);
                }

                if (false == ready)
                {
                    /* Stop the measurement */
                    result = ob1203_bio_measurement_stop(&ob1203_bio);
                    if (false == result)
                    {
                        vTaskDelete(sensor_thread);
                    }

                    /* Reset the algorithm */
                    result = ob1203_bio_algorithm_reset(&ob1203_bio);
                    if (false == result)
                    {
                        vTaskDelete(sensor_thread);
                    }

                    /* Clear PPG samples */
                    result = ob1203_bio_samples_clear(&ob1203_bio);
                    if (false == result)
                    {
                        vTaskDelete(sensor_thread);
                    }

                    sequence = DEMO_SEQUENCE_2;
                }
                else
                {
                    sequence = DEMO_SEQUENCE_9;
                }
            }
            break;

            case DEMO_SEQUENCE_9:
            {
                /* Add PPG samples */
                result = ob1203_bio_samples_add(&ob1203_bio, &ppg_data);
                if (false == result)
                {
                    vTaskDelete(sensor_thread);
                }

                sequence = DEMO_SEQUENCE_10;
            }
            break;

            case DEMO_SEQUENCE_10:
            {
                /* Calculate heart rate and SpO2 values */
                result = ob1203_bio_hr_spo2_calculate(&ob1203_bio, (ob1203_bio_data_t *)&gs_ob1203_bio_data);
                if (false == result)
                {
                    vTaskDelete(sensor_thread);
                }

                TX_buf[2] = (uint8_t)gs_ob1203_bio_data.spo2;
                TX_buf[3] = (uint8_t)gs_ob1203_bio_data.heart_rate;
                usb_send(TX_buf, sizeof(TX_buf));
                sequence = DEMO_SEQUENCE_11;
            }
            break;

            case DEMO_SEQUENCE_11:
            {
                /* Calculate a respiration rate value */
                result = ob1203_bio_rr_calculate(&ob1203_bio,
                                                 (ob1203_bio_data_t *)&gs_ob1203_bio_data);
                if (false == result)
                {
                    vTaskDelete(sensor_thread);
                }

                sequence = DEMO_SEQUENCE_12;
            }
            break;

            case DEMO_SEQUENCE_12:
            {
                /* Check perfusion index (PI) */
                result = ob1203_bio_perfusion_index_check(&ob1203_bio,
                                                          &valid);
                if (false == result)
                {
                    vTaskDelete(sensor_thread);
                }

                if (false != valid)
                {
                    sequence = DEMO_SEQUENCE_3;
                }
                else
                {
                    /* Stop the measurement */
                    result = ob1203_bio_measurement_stop(&ob1203_bio);
                    if (false == result)
                    {
                        vTaskDelete(sensor_thread);
                    }

                    /* Reset the algorithm */
                    result = ob1203_bio_algorithm_reset(&ob1203_bio);
                    if (false == result)
                    {
                        vTaskDelete(sensor_thread);
                    }

                    sequence = DEMO_SEQUENCE_2;
                }
            }
            break;

            default:
            {
                vTaskDelete(sensor_thread);
            }
            break;
        }
        vTaskDelay(1);
    }
}
