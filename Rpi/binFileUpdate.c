/*
 * binFileUpdate.c
 * Raspberry Pi to STM32 Firmware Update via UART
 *
 * Author: Improved version
 * Date: 2024
 *
 * Description:
 * This program sends binary firmware files from Raspberry Pi to STM32
 * microcontrollers over UART. It supports both bootloader and application
 * updates with checksums and retry mechanisms.
 */

#include "RpiUart.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

/* Configuration */
#define MAX_BLOCK_SIZE          1024
#define MAX_FW_SIZE             (MAX_BLOCK_SIZE * 256)  // 256KB max
#define HANDSHAKE_TIMEOUT_MS    5000
#define BLOCK_TIMEOUT_MS        2000
#define MAX_RETRY_COUNT         5
#define PROGRESS_BAR_WIDTH      50

/* Protocol bytes */
#define PROTO_START_BYTE        '{'
#define PROTO_END_BYTE          '}'
#define PROTO_ACK_BYTE          'O'
#define PROTO_NACK_BYTE         'N'

/* Global variables */
static uint8_t  g_bin_file[MAX_FW_SIZE];
static uint32_t g_bin_file_size = 0;
static int      g_comport = -1;

/* Function prototypes */
static bool read_binary_file(const char *filename);
static bool send_handshake(void);
static bool send_block(uint32_t block_num, uint32_t block_size, uint32_t offset);
static void print_progress(uint32_t current, uint32_t total);
static uint8_t calculate_checksum(uint8_t first, uint8_t last);
static bool wait_for_response(uint8_t expected, uint32_t timeout_ms);
static void print_usage(const char *prog_name);

/*
 * Main function
 */
int main(int argc, char *argv[])
{
    int     baudrate = 115200;
    char    mode[] = {'8', 'N', '1', 0};
    char    bin_filename[256];
    bool    success = true;

    /* Check arguments */
    if(argc < 3)
    {
        print_usage(argv[0]);
        return -1;
    }

    /* Parse arguments */
    strncpy(bin_filename, argv[1], sizeof(bin_filename) - 1);
    g_comport = atoi(argv[2]) - 1;

    printf("\n");
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║         RPI to STM32 Firmware Update Tool v2.0                ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");
    printf("\n");

    /* Open UART port */
    printf("[1/5] Opening UART port %d...\n", g_comport + 1);
    if(RpiUart_OpenComport(g_comport, baudrate, mode, 0))
    {
        printf("ERROR: Cannot open COM port %d\n", g_comport + 1);
        return -1;
    }
    printf("      ✓ UART port opened successfully (115200 8N1)\n\n");

    /* Flush any pending data */
    RpiUart_flushRXTX(g_comport);
    usleep(100000);  // 100ms delay

    /* Read binary file */
    printf("[2/5] Reading binary file: %s\n", bin_filename);
    if(!read_binary_file(bin_filename))
    {
        RpiUart_CloseComport(g_comport);
        return -1;
    }
    printf("      ✓ File loaded: %u bytes (%.2f KB)\n\n",
           g_bin_file_size, (float)g_bin_file_size / 1024.0);

    /* Send handshake */
    printf("[3/5] Performing handshake with STM32...\n");
    if(!send_handshake())
    {
        printf("ERROR: Handshake failed!\n");
        RpiUart_CloseComport(g_comport);
        return -1;
    }
    printf("      ✓ Handshake successful\n\n");

    /* Calculate blocks */
    uint32_t total_blocks = (g_bin_file_size + MAX_BLOCK_SIZE - 1) / MAX_BLOCK_SIZE;
    printf("[4/5] Transferring firmware (%u blocks)...\n", total_blocks);

    /* Send all blocks */
    uint32_t offset = 0;
    for(uint32_t block = 0; block < total_blocks; block++)
    {
        uint32_t block_size = MAX_BLOCK_SIZE;
        if(offset + block_size > g_bin_file_size)
        {
            block_size = g_bin_file_size - offset;
        }

        /* Print progress */
        print_progress(block + 1, total_blocks);

        /* Send block with retry */
        bool block_success = false;
        for(int retry = 0; retry < MAX_RETRY_COUNT; retry++)
        {
            if(send_block(block, block_size, offset))
            {
                block_success = true;
                break;
            }

            if(retry < MAX_RETRY_COUNT - 1)
            {
                printf("\n      ⚠ Block %u failed, retrying... (%d/%d)\n",
                       block, retry + 1, MAX_RETRY_COUNT);
                usleep(100000);  // 100ms delay before retry
            }
        }

        if(!block_success)
        {
            printf("\n\nERROR: Failed to send block %u after %d retries\n",
                   block, MAX_RETRY_COUNT);
            success = false;
            break;
        }

        offset += block_size;
    }

    printf("\n\n");

    /* Wait for bootloader to finish */
    if(success)
    {
        printf("[5/5] Waiting for STM32 to complete...\n");
        usleep(500000);  // 500ms delay
        printf("      ✓ Transfer completed successfully!\n\n");

        printf("╔════════════════════════════════════════════════════════════════╗\n");
        printf("║                  FIRMWARE UPDATE SUCCESSFUL                    ║\n");
        printf("╠════════════════════════════════════════════════════════════════╣\n");
        printf("║  Total size:    %-10u bytes                              ║\n", g_bin_file_size);
        printf("║  Total blocks:  %-10u                                    ║\n", total_blocks);
        printf("║  Block size:    %-10d bytes                              ║\n", MAX_BLOCK_SIZE);
        printf("╚════════════════════════════════════════════════════════════════╝\n");
    }
    else
    {
        printf("╔════════════════════════════════════════════════════════════════╗\n");
        printf("║                   FIRMWARE UPDATE FAILED                       ║\n");
        printf("╚════════════════════════════════════════════════════════════════╝\n");
    }

    printf("\n");

    /* Close UART */
    RpiUart_CloseComport(g_comport);

    return success ? 0 : -1;
}

/*
 * Read binary file into buffer
 */
static bool read_binary_file(const char *filename)
{
    FILE *fp = fopen(filename, "rb");
    if(!fp)
    {
        printf("ERROR: Cannot open file: %s\n", filename);
        return false;
    }

    /* Get file size */
    fseek(fp, 0L, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    /* Check ftell() error */
    if(file_size < 0)
    {
        printf("ERROR: Cannot determine file size\n");
        fclose(fp);
        return false;
    }

    g_bin_file_size = (uint32_t)file_size;

    /* Check size */
    if(g_bin_file_size == 0)
    {
        printf("ERROR: File is empty\n");
        fclose(fp);
        return false;
    }

    if(g_bin_file_size > MAX_FW_SIZE)
    {
        printf("ERROR: File too large (%u bytes). Max: %u bytes\n",
               g_bin_file_size, MAX_FW_SIZE);
        fclose(fp);
        return false;
    }

    /* Read file */
    size_t bytes_read = fread(g_bin_file, 1, g_bin_file_size, fp);
    fclose(fp);

    if(bytes_read != g_bin_file_size)
    {
        printf("ERROR: Failed to read complete file\n");
        return false;
    }

    return true;
}

/*
 * Send handshake to STM32
 * Protocol: {SIZE} where SIZE is 4 bytes (big-endian)
 */
static bool send_handshake(void)
{
    uint8_t handshake[6];

    /* Prepare handshake packet */
    handshake[0] = PROTO_START_BYTE;
    handshake[1] = (g_bin_file_size >> 24) & 0xFF;
    handshake[2] = (g_bin_file_size >> 16) & 0xFF;
    handshake[3] = (g_bin_file_size >> 8)  & 0xFF;
    handshake[4] = (g_bin_file_size >> 0)  & 0xFF;
    handshake[5] = PROTO_END_BYTE;

    /* Flush RX buffer */
    RpiUart_flushRX(g_comport);

    /* Send handshake */
    for(int i = 0; i < 6; i++)
    {
        if(RpiUart_SendByte(g_comport, handshake[i]))
        {
            printf("ERROR: Failed to send handshake byte %d\n", i);
            return false;
        }
        usleep(1000);  // 1ms delay between bytes
    }

    /* Wait for ACK */
    printf("      Waiting for ACK from STM32...\n");
    if(!wait_for_response(PROTO_ACK_BYTE, HANDSHAKE_TIMEOUT_MS))
    {
        printf("ERROR: No ACK received from STM32\n");
        return false;
    }

    return true;
}

/*
 * Send a single data block
 */
static bool send_block(uint32_t block_num, uint32_t block_size, uint32_t offset)
{
    uint8_t checksum;
    uint8_t response;

    /* Send data */
    for(uint32_t i = 0; i < block_size; i++)
    {
        if(RpiUart_SendByte(g_comport, g_bin_file[offset + i]))
        {
            return false;
        }

        /* Small delay every 64 bytes to prevent buffer overflow */
        if((i % 64) == 63)
        {
            usleep(500);  // 0.5ms
        }
    }

    /* Calculate and wait for checksum */
    checksum = calculate_checksum(g_bin_file[offset],
                                  g_bin_file[offset + block_size - 1]);

    /* Wait for checksum response */
    if(!wait_for_response(checksum, BLOCK_TIMEOUT_MS))
    {
        return false;
    }

    return true;
}

/*
 * Calculate checksum (first byte + last byte)
 */
static uint8_t calculate_checksum(uint8_t first, uint8_t last)
{
    return (first + last) & 0xFF;
}

/*
 * Wait for specific response byte
 */
static bool wait_for_response(uint8_t expected, uint32_t timeout_ms)
{
    uint8_t response;
    struct timespec start, current;
    uint32_t elapsed_ms;

    clock_gettime(CLOCK_MONOTONIC, &start);

    while(1)
    {
        /* Check timeout */
        clock_gettime(CLOCK_MONOTONIC, &current);
        elapsed_ms = (current.tv_sec - start.tv_sec) * 1000 +
                     (current.tv_nsec - start.tv_nsec) / 1000000;

        if(elapsed_ms >= timeout_ms)
        {
            return false;
        }

        /* Try to read */
        int n = RpiUart_PollComport(g_comport, &response, 1);
        if(n > 0 && response == expected)
        {
            return true;
        }

        usleep(1000);  // 1ms delay
    }

    return false;
}

/*
 * Print progress bar
 */
static void print_progress(uint32_t current, uint32_t total)
{
    float percentage = (float)current / (float)total * 100.0;
    int filled = (int)(percentage / 100.0 * PROGRESS_BAR_WIDTH);

    printf("\r      [");
    for(int i = 0; i < PROGRESS_BAR_WIDTH; i++)
    {
        if(i < filled)
            printf("█");
        else
            printf("░");
    }
    printf("] %.1f%% (%u/%u)", percentage, current, total);
    fflush(stdout);
}

/*
 * Print usage information
 */
static void print_usage(const char *prog_name)
{
    printf("Usage: %s <binary_file> <com_port_number>\n", prog_name);
    printf("\n");
    printf("Arguments:\n");
    printf("  <binary_file>       Path to the binary firmware file\n");
    printf("  <com_port_number>   COM port number (1-38)\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s firmware.bin 1         # Use /dev/ttyS0\n", prog_name);
    printf("  %s app.bin 2              # Use /dev/ttyAMA0\n", prog_name);
    printf("  %s bootloader.bin 3       # Use /dev/ttyUSB0\n", prog_name);
    printf("\n");
    printf("COM Port Mapping:\n");
    printf("  1  = /dev/ttyS0      22 = /dev/ttyAMA0\n");
    printf("  2  = /dev/ttyAMA0    23 = /dev/ttyAMA1\n");
    printf("  3  = /dev/ttyUSB0    24 = /dev/ttyACM0\n");
    printf("  17 = /dev/ttyUSB0    25 = /dev/ttyACM1\n");
    printf("\n");
}
