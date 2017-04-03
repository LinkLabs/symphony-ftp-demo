// Symphony FTP Demo
// Link Labs

// For in-depth explanation of this program,
// check out http://docs.link-labs.com

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <sys/time.h>
#include <stdbool.h>

#include "ll_ifc.h"
#include "ll_ifc_ftp.h"
#include "ll_ifc_symphony.h"
#include "ll_ifc_transport_pc.h"

// Uncomment for debug text
#define DEBUG

////////////////////////////////////////////////////////////////////////////////
// FTP Variables
////////////////////////////////////////////////////////////////////////////////
static ll_ftp_t           ftp;
static ll_ftp_callbacks_t ftp_callbacks;
static FILE *             storage;

////////////////////////////////////////////////////////////////////////////////
// Utility Definitions
////////////////////////////////////////////////////////////////////////////////
static const char *ftp_return_translate[] = {"LL_FTP_OK, Success.\n", "LL_FTP_OOR, Out of range.\n",
                                             "LL_FTP_INVALID_VALUE, Invalid input value.\n",
                                             "LL_FTP_NO_ACTION, No action.\n",
                                             "LL_FTP_ERROR Error.\n"};

static const char *ftp_state_translate[] = {"IDLE\n", "SEGMENT\n", "APPLY\n"};

////////////////////////////////////////////////////////////////////////////////
// Utiilty Functions
////////////////////////////////////////////////////////////////////////////////
static void print_irq_flags_text(uint32_t flags);
static void print_ll_ftp_error(char *label, ll_ftp_return_code_t ret_val);
static void print_ll_ifc_error(char *label, int32_t ret_val);

////////////////////////////////////////////////////////////////////////////////
// Symphony FTP Callbacks
////////////////////////////////////////////////////////////////////////////////
static ll_ftp_return_code_t ftp_open_callback(uint32_t file_id, uint32_t file_version,
                                              uint32_t file_size)
{
#ifdef DEBUG
    printf("Open Callback Called!\n");
#endif

    storage = fopen("fota_dump.txt", "rb+");
    if (storage == NULL)
    {
        storage = fopen("fota_dump.txt", "wb");
    }

    ll_ftp_return_code_t func_ret = LL_FTP_OK;

#ifdef DEBUG
    printf("Open Callback returned %s\n", ftp_return_translate[func_ret]);
#endif

    return func_ret;
}

static ll_ftp_return_code_t ftp_read_callback(uint32_t file_id, uint32_t file_version,
                                              uint32_t offset, uint8_t *payload, uint16_t len)
{
#ifdef DEBUG
    printf("Read Callback Called!\n");
#endif

    ll_ftp_return_code_t func_ret = LL_FTP_OK;

    fseek(storage, offset, SEEK_SET);
    fread(payload, sizeof(uint8_t), len, storage);

#ifdef DEBUG
    printf("Read Callback returned %s\n", ftp_return_translate[func_ret]);
#endif

    return func_ret;
}

static ll_ftp_return_code_t ftp_write_callback(uint32_t file_id, uint32_t file_version,
                                               uint32_t offset, uint8_t *payload, uint16_t len)
{
#ifdef DEBUG
    printf("Write Callback Called!\n");
#endif

    fseek(storage, offset, SEEK_SET);
    fwrite(payload, sizeof(uint8_t), len, storage);

    ll_ftp_return_code_t func_ret = LL_FTP_OK;

#ifdef DEBUG
    printf("Write Callback returned %s\n", ftp_return_translate[func_ret]);
#endif

    return func_ret;
}

static ll_ftp_return_code_t ftp_close_callback(uint32_t file_id, uint32_t file_version)
{
#ifdef DEBUG
    printf("Close Callback Called!\n");
#endif

    ll_ftp_return_code_t func_ret = LL_FTP_OK;
    fclose(storage);

#ifdef DEBUG
    printf("Close Callback returned %s\n", ftp_return_translate[func_ret]);
#endif

    return func_ret;
}

static ll_ftp_return_code_t ftp_apply_callback(uint32_t file_id, uint32_t file_version,
                                               uint32_t file_size)
{
#ifdef DEBUG
    printf("Apply Callback Called!\n");
#endif

    ll_ftp_return_code_t func_ret = LL_FTP_OK;

#ifdef DEBUG
    printf("Apply Callback returned %s\n", ftp_return_translate[func_ret]);
#endif

    exit(EXIT_SUCCESS);
    return func_ret;
}

static ll_ftp_return_code_t ftp_send_uplink_callback(const uint8_t *buf, uint8_t len, bool acked,
                                                     uint8_t port)
{
#ifdef DEBUG
    printf("Send Uplink Callback Called!\n");
    printf("\tport: %i, acked: %i, len: %i\n\tbuffer: ", port, acked, len);
    for (uint16_t i = 0; i < len; i++)
    {
        printf("%02X", buf[i]);
    }
    printf("\n");
#endif

    int32_t ret = ll_message_send(buf, len, acked, port);
    print_ll_ifc_error("uplink send", ret);

    ll_ftp_return_code_t func_ret = (ret == 0) ? LL_FTP_OK : LL_FTP_ERROR;
#ifdef DEBUG
    printf("Send Uplink Callback returned %s\n", ftp_return_translate[func_ret]);
#endif

    return func_ret;
}

static ll_ftp_return_code_t ftp_dl_config_callback(bool downlink_on)
{
#ifdef DEBUG
    printf("Downlink Config Callback Called!\n");
#endif

    uint8_t               app_token[APP_TOKEN_LEN], qos;
    enum ll_downlink_mode dl_mode;
    uint32_t              net_token, ret;

    ret = ll_config_get(&net_token, app_token, &dl_mode, &qos);
    print_ll_ifc_error("config get", ret);
    assert(ret >= 0);

    dl_mode = (downlink_on) ? LL_DL_ALWAYS_ON : LL_DL_MAILBOX;

    ret = ll_config_set(net_token, app_token, dl_mode, 0);
    print_ll_ifc_error("config set", ret);
    assert(ret >= 0);

    ll_ftp_return_code_t func_ret = LL_FTP_OK;

#ifdef DEBUG
    printf("Downlink Config Callback returned %s\n", ftp_return_translate[func_ret]);
#endif

    return func_ret;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        puts("ERROR: Not Enough Args");
        puts("Usage: ./symphony-ftp-demo {tty handle}");
        return EXIT_FAILURE;
    }

    uint8_t               app_token[APP_TOKEN_LEN], qos;
    enum ll_downlink_mode dl_mode;
    uint32_t              net_token, ret;
    uint8_t               count = 0;

    // Register FTP Callbacks
    ftp_callbacks.open   = ftp_open_callback;
    ftp_callbacks.read   = ftp_read_callback;
    ftp_callbacks.write  = ftp_write_callback;
    ftp_callbacks.close  = ftp_close_callback;
    ftp_callbacks.apply  = ftp_apply_callback;
    ftp_callbacks.uplink = ftp_send_uplink_callback;
    ftp_callbacks.config = ftp_dl_config_callback;

    ret = ll_ftp_init(&ftp, &ftp_callbacks);
    print_ll_ftp_error("ll_ftp_init", ret);
    assert(ret >= 0);

    // Open the tty to talk to the module
    ret = ll_tty_open(argv[1], 115200);
    print_ll_ifc_error("ll_tty_open", ret);
    assert(ret >= 0);

    while (true)
    {
        sleep(1);
        uint32_t irq_flags;
        ret = ll_irq_flags(0, &irq_flags);
        assert(ret >= 0);

        if (irq_flags & IRQ_FLAGS_RX_DONE)
        {
            // Clear IRQ Flags
            ret = ll_irq_flags(IRQ_FLAGS_RX_DONE, &irq_flags);
            print_ll_ifc_error("ll_irq_flags", ret);
            assert(ret >= 0);

            // Polling Recieve Message
            while (ret >= LL_IFC_ACK)
            {
                uint8_t  buff[256];
                int16_t  rssi;
                uint8_t  raw_snr;
                uint8_t  port;
                uint16_t len = sizeof(buff);

                ret = ll_retrieve_message(buff, &len, &port, &rssi, &raw_snr);
                if (ret < LL_IFC_ACK || len == 0)
                {
                    break;
                }

                puts("--------------------------------------------------------------");
                printf("len: %i, port: %i, rssi:%i, raw_snr: %i\nbuffer: ", len, port, rssi,
                       raw_snr);
                for (int i = 0; i < len; i++)
                {
                    printf("%02X", buff[i]);
                }
                puts("\n--------------------------------------------------------------\n");

                printf("Recieved %i segments out of %i!\n\n",
                       ftp.num_segs - ll_ftp_num_missing_segs_get(&ftp), ftp.num_segs);

                if (128 == port)
                {
                    ll_ftp_return_code_t ftp_ret = ll_ftp_msg_process(&ftp, buff, len);
                    print_ll_ftp_error("ll_ftp_msg_process", ftp_ret);
                    assert(ret >= 0);
                }
            }
        }
        else
        {
            ll_ftp_msg_process(&ftp, NULL, 0);
        }
    }
    return 0;
}

static void print_irq_flags_text(uint32_t flags)
{
    // Most significant bit to least significant bit
    if (IRQ_FLAGS_ASSERT & flags)
    {
        printf("[IRQ_FLAGS_ASSERT]");
    }
    if (IRQ_FLAGS_APP_TOKEN_ERROR & flags)
    {
        printf("[IRQ_FLAGS_APP_TOKEN_ERROR]");
    }
    if (IRQ_FLAGS_CRYPTO_ERROR & flags)
    {
        printf("[IRQ_FLAGS_CRYPTO_ERROR]");
    }
    if (IRQ_FLAGS_DOWNLINK_REQUEST_ACK & flags)
    {
        printf("[IRQ_FLAGS_DOWNLINK_REQUEST_ACK]");
    }
    if (IRQ_FLAGS_INITIALIZATION_COMPLETE & flags)
    {
        printf("[IRQ_FLAGS_INITIALIZATION_COMPLETE]");
    }
    if (IRQ_FLAGS_APP_TOKEN_CONFIRMED & flags)
    {
        printf("[IRQ_FLAGS_APP_TOKEN_CONFIRMED]");
    }
    if (IRQ_FLAGS_CRYPTO_ESTABLISHED & flags)
    {
        printf("[IRQ_FLAGS_CRYPTO_ESTABLISHED]");
    }
    if (IRQ_FLAGS_DISCONNECTED & flags)
    {
        printf("[IRQ_FLAGS_DISCONNECTED]");
    }
    if (IRQ_FLAGS_CONNECTED & flags)
    {
        printf("[IRQ_FLAGS_CONNECTED]");
    }
    if (IRQ_FLAGS_RX_DONE & flags)
    {
        printf("[IRQ_FLAGS_RX_DONE]");
    }
    if (IRQ_FLAGS_TX_ERROR & flags)
    {
        printf("[IRQ_FLAGS_TX_ERROR]");
    }
    if (IRQ_FLAGS_TX_DONE & flags)
    {
        printf("[IRQ_FLAGS_TX_DONE]");
    }
    if (IRQ_FLAGS_RESET & flags)
    {
        printf("[IRQ_FLAGS_RESET]");
    }
    if (IRQ_FLAGS_WDOG_RESET & flags)
    {
        printf("[IRQ_FLAGS_WDOG_RESET]");
    }
    if (flags != 0)
    {
        printf("\n");
    }
}

static void print_ll_ftp_error(char *label, ll_ftp_return_code_t ret_val)
{
    if (ret_val != LL_FTP_OK)
    {
        fprintf(stderr, "ERROR(%s): %s", label, ftp_return_translate[ret_val]);
    }
}

static void print_ll_ifc_error(char *label, int32_t ret_val)
{
    if (ret_val < 0)
    {
        fprintf(stderr, "ERROR(%s): Host interface - ", label);

        // Map Error code to NACK code
        if (ret_val >= -99 && ret_val <= -1)
        {
            ret_val = 0 - ret_val;
        }
        switch (ret_val)
        {
            case LL_IFC_NACK_CMD_NOT_SUPPORTED:
                fprintf(stderr, "NACK received - Command not supported");
                break;
            case LL_IFC_NACK_INCORRECT_CHKSUM:
                fprintf(stderr, "NACK received - Incorrect Checksum");
                break;
            case LL_IFC_NACK_PAYLOAD_LEN_OOR:
                fprintf(stderr, "NACK received - Payload length out of range");
                break;
            case LL_IFC_NACK_PAYLOAD_OOR:
                fprintf(stderr, "NACK received - Payload out of range");
                break;
            case LL_IFC_NACK_BOOTUP_IN_PROGRESS:
                fprintf(stderr, "NACK received - Not allowed, bootup in progress");
                break;
            case LL_IFC_NACK_BUSY_TRY_AGAIN:
                fprintf(stderr, "NACK received - Busy try again");
                break;
            case LL_IFC_NACK_APP_TOKEN_REG:
                fprintf(stderr, "NACK received - Application Token not registered");
                break;
            case LL_IFC_NACK_PAYLOAD_LEN_EXCEEDED:
                fprintf(stderr, "NACK received - Payload length greater than maximum");
                break;
            case LL_IFC_NACK_NOT_IN_MAILBOX_MODE:
                fprintf(stderr, "NACK received - Module is not in DOWNLINK_MAILBOX mode");
                break;
            case LL_IFC_NACK_NODATA:
                fprintf(stderr, "NACK received - No data available");
                break;
            case LL_IFC_NACK_OTHER:
                fprintf(stderr, "NACK received - Other");
                break;
            case LL_IFC_ERROR_INCORRECT_PARAMETER:
                fprintf(stderr, "Invalid Parameter");
                break;
            case -103:
                fprintf(stderr, "Message Number mismatch");
                break;
            case -104:
                fprintf(stderr, "Checksum mismatch");
                break;
            case -105:
                fprintf(stderr, "Command mismatch");
                break;
            case -106:
                fprintf(stderr, "Timed out");
                break;
            case -107:
                fprintf(stderr, "Payload larger than buffer provided");
                break;
            default:
                break;
        }
        fprintf(stderr, "\n");
    }
}
