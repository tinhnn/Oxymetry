#include "usb_thread.h"
#include "usb_pcdc.h"

/* Buffer to store user data */
uint8_t user_data_buf[DATA_LEN] = {RESET_VALUE};

/* Pointers to store USB descriptors */
extern uint8_t g_apl_device[];
extern uint8_t g_apl_configuration[];
extern uint8_t g_apl_hs_configuration[];
extern uint8_t g_apl_qualifier_descriptor[];
extern uint8_t * g_apl_string_table[];

const usb_descriptor_t g_usb_pcdc_descriptor =
{
 g_apl_device,                   /* Pointer to the device descriptor */
 g_apl_configuration,            /* Pointer to the configuration descriptor for Full-speed */
 g_apl_hs_configuration,         /* Pointer to the configuration descriptor for Hi-speed */
 g_apl_qualifier_descriptor,     /* Pointer to the qualifier descriptor */
 g_apl_string_table,             /* Pointer to the string descriptor table */
 NUM_STRING_DESCRIPTOR           /* Num entry String Descriptor */
};

/* Flag to indicate USB resume/suspend status */
static bool  b_usb_attach = false;
volatile usb_pcdc_linecoding_t g_line_coding;
usb_pcdc_ctrllinestate_t g_control_line_state =
{
 .bdtr = 0,
 .brts = 0,
};

//
bool flg_snd = false;
uint8_t TX_BUFFER[1024] = {0};
int16_t TX_LEN = 0;

void deinit_usb();

/* USB thread entry function */
/* pvParameters contains TaskHandle_t */
void usb_thread_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED (pvParameters);
    fsp_err_t err              = FSP_SUCCESS;

    /* Open USB instance */
    {
        err = R_USB_Open (&g_UsbBasic_ctrl, &g_UsbBasic_cfg);
        if (FSP_SUCCESS != err)
        {
            // error handle
        }
    }

    /* TODO: add your own code here */
    while (1)
    {
        if (true == b_usb_attach){
            if(flg_snd){
                err = R_USB_Write (&g_UsbBasic_ctrl, &TX_BUFFER[0], (uint32_t)TX_LEN, USB_CLASS_PCDC);
                if (FSP_SUCCESS != err)
                {
                    deinit_usb();
                }
                else{
                    flg_snd = false;
                }
            }
        }
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
}

void deinit_usb(void)
{
    fsp_err_t err = FSP_SUCCESS;

    /* Close module */
    err = R_USB_Close (&g_UsbBasic_ctrl);
    if (FSP_SUCCESS != err)
    {
        // TODO
    }
}

void usb_pcdc_callback(usb_event_info_t * p_pcdc_event , usb_hdl_t task, usb_onoff_t state)
{
    FSP_PARAMETER_NOT_USED (task);
    FSP_PARAMETER_NOT_USED (state);

    switch (p_pcdc_event->event)
    {
        /* USB Read complete Class Request */
        case USB_STATUS_READ_COMPLETE:
        {
            // TODO
        }
        break;

        /*  Write Complete Class Request */
        case USB_STATUS_WRITE_COMPLETE:
        {
            // TODO
        }
        break;

        /* Configured state class request */
        case USB_STATUS_CONFIGURED:
        {
            /* Read data from tera term */
            fsp_err_t err = R_USB_Read (&g_UsbBasic_ctrl, user_data_buf, DATA_LEN, USB_CLASS_PCDC);
            if (FSP_SUCCESS != err)
            {
               //
            }
            return;
        }
        break;

        /* Receive Class Request */
        case USB_STATUS_REQUEST:
        {
            /* Check for the specific CDC class request IDs */
            if (USB_PCDC_SET_LINE_CODING == (p_pcdc_event->setup.request_type & USB_BREQUEST))
            {
                /* Get the class request.*/
                fsp_err_t err = R_USB_PeriControlDataGet (&g_UsbBasic_ctrl, (uint8_t*) &g_line_coding, LINE_CODING_LENGTH);
                if(FSP_SUCCESS == err)
                {
                    // TODO
                }
            }

            else if (USB_PCDC_GET_LINE_CODING == (p_pcdc_event->setup.request_type & USB_BREQUEST))
            {
                /* Set the class request.*/
                fsp_err_t err = R_USB_PeriControlDataSet (&g_UsbBasic_ctrl, (uint8_t*) &g_line_coding, LINE_CODING_LENGTH);
                if (FSP_SUCCESS == err)
                {
                    //
                }
                else
                {
                    deinit_usb();
                }
            }
            else if (USB_PCDC_SET_CONTROL_LINE_STATE == (p_pcdc_event->setup.request_type & USB_BREQUEST))
            {
                /* Get the status of the control signals */
                fsp_err_t err = R_USB_PeriControlDataGet(&g_UsbBasic_ctrl,
                                                         (uint8_t*) &g_control_line_state,
                                                         sizeof(usb_pcdc_ctrllinestate_t));
                if (FSP_SUCCESS == err)
                {
                    if (FSP_SUCCESS == err)
                    {
                        // TODO
                    }
                }

                /* Set the usb status as ACK response.*/
                err = R_USB_PeriControlStatusSet (&g_UsbBasic_ctrl, USB_SETUP_STATUS_ACK);
                if (FSP_SUCCESS != err)
                {
                    deinit_usb();
                }
            }
            else
            {
                ;
            }
            return;
        }
        break;
        /* Complete Class Request */
        case USB_STATUS_REQUEST_COMPLETE:
        {
            if(USB_PCDC_SET_LINE_CODING == (p_pcdc_event->setup.request_type & USB_BREQUEST))
            {
            }
            else if (USB_PCDC_GET_LINE_CODING == (p_pcdc_event->setup.request_type & USB_BREQUEST))
            {
            }
            else
            {
                ;
            }
            return;
        }
        break;

        /* Detach, Suspend State Class requests */
        case USB_STATUS_DETACH:
            /* Stop all read/write transactions using R_USB_Stop */
        case USB_STATUS_SUSPEND:
        {
            /* Reset the usb attached flag indicating usb is removed.*/
            b_usb_attach = false;
            memset (user_data_buf, RESET_VALUE, sizeof(user_data_buf));
            return;
        }
        /* Resume state */
        case USB_STATUS_RESUME:
        {
            /* set the usb attached flag*/
            b_usb_attach = true;
        }
        break;

        default:
            return;
            break;
    }
}

void usb_send(uint8_t *buf, int16_t len){
    if(flg_snd){
        flg_snd = false;
    }

    memcpy((void*)&TX_BUFFER[0], (void*)buf, (size_t)len);
    TX_LEN = len;

    flg_snd = true;
}

