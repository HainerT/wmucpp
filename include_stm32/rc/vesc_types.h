#pragma once

namespace RC::VESC {
    // Communication commands
    enum class  CommPacketId {
        COMM_FW_VERSION = 0,
        COMM_JUMP_TO_BOOTLOADER,
        COMM_ERASE_NEW_APP,
        COMM_WRITE_NEW_APP_DATA,
        COMM_GET_VALUES,
        COMM_SET_DUTY,
        COMM_SET_CURRENT,
        COMM_SET_CURRENT_BRAKE,
        COMM_SET_RPM,
        COMM_SET_POS,
        COMM_SET_HANDBRAKE,
        COMM_SET_DETECT,
        COMM_SET_SERVO_POS,
        COMM_SET_MCCONF,
        COMM_GET_MCCONF,
        COMM_GET_MCCONF_DEFAULT,
        COMM_SET_APPCONF,
        COMM_GET_APPCONF,
        COMM_GET_APPCONF_DEFAULT,
        COMM_SAMPLE_PRINT,
        COMM_TERMINAL_CMD,
        COMM_PRINT,
        COMM_ROTOR_POSITION,
        COMM_EXPERIMENT_SAMPLE,
        COMM_DETECT_MOTOR_PARAM,
        COMM_DETECT_MOTOR_R_L,
        COMM_DETECT_MOTOR_FLUX_LINKAGE,
        COMM_DETECT_ENCODER,
        COMM_DETECT_HALL_FOC,
        COMM_REBOOT,
        COMM_ALIVE,
        COMM_GET_DECODED_PPM,
        COMM_GET_DECODED_ADC,
        COMM_GET_DECODED_CHUK,
        COMM_FORWARD_CAN,
        COMM_SET_CHUCK_DATA,
        COMM_CUSTOM_APP_DATA,
        COMM_NRF_START_PAIRING,
        COMM_GPD_SET_FSW,
        COMM_GPD_BUFFER_NOTIFY,
        COMM_GPD_BUFFER_SIZE_LEFT,
        COMM_GPD_FILL_BUFFER,
        COMM_GPD_OUTPUT_SAMPLE,
        COMM_GPD_SET_MODE,
        COMM_GPD_FILL_BUFFER_INT8,
        COMM_GPD_FILL_BUFFER_INT16,
        COMM_GPD_SET_BUFFER_INT_SCALE,
        COMM_GET_VALUES_SETUP,
        COMM_SET_MCCONF_TEMP,
        COMM_SET_MCCONF_TEMP_SETUP,
        COMM_GET_VALUES_SELECTIVE,
        COMM_GET_VALUES_SETUP_SELECTIVE,
        COMM_EXT_NRF_PRESENT,
        COMM_EXT_NRF_ESB_SET_CH_ADDR,
        COMM_EXT_NRF_ESB_SEND_DATA,
        COMM_EXT_NRF_ESB_RX_DATA,
        COMM_EXT_NRF_SET_ENABLED,
        COMM_DETECT_MOTOR_FLUX_LINKAGE_OPENLOOP,
        COMM_DETECT_APPLY_ALL_FOC,
        COMM_JUMP_TO_BOOTLOADER_ALL_CAN,
        COMM_ERASE_NEW_APP_ALL_CAN,
        COMM_WRITE_NEW_APP_DATA_ALL_CAN,
        COMM_PING_CAN,
        COMM_APP_DISABLE_OUTPUT,
        COMM_TERMINAL_CMD_SYNC,
        COMM_GET_IMU_DATA,
        COMM_BM_CONNECT,
        COMM_BM_ERASE_FLASH_ALL,
        COMM_BM_WRITE_FLASH,
        COMM_BM_REBOOT,
        COMM_BM_DISCONNECT,
        COMM_BM_MAP_PINS_DEFAULT,
        COMM_BM_MAP_PINS_NRF5X,
        COMM_ERASE_BOOTLOADER,
        COMM_ERASE_BOOTLOADER_ALL_CAN,
        COMM_PLOT_INIT,
        COMM_PLOT_DATA,
        COMM_PLOT_ADD_GRAPH,
        COMM_PLOT_SET_GRAPH,
        COMM_GET_DECODED_BALANCE,
        COMM_BM_MEM_READ,
        COMM_WRITE_NEW_APP_DATA_LZO,
        COMM_WRITE_NEW_APP_DATA_ALL_CAN_LZO,
        COMM_BM_WRITE_FLASH_LZO,
        COMM_SET_CURRENT_REL,
        COMM_CAN_FWD_FRAME,
        COMM_SET_BATTERY_CUT,
        COMM_SET_BLE_NAME,
        COMM_SET_BLE_PIN,
        COMM_SET_CAN_MODE,
        COMM_GET_IMU_CALIBRATION,
        COMM_GET_MCCONF_TEMP,

        // Custom configuration for hardware
        COMM_GET_CUSTOM_CONFIG_XML,
        COMM_GET_CUSTOM_CONFIG,
        COMM_GET_CUSTOM_CONFIG_DEFAULT,
        COMM_SET_CUSTOM_CONFIG,

        // BMS commands
        COMM_BMS_GET_VALUES,
        COMM_BMS_SET_CHARGE_ALLOWED,
        COMM_BMS_SET_BALANCE_OVERRIDE,
        COMM_BMS_RESET_COUNTERS,
        COMM_BMS_FORCE_BALANCE,
        COMM_BMS_ZERO_CURRENT_OFFSET,

        // FW updates commands for different HW types
        COMM_JUMP_TO_BOOTLOADER_HW,
        COMM_ERASE_NEW_APP_HW,
        COMM_WRITE_NEW_APP_DATA_HW,
        COMM_ERASE_BOOTLOADER_HW,
        COMM_JUMP_TO_BOOTLOADER_ALL_CAN_HW,
        COMM_ERASE_NEW_APP_ALL_CAN_HW,
        COMM_WRITE_NEW_APP_DATA_ALL_CAN_HW,
        COMM_ERASE_BOOTLOADER_ALL_CAN_HW,

        COMM_SET_ODOMETER,

        // Power switch commands
        COMM_PSW_GET_STATUS,
        COMM_PSW_SWITCH,

        COMM_BMS_FWD_CAN_RX,
        COMM_BMS_HW_DATA,
        COMM_GET_BATTERY_CUT,
        COMM_BM_HALT_REQ,
        COMM_GET_QML_UI_HW,
        COMM_GET_QML_UI_APP,
        COMM_CUSTOM_HW_DATA,
        COMM_QMLUI_ERASE,
        COMM_QMLUI_WRITE,

        // IO Board
        COMM_IO_BOARD_GET_ALL,
        COMM_IO_BOARD_SET_PWM,
        COMM_IO_BOARD_SET_DIGITAL,

        COMM_BM_MEM_WRITE,
        COMM_BMS_BLNC_SELFTEST,
        COMM_GET_EXT_HUM_TMP,
        COMM_GET_STATS,
        COMM_RESET_STATS,

        // Lisp
        COMM_LISP_READ_CODE,
        COMM_LISP_WRITE_CODE,
        COMM_LISP_ERASE_CODE,
        COMM_LISP_SET_RUNNING,
        COMM_LISP_GET_STATS,
        COMM_LISP_PRINT,

        COMM_BMS_SET_BATT_TYPE,
        COMM_BMS_GET_BATT_TYPE,

        COMM_LISP_REPL_CMD,
    };
}
