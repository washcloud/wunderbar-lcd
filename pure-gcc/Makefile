# List all source files the application uses.
APPLICATION_SRCS = $(notdir $(wildcard ../*.c))


APPLICATION_SRCS += app_button.c
APPLICATION_SRCS += app_gpiote.c
APPLICATION_SRCS += app_timer.c
APPLICATION_SRCS += app_trace.c
APPLICATION_SRCS += ble_advdata.c
APPLICATION_SRCS += ble_bas.c
APPLICATION_SRCS += ble_conn_params.c
APPLICATION_SRCS += ble_debug_assert_handler.c
APPLICATION_SRCS += ble_dis.c
APPLICATION_SRCS += ble_error_log.c
APPLICATION_SRCS += ble_hts.c
APPLICATION_SRCS += ble_sensorsim.c
APPLICATION_SRCS += ble_srv_common.c
APPLICATION_SRCS += crc16.c
APPLICATION_SRCS += device_manager_peripheral.c
APPLICATION_SRCS += nrf_delay.c
APPLICATION_SRCS += pstorage.c
APPLICATION_SRCS += simple_uart.c
APPLICATION_SRCS += softdevice_handler.c
APPLICATION_SRCS += twi_sw_master.c

PROJECT_NAME = button

DEVICE = NRF51
BOARD = BOARD_AIRFY
#SEGGER_SERIAL =

USE_SOFTDEVICE = s110
LINKER_SCRIPT=gcc_nrf51_s120v7.ld
SOFTDEVICE=/home/aep/proj/nordic/sds/s110_nrf51822_7.0.0_softdevice.hex

SDK_PATH = /home/aep/proj/nordic/sdk/nrf51822/
TEMPLATE_PATH = /home/aep/proj/nordic/nrf51-pure-gcc-setup/template/

CFLAGS = -g3 -O0
LDFLAGS = -g3 -O0

GDB_PORT_NUMBER = 2331

include $(TEMPLATE_PATH)Makefile
