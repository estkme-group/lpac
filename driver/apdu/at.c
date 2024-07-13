#include "at.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <euicc/interface.h>
#include <euicc/hexutil.h>

// Include necessary headers for serial communication
#include <fcntl.h>
#include <stdint.h>
#include <termios.h>
#include <sys/file.h>

// Declare static variables
static int fd = -1;
static int logic_channel = 0;

// Function to open AT device
static int open_at_device(const char *device)
{
    fd = open(device, O_RDWR | O_NOCTTY);
    if (fd == -1)
    {
        fprintf(stderr, "Failed to open device: %s\n", device);
        return -1;
    }

    int result = tcflush(fd, TCIOFLUSH);
    if (result == -1)
    {
        fprintf(stderr, "Failed to flush device: %s\n", device);
        return -1;
    }

    struct termios tty;
    result = tcgetattr(fd, &tty);
    if (result == -1)
    {
        fprintf(stderr, "Failed to get device attributes: %s\n", device);
        return -1;
    }

    tty.c_cflag &= ~(PARENB | CSTOPB | CSIZE);                                                          // Clear parity, stop bits, and data size
    tty.c_cflag |= CS8;                                                                                 // Set 8 bits per byte
    tty.c_cflag |= CREAD | CLOCAL;                                                                      // Enable READ & ignore ctrl lines
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHONL | ISIG);                                            // Disable canonical mode, echo, erasure, new-line echo, and interpretation of special characters
    tty.c_iflag &= ~(IXON | IXOFF | IXANY | IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL); // Turn off software flow control and various input processing options
    tty.c_cc[VTIME] = 0;
    tty.c_cc[VMIN] = 0;

    cfsetispeed(&tty, B19200);
    cfsetospeed(&tty, B19200);

    result = tcsetattr(fd, TCSANOW, &tty);
    if (result == -1)
    {
        fprintf(stderr, "Failed to set device attributes: %s\n", device);
        close(fd);
        return -1;
    }

    if (flock(fd, LOCK_EX | LOCK_NB) == -1)
    {
        fprintf(stderr, "Failed to lock device: %s\n", device);
        close(fd);
        return -1;
    }
    return fd;
}

static int execute_command(uint8_t *buffer, size_t size)
{
    if (getenv("AT_DEBUG"))
        printf("AT_DEBUG[TX]: %s\r\n", buffer);
    ssize_t result = write(fd, buffer, size);
    if (result != (ssize_t)size)
    {
        perror("failed to write to port");
        return -1;
    }

    return 0;
}

static ssize_t read_command_response(uint8_t **buffer, size_t *size)
{
    size_t received = 0;
    size_t buffer_size = 1024;
    *buffer = malloc(buffer_size);
    if (*buffer == NULL)
    {
        perror("failed to allocate memory");
        return -1;
    }

    while (1)
    {
        ssize_t r = read(fd, *buffer + received, buffer_size - received - 1);
        if (r < 0)
        {
            perror("failed to read from port");
            free(*buffer);
            *buffer = NULL;
            return -1;
        }
        if (r == 0)
        {
            break;
        }
        received += r;
        if (received >= buffer_size - 1)
        {
            buffer_size *= 2;
            uint8_t *new_buffer = realloc(*buffer, buffer_size);
            if (new_buffer == NULL)
            {
                perror("failed to reallocate memory");
                free(*buffer);
                *buffer = NULL;
                return -1;
            }
            *buffer = new_buffer;
        }
    }
    (*buffer)[received] = '\0';
    *size = received;

    return received;
}

static int at_expect(char **response, const char *expected)
{
    uint8_t *buffer = NULL;
    size_t buffer_len = 0;
    size_t total_len = 0;

    if (response)
        *response = NULL;

    while (1)
    {
        ssize_t r = read_command_response(&buffer, &buffer_len);
        if (r < 0)
        {
            free(buffer);
            return -1;
        }
        total_len += buffer_len;

        char *line = strtok((char *)buffer, "\r\n");
        while (line != NULL)
        {
            if (getenv("AT_DEBUG"))
                printf("AT_DEBUG[RX]: %s\r\n", line);
            if (strcmp(line, "ERROR") == 0)
            {
                free(buffer);
                return -1;
            }
            else if (strcmp(line, "OK") == 0)
            {
                free(buffer);
                return 0;
            }
            else if (expected && strncmp(line, expected, strlen(expected)) == 0)
            {
                if (response)
                    *response = strdup(line + strlen(expected));
            }

            line = strtok(NULL, "\r\n");
        }
        free(buffer);
    }
    return 0;
}

static int apdu_interface_connect(struct euicc_ctx *ctx)
{
    const char *device;

    logic_channel = 0;

    if (!(device = getenv("AT_DEVICE")))
    {
        device = "/dev/ttyUSB2";
    }

    fd = open_at_device(device);
    if (fd == -1)
    {
        return -1;
    }

    uint8_t command[] = "AT+CCHO=?\r\n";
    if (execute_command(command, sizeof(command) - 1) == -1)
    {
        fprintf(stderr, "Failed to execute command: AT+CCHO=?\n");
        return -1;
    }
    if (at_expect(NULL, NULL))
    {
        fprintf(stderr, "Device missing AT+CCHO/AT+CGLA/AT+CCHC support\n");
        return -1;
    }

    return 0;
}

static void apdu_interface_disconnect(struct euicc_ctx *ctx)
{
    close(fd);
    fd = -1;
    logic_channel = 0;
}

static int apdu_interface_transmit(struct euicc_ctx *ctx, uint8_t **rx, uint32_t *rx_len, const uint8_t *tx, uint32_t tx_len)
{
    int fret = 0;
    int ret;
    char *response = NULL;
    char *hexstr = NULL;

    *rx = NULL;
    *rx_len = 0;

    if (!logic_channel)
    {
        return -1;
    }

    uint8_t buffer[tx_len * 2 + 32];
    int len = snprintf((char *)buffer, sizeof(buffer), "AT+CGLA=%d,%u,\"", logic_channel, tx_len * 2);
    for (uint32_t i = 0; i < tx_len; i++)
    {
        len += snprintf((char *)buffer + len, sizeof(buffer) - len, "%02X", (uint8_t)(tx[i] & 0xFF));
    }
    len += snprintf((char *)buffer + len, sizeof(buffer) - len, "\"\r\n");

    if (execute_command(buffer, len) == -1)
    {
        goto err;
    }

    if (at_expect(&response, "+CGLA: "))
    {
        goto err;
    }

    if (response == NULL)
    {
        goto err;
    }

    strtok(response, ",");
    hexstr = strtok(NULL, ",");
    if (!hexstr)
    {
        goto err;
    }
    if (hexstr[0] == '"')
    {
        hexstr++;
    }
    hexstr[strcspn(hexstr, "\"")] = '\0';

    *rx_len = strlen(hexstr) / 2;
    *rx = malloc(*rx_len);
    if (!*rx)
    {
        goto err;
    }

    ret = euicc_hexutil_hex2bin_r(*rx, *rx_len, hexstr, strlen(hexstr));
    if (ret < 0)
    {
        goto err;
    }
    *rx_len = ret;

    goto exit;

err:
    fret = -1;
    free(*rx);
    *rx = NULL;
    *rx_len = 0;
exit:
    free(response);
    return fret;
}

static int apdu_interface_logic_channel_open(struct euicc_ctx *ctx, const uint8_t *aid, uint8_t aid_len)
{
    char *response;

    if (logic_channel)
    {
        return logic_channel;
    }

    for (int i = 1; i <= 4; i++)
    {
        uint8_t command[16];
        int len = snprintf((char *)command, sizeof(command), "AT+CCHC=%d\r\n", i);
        execute_command(command, len);
        at_expect(NULL, NULL);
    }

    uint8_t command[aid_len * 2 + 24];
    int len = snprintf((char *)command, sizeof(command), "AT+CCHO=\"");
    for (int i = 0; i < aid_len; i++)
    {
        len += snprintf((char *)command + len, sizeof(command) - len, "%02X", (uint8_t)(aid[i] & 0xFF));
    }
    len += snprintf((char *)command + len, sizeof(command) - len, "\"\r\n");

    if (execute_command(command, len) == -1)
    {
        return -1;
    }
    if (at_expect(&response, "+CCHO: "))
    {
        return -1;
    }
    if (response == NULL)
    {
        return -1;
    }
    logic_channel = atoi(response);

    return logic_channel;
}

static void apdu_interface_logic_channel_close(struct euicc_ctx *ctx, uint8_t channel)
{
    if (!logic_channel)
    {
        return;
    }

    uint8_t command[16];
    int len = snprintf((char *)command, sizeof(command), "AT+CCHC=%d\r\n", logic_channel);
    execute_command(command, len);
    at_expect(NULL, NULL);
}

static int libapduinterface_init(struct euicc_apdu_interface *ifstruct)
{
    memset(ifstruct, 0, sizeof(struct euicc_apdu_interface));

    ifstruct->connect = apdu_interface_connect;
    ifstruct->disconnect = apdu_interface_disconnect;
    ifstruct->logic_channel_open = apdu_interface_logic_channel_open;
    ifstruct->logic_channel_close = apdu_interface_logic_channel_close;
    ifstruct->transmit = apdu_interface_transmit;

    return 0;
}

static int libapduinterface_main(int argc, char **argv)
{
    return 0;
}

static void libapduinterface_fini(void)
{
}

const struct euicc_driver driver_apdu_at = {
    .type = DRIVER_APDU,
    .name = "at",
    .init = (int (*)(void *))libapduinterface_init,
    .main = libapduinterface_main,
    .fini = libapduinterface_fini,
};
