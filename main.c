#include <stdint.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf.h"
#include "nrf51_bitfields.h"
#include "ble_hci.h"
#include "ble_advdata.h"
#include "ble_conn_params.h"
#include "softdevice_handler.h"
#include "app_timer.h"
#include "app_button.h"
#include "ble_nus.h"
#include "simple_uart.h"
#include "boards.h"
#include "ble_error_log.h"
#include "ble_debug_assert_handler.h"
#include "app_util_platform.h"
#include "app_gpiote.h"
#include "app_trace.h"
#include "twi_master.h"
#include "lcd_defs.h"


#define IS_SRVC_CHANGED_CHARACT_PRESENT 0                                           /**< Include or not the service_changed characteristic. if not enabled,
                                                                                      the server's database cannot be changed for the lifetime of the device*/

#define DEVICE_NAME                     "Hans Wurst"                              /**< Name of device. Will be included in the advertising data. */

#define APP_ADV_INTERVAL                64                                          /**< The advertising interval (in units of 0.625 ms. This value corresponds to 40 ms).*/
#define APP_ADV_TIMEOUT_IN_SECONDS      180                                         /**< The advertising timeout (in units of seconds). */

#define APP_TIMER_PRESCALER             0                                           /**< Value of the RTC1 PRESCALER register. */
#define APP_TIMER_MAX_TIMERS            2                                           /**< Maximum number of simultaneously created timers. */
#define APP_TIMER_OP_QUEUE_SIZE         4                                           /**< Size of timer operation queues. */

#define MIN_CONN_INTERVAL               16                                          /**< Minimum acceptable connection interval (20 ms),
                                                                                      Connection interval uses 1.25 ms units. */
#define MAX_CONN_INTERVAL               60                                          /**< Maximum acceptable connection interval (75 ms),
                                                                                      Connection interval uses 1.25 ms units. */
#define SLAVE_LATENCY                   0                                           /**< slave latency. */
#define CONN_SUP_TIMEOUT                400                                         /**< Connection supervisory timeout (4 seconds), Supervision Timeout uses 10 ms units. */
#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000, APP_TIMER_PRESCALER)  /**< Time from initiating event (connect or start of notification)
                                                                                      to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000, APP_TIMER_PRESCALER) /**< Time between each call to sd_ble_gap_conn_param_update
                                                                                      after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                           /**< Number of attempts before giving up the connection parameter negotiation. */

#define BUTTON_DETECTION_DELAY          APP_TIMER_TICKS(50, APP_TIMER_PRESCALER)    /**< Delay from a GPIOTE event until a button
                                                                                      is reported as pushed (in number of timer ticks). */

#define SEC_PARAM_TIMEOUT               30                                          /**< Timeout for Pairing Request or Security Request (in seconds). */
#define SEC_PARAM_BOND                  1                                           /**< Perform bonding. */
#define SEC_PARAM_MITM                  0                                           /**< Man In The Middle protection not required. */
#define SEC_PARAM_IO_CAPABILITIES       BLE_GAP_IO_CAPS_NONE                        /**< No I/O capabilities. */
#define SEC_PARAM_OOB                   0                                           /**< Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE          7                                           /**< Minimum encryption key size. */
#define SEC_PARAM_MAX_KEY_SIZE          16                                          /**< Maximum encryption key size. */

#define START_STRING                    "Start...\n"                                /**< The string that will be sent over the UART when the application starts. */

#define DEAD_BEEF                       0xDEADBEEF                                  /**< Value used as error code on stack dump,
                                                                                      can be used to identify stack location on stack unwind. */

#define ADVERTISING_LED_PIN_NO           13                                          /**< Is on when device is advertising. */
#define CONNECTED_LED_PIN_NO             14                                          /**< Is on when device has connected. */
#define CAPS_ON_LED_PIN_NO               15                                          /**< Pin for indicating that CAPS LOCK is on. */
#define ADV_DIRECTED_LED_PIN_NO          16                                          /**< Is on when we are doing directed advertisement. */
#define ADV_WHITELIST_LED_PIN_NO         17                                          /**< Is on when we are doing advertising with whitelist. */
#define ADV_INTERVAL_SLOW_LED_PIN_NO     18                                          /**< Is on when we are doing slow advertising.  */
#define DEBUG_GPIO_LED_PIN               18                                          /**< Is on when we are doing slow advertising.  */


#define APP_GPIOTE_MAX_USERS             1

#define ACTION_BUTTON_PIN                0
#define WAKEUP_BUTTON_PIN                1


static ble_gap_sec_params_t             m_sec_params;                               /**< Security requirements for this application. */
static uint16_t                         m_conn_handle = BLE_CONN_HANDLE_INVALID;    /**< Handle of the current connection. */
static ble_nus_t                        m_nus;                                      /**< Structure to identify the Nordic UART Service. */


/**@brief     Error handler function, which is called when an error has occurred.
 *
 * @warning   This handler is an example only and does not fit a final product. You need to analyze
 *            how your product is supposed to react in case of error.
 *
 * @param[in] error_code  Error code supplied to the handler.
 * @param[in] line_num    Line number where the handler is called.
 * @param[in] p_file_name Pointer to the file name. 
 */


void rgb_lcd_default();
void rgb_lcd_connected();
void rgb_lcd_error();
void rgb_lcd_sleep();
void rgb_lcd_wash_open();
void rgb_lcd_wash_closed();


void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name)
{
    for (;;) {
        nrf_gpio_pin_set(13);
        nrf_gpio_pin_set(14);
        nrf_gpio_pin_set(15);
        nrf_gpio_pin_set(16);
        nrf_gpio_pin_set(17);
        nrf_gpio_pin_set(18);
        nrf_gpio_pin_toggle(DEBUG_GPIO_LED_PIN);
        nrf_delay_ms(1000);
        nrf_gpio_pin_clear(13);
        nrf_gpio_pin_clear(14);
        nrf_gpio_pin_clear(15);
        nrf_gpio_pin_clear(16);
        nrf_gpio_pin_clear(17);
        nrf_gpio_pin_clear(18);
        nrf_delay_ms(1000);
    }

    // This call can be used for debug purposes during application development.
    // @note CAUTION: Activating this code will write the stack to flash on an error.
    //                This function should NOT be used in a final product.
    //                It is intended STRICTLY for development/debugging purposes.
    //                The flash write will happen EVEN if the radio is active, thus interrupting
    //                any communication.
    //                Use with care. Un-comment the line below to use.
    // ble_debug_assert_handler(error_code, line_num, p_file_name);

    // On assert, the system can only recover with a reset.
    NVIC_SystemReset();
}


/**@brief       Assert macro callback function.
 *
 * @details     This function will be called in case of an assert in the SoftDevice.
 *
 * @warning     This handler is an example only and does not fit a final product. You need to
 *              analyze how your product is supposed to react in case of Assert.
 * @warning     On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in]   line_num   Line number of the failing ASSERT call.
 * @param[in]   file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}


/**@brief   Function for the LEDs initialization.
 *
 * @details Initializes all LEDs used by this application.
 */
static void leds_init(void)
{
    nrf_gpio_range_cfg_output(13, 18);
}


/**@brief   Function for Timer initialization.
 *
 * @details Initializes the timer module.
 */
static void timers_init(void)
{
    // Initialize timer module
    APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_MAX_TIMERS, APP_TIMER_OP_QUEUE_SIZE, false);
}


/**@brief   Function for the GAP initialization.
 *
 * @details This function will setup all the necessary GAP (Generic Access Profile)
 *          parameters of the device. It also sets the permissions and appearance.
 */
static void gap_params_init(void)
{
    uint32_t                err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);
    
    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *) DEVICE_NAME,
                                          strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}


/**@brief   Function for the Advertising functionality initialization.
 *
 * @details Encodes the required advertising data and passes it to the stack.
 *          Also builds a structure to be passed to the stack when starting advertising.
 */
static void advertising_init(void)
{
    uint32_t      err_code;
    ble_advdata_t advdata;
    ble_advdata_t scanrsp;
    uint8_t       flags = BLE_GAP_ADV_FLAGS_LE_ONLY_LIMITED_DISC_MODE;
    
    ble_uuid_t adv_uuids[] = {{BLE_UUID_NUS_SERVICE, m_nus.uuid_type}};

    memset(&advdata, 0, sizeof(advdata));
    advdata.name_type               = BLE_ADVDATA_FULL_NAME;
    advdata.include_appearance      = false;
    advdata.flags.size              = sizeof(flags);
    advdata.flags.p_data            = &flags;

    memset(&scanrsp, 0, sizeof(scanrsp));
    scanrsp.uuids_complete.uuid_cnt = sizeof(adv_uuids) / sizeof(adv_uuids[0]);
    scanrsp.uuids_complete.p_uuids  = adv_uuids;
    
    err_code = ble_advdata_set(&advdata, &scanrsp);
    APP_ERROR_CHECK(err_code);
}


/**@brief    Function for handling the data from the Nordic UART Service.
 *
 * @details  This function will process the data received from the Nordic UART BLE Service and send
 *           it to the UART module.
 */
/**@snippet [Handling the data received over BLE] */
void nus_data_handler(ble_nus_t * p_nus, uint8_t * p_data, uint16_t length);
/**@snippet [Handling the data received over BLE] */


/**@brief Function for initializing services that will be used by the application.
 */
static void services_init(void)
{
    uint32_t         err_code;
    ble_nus_init_t   nus_init;
    
    memset(&nus_init, 0, sizeof(nus_init));

    nus_init.data_handler = nus_data_handler;
    
    err_code = ble_nus_init(&m_nus, &nus_init);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing security parameters.
 */
static void sec_params_init(void)
{
    m_sec_params.timeout      = SEC_PARAM_TIMEOUT;
    m_sec_params.bond         = SEC_PARAM_BOND;
    m_sec_params.mitm         = SEC_PARAM_MITM;
    m_sec_params.io_caps      = SEC_PARAM_IO_CAPABILITIES;
    m_sec_params.oob          = SEC_PARAM_OOB;  
    m_sec_params.min_key_size = SEC_PARAM_MIN_KEY_SIZE;
    m_sec_params.max_key_size = SEC_PARAM_MAX_KEY_SIZE;
}


/**@brief       Function for handling an event from the Connection Parameters Module.
 *
 * @details     This function will be called for all events in the Connection Parameters Module
 *              which are passed to the application.
 *
 * @note        All this function does is to disconnect. This could have been done by simply setting
 *              the disconnect_on_fail config parameter, but instead we use the event handler
 *              mechanism to demonstrate its use.
 *
 * @param[in]   p_evt   Event received from the Connection Parameters Module.
 */
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
    uint32_t err_code;
    
    if(p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
    }
}


/**@brief       Function for handling errors from the Connection Parameters module.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for initializing the Connection Parameters module.
 */
static void conn_params_init(void)
{
    uint32_t               err_code;
    ble_conn_params_init_t cp_init;
    
    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = on_conn_params_evt;
    cp_init.error_handler                  = conn_params_error_handler;
    
    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for starting advertising.
 */
static void advertising_start(void)
{
    uint32_t             err_code;
    ble_gap_adv_params_t adv_params;
    
    // Start advertising
    memset(&adv_params, 0, sizeof(adv_params));
    
    adv_params.type        = BLE_GAP_ADV_TYPE_ADV_IND;
    adv_params.p_peer_addr = NULL;
    adv_params.fp          = BLE_GAP_ADV_FP_ANY;
    adv_params.interval    = APP_ADV_INTERVAL;
    adv_params.timeout     = APP_ADV_TIMEOUT_IN_SECONDS;

    err_code = sd_ble_gap_adv_start(&adv_params);
    APP_ERROR_CHECK(err_code);

    nrf_gpio_pin_set(ADVERTISING_LED_PIN_NO);
}


/**@brief       Function for the Application's S110 SoftDevice event handler.
 *
 * @param[in]   p_ble_evt   S110 SoftDevice event.
 */
static void on_ble_evt(ble_evt_t * p_ble_evt)
{
    uint32_t                         err_code;
    static ble_gap_evt_auth_status_t m_auth_status;
    ble_gap_enc_info_t *             p_enc_info;
    
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            rgb_lcd_connected();
            nrf_gpio_pin_set(CONNECTED_LED_PIN_NO);
            nrf_gpio_pin_clear(ADVERTISING_LED_PIN_NO);
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;

            break;
            
        case BLE_GAP_EVT_DISCONNECTED:
            rgb_lcd_default();
            nrf_gpio_pin_clear(CONNECTED_LED_PIN_NO);
            m_conn_handle = BLE_CONN_HANDLE_INVALID;

            advertising_start();

            break;
            
        case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
            err_code = sd_ble_gap_sec_params_reply(m_conn_handle, 
                                                   BLE_GAP_SEC_STATUS_SUCCESS, 
                                                   &m_sec_params);
            APP_ERROR_CHECK(err_code);
            break;
            
        case BLE_GATTS_EVT_SYS_ATTR_MISSING:
            err_code = sd_ble_gatts_sys_attr_set(m_conn_handle, NULL, 0);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GAP_EVT_AUTH_STATUS:
            m_auth_status = p_ble_evt->evt.gap_evt.params.auth_status;
            break;
            
        case BLE_GAP_EVT_SEC_INFO_REQUEST:
            p_enc_info = &m_auth_status.periph_keys.enc_info;
            if (p_enc_info->div == p_ble_evt->evt.gap_evt.params.sec_info_request.div)
            {
                err_code = sd_ble_gap_sec_info_reply(m_conn_handle, p_enc_info, NULL);
                APP_ERROR_CHECK(err_code);
            }
            else
            {
                // No keys found for this device
                err_code = sd_ble_gap_sec_info_reply(m_conn_handle, NULL, NULL);
                APP_ERROR_CHECK(err_code);
            }
            break;

        case BLE_GAP_EVT_TIMEOUT:
            if (p_ble_evt->evt.gap_evt.params.timeout.src == BLE_GAP_TIMEOUT_SRC_ADVERTISEMENT)
            { 
                rgb_lcd_sleep();
                nrf_gpio_pin_clear(ADVERTISING_LED_PIN_NO);

                // Configure buttons with sense level low as wakeup source.
                nrf_gpio_cfg_sense_input(WAKEUP_BUTTON_PIN,
                                         BUTTON_PULL,
                                         NRF_GPIO_PIN_SENSE_LOW);
                
                // Go to system-off mode (this function will not return; wakeup will cause a reset)
                err_code = sd_power_system_off();    
                APP_ERROR_CHECK(err_code);
            }
            break;

        default:
            // No implementation needed.
            break;
    }
}


/**@brief       Function for dispatching a S110 SoftDevice event to all modules with a S110
 *              SoftDevice event handler.
 *
 * @details     This function is called from the S110 SoftDevice event interrupt handler after a
 *              S110 SoftDevice event has been received.
 *
 * @param[in]   p_ble_evt   S110 SoftDevice event.
 */
static void ble_evt_dispatch(ble_evt_t * p_ble_evt)
{
    ble_conn_params_on_ble_evt(p_ble_evt);
    ble_nus_on_ble_evt(&m_nus, p_ble_evt);
    on_ble_evt(p_ble_evt);
}


/**@brief   Function for the S110 SoftDevice initialization.
 *
 * @details This function initializes the S110 SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
    uint32_t err_code;
    
    // Initialize SoftDevice.
    SOFTDEVICE_HANDLER_INIT(NRF_CLOCK_LFCLKSRC_XTAL_20_PPM, false);

    // Enable BLE stack 
    ble_enable_params_t ble_enable_params;
    memset(&ble_enable_params, 0, sizeof(ble_enable_params));
    ble_enable_params.gatts_enable_params.service_changed = IS_SRVC_CHANGED_CHARACT_PRESENT;
    err_code = sd_ble_enable(&ble_enable_params);
    APP_ERROR_CHECK(err_code);
    
    // Subscribe for BLE events.
    err_code = softdevice_ble_evt_handler_set(ble_evt_dispatch);
    APP_ERROR_CHECK(err_code);
}


static void button_event_handler(uint8_t pin_no, uint8_t button_action)
{
    if (button_action == APP_BUTTON_PUSH)
    {
        switch (pin_no)
        {
            case ACTION_BUTTON_PIN:
                nrf_gpio_pin_toggle(DEBUG_GPIO_LED_PIN);


                static char *data_array = "swag\r\n";
                uint32_t err_code = ble_nus_send_string(&m_nus, data_array, strlen(data_array));
                if (err_code != NRF_ERROR_INVALID_STATE)
                {
                    APP_ERROR_CHECK(err_code);
                }

                break;

            default:
                //APP_ERROR_HANDLER(pin_no);
                break;
        }
    }
}

static void gpiote_init(void)
{
    APP_GPIOTE_INIT(APP_GPIOTE_MAX_USERS);
}

/**@brief  Function for configuring the buttons.
 */
static void buttons_init(void)
{
    static app_button_cfg_t buttons[] =
    {
        {WAKEUP_BUTTON_PIN,  APP_BUTTON_ACTIVE_LOW, NRF_GPIO_PIN_PULLUP, button_event_handler},
        {ACTION_BUTTON_PIN,  APP_BUTTON_ACTIVE_LOW, NRF_GPIO_PIN_PULLUP, button_event_handler}
    };
    APP_BUTTON_INIT(buttons, sizeof(buttons) / sizeof(buttons[0]), BUTTON_DETECTION_DELAY, false);
    nrf_gpio_pin_toggle(DEBUG_GPIO_LED_PIN);

    uint32_t err_code = app_button_enable();
    APP_ERROR_CHECK(err_code);
}


/**@brief  Function for placing the application in low power state while waiting for events.
 */
static void power_manage(void)
{
    uint32_t err_code = sd_app_evt_wait();
    APP_ERROR_CHECK(err_code);
}


/**@brief  Function for initializing the UART module.
 */
static void uart_init(void)
{
    /**@snippet [UART Initialization] */
    simple_uart_config(RTS_PIN_NUMBER, TX_PIN_NUMBER, CTS_PIN_NUMBER, RX_PIN_NUMBER, false);
    
    NRF_UART0->INTENSET = UART_INTENSET_RXDRDY_Enabled << UART_INTENSET_RXDRDY_Pos;
    
    NVIC_SetPriority(UART0_IRQn, APP_IRQ_PRIORITY_LOW);
    NVIC_EnableIRQ(UART0_IRQn);
    /**@snippet [UART Initialization] */
}


/**@brief   Function for handling UART interrupts.
 *
 * @details This function will receive a single character from the UART and append it to a string.
 *          The string will be be sent over BLE when the last character received was a 'new line'
 *          i.e '\n' (hex 0x0D) or if the string has reached a length of @ref NUS_MAX_DATA_LENGTH.
 */
void UART0_IRQHandler(void)
{
    static uint8_t data_array[BLE_NUS_MAX_DATA_LEN];
    static uint8_t index = 0;
    uint32_t err_code;

    /**@snippet [Handling the data received over UART] */

    data_array[index] = simple_uart_get();
    index++;

    if ((data_array[index - 1] == '\n') || (index >= (BLE_NUS_MAX_DATA_LEN - 1)))
    {
        err_code = ble_nus_send_string(&m_nus, data_array, index + 1);
        if (err_code != NRF_ERROR_INVALID_STATE)
        {
            APP_ERROR_CHECK(err_code);
        }
        
        index = 0;
    }

    /**@snippet [Handling the data received over UART] */
}






void rgb_lcd_command(uint8_t value)
{
    unsigned char dta[2] = {0x80, value};
    twi_master_transfer (LCD_ADDRESS, dta, 2, true);
}

// send data
size_t rgb_lcd_write(uint8_t value)
{

    unsigned char dta[2] = {0x40, value};
// ardunio:    i2c_send_byteS(dta, 2);
    twi_master_transfer (LCD_ADDRESS, dta, 2, true);
    return 1; // assume sucess
}

void rgb_lcd_setReg(unsigned char addr, unsigned char value)
{
    unsigned char dta[2] = {addr, value};
    twi_master_transfer (RGB_ADDRESS, dta, 2, true);
//    Wire.beginTransmission(RGB_ADDRESS); // transmit to device #4
//    Wire.write(addr);
//    Wire.write(dta);
//    Wire.endTransmission();    // stop transmitting
}

void rgb_lcd_setRGB(unsigned char r, unsigned char g, unsigned char b)
{
    rgb_lcd_setReg(REG_RED,   r);
    rgb_lcd_setReg(REG_GREEN, g);
    rgb_lcd_setReg(REG_BLUE,  b);
}

const unsigned char color_define[4][3] =
{
    {255, 255, 255},             // white
    {255, 0,   0},               // red
    {0,   255, 0},               // green
    {0,   0,   255},             // blue
    {0,   255, 255},             // yellow
};

void rgb_lcd_setColor(unsigned char color)
{
    if(color > 3)return ;
    rgb_lcd_setRGB(color_define[color][0], color_define[color][1], color_define[color][2]);
}


uint8_t _displayfunction;
uint8_t _displaycontrol;
uint8_t _displaymode;

uint8_t _initialized;

uint8_t _numlines,_currline;

void rgb_lcd_display()
{
    _displaycontrol |= LCD_DISPLAYON;
    rgb_lcd_command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void rgb_lcd_clear()
{
    rgb_lcd_command(LCD_CLEARDISPLAY);
}

void rgb_lcd_home()
{
    rgb_lcd_command(LCD_RETURNHOME);
    nrf_delay_ms(2);
}

void rgb_set_cursor(uint8_t col, uint8_t row)
{
    col = (row == 0 ? col | 0x80 : col | 0xc0);
    unsigned char dta[2] = {0x80, col};
    twi_master_transfer (LCD_ADDRESS, dta, 2, true);
}


void rgb_lcd_default()
{
    rgb_lcd_setRGB(0, 232, 181);
}
void rgb_lcd_connected()
{
    rgb_lcd_setRGB(0, 232, 181);
    rgb_lcd_clear();
}
void rgb_lcd_sleep()
{
    rgb_lcd_setRGB(230, 0, 233);
    rgb_lcd_clear();
    rgb_lcd_write('z');
    rgb_lcd_write('Z');
    rgb_lcd_write('z');
    rgb_lcd_write('Z');
}
void rgb_lcd_error()
{
    rgb_lcd_setRGB(232, 207, 0);
    rgb_lcd_clear();
    rgb_lcd_write('E');
    rgb_lcd_write('R');
    rgb_lcd_write('R');
}

void rgb_lcd_wash_open()
{
    rgb_lcd_setRGB(71, 233, 0);
}

void rgb_lcd_wash_closed()
{
    rgb_lcd_setRGB(233, 0, 0);
}

void rgb_lcd_begin()
{
    _displayfunction = LCD_2LINE;
    nrf_delay_ms(50);
    rgb_lcd_command(LCD_FUNCTIONSET | _displayfunction);
    nrf_delay_ms(5);
    rgb_lcd_command(LCD_FUNCTIONSET | _displayfunction);
    nrf_delay_ms(2);
    rgb_lcd_command(LCD_FUNCTIONSET | _displayfunction);
    rgb_lcd_command(LCD_FUNCTIONSET | _displayfunction);

    _displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
    rgb_lcd_display();

    rgb_lcd_clear();
    nrf_delay_ms(2);

    _displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
    rgb_lcd_command(LCD_ENTRYMODESET | _displaymode);

    rgb_lcd_setReg(0, 0);
    rgb_lcd_setReg(1, 0);
    rgb_lcd_setReg(0x08, 0xAA);

    nrf_delay_ms(1);
    rgb_lcd_default();

    nrf_delay_ms(1);

    rgb_lcd_write('O');
    rgb_lcd_write('K');
}


void rgb_lcd_print()
{

}



void twi_init()
{
    if (!twi_master_init()) {
        APP_ERROR_CHECK(1);
    }


    char data[1];
    data[0] = LCD_DISPLAYON;

    rgb_lcd_begin();

   // nrf_gpio_pin_set(CONNECTED_LED_PIN_NO);
}

void nus_data_handler(ble_nus_t * p_nus, uint8_t * p_data, uint16_t length)
{
    nrf_gpio_pin_toggle(CONNECTED_LED_PIN_NO);
    for (int i = 0; i < length; i++)
    {
        if (p_data[i] == 1) {
            rgb_lcd_clear();
        } else if (p_data[i] == 2) {
            rgb_set_cursor(0, 0);
        } else if (p_data[i] == 3) {
            rgb_set_cursor(0, 1);
        } else if (p_data[i] == 4) {
            rgb_lcd_setColor(0);
        } else if (p_data[i] == 5) {
            rgb_lcd_setColor(1);
        } else if (p_data[i] == 6) {
            rgb_lcd_wash_open();
        } else if (p_data[i] == 7) {
            rgb_lcd_wash_closed();
        } else {
            rgb_lcd_write(p_data[i]);
        }
    }
}

/**@brief  Application main function.
 */
int main(void)
{
    // Initialize
    app_trace_init();
    leds_init();
    timers_init();
    gpiote_init();
    buttons_init();
    uart_init();
    ble_stack_init();
    gap_params_init();
    services_init();
    advertising_init();
    conn_params_init();
    sec_params_init();
    twi_init();

    advertising_start();

    // Enter main loop
    for (;;)
    {
         power_manage();
        //simple_uart_put('L');
        //simple_uart_put('X');
    }
}

/** 
 * @}
 */
