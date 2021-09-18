#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <inttypes.h>
#include <linux/i2c-dev.h>
#include <linux/limits.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <i2c/smbus.h>

#define ARRAY_SZ(x)         (size_t)(sizeof(x) / sizeof(x[0]))

#pragma pack(push, 1)
typedef union {
    struct _u {
        uint8_t rs : 1;
        uint8_t rw : 1;
        uint8_t en : 1;
        uint8_t bl : 1;
        uint8_t data : 4;
    } u;
    uint8_t value;
} i2c_data;
#pragma pack(pop)

static unsigned char device_path[NAME_MAX];
static uint8_t device_addr = 0x00;

void toggle_backlight(int fd, uint8_t state)
{
    i2c_data data;
    int ret = 0;
    memset(&data, 0, sizeof(i2c_data));

    data.u.rs = 0;
    data.u.rw = 0;

    // Set BL pin
    data.u.bl = (state) ? 1 : 0;
    ret = i2c_smbus_write_byte(fd, data.value);
    if (ret < 0) {
        fprintf(stderr, "Unable to write to device: %s.\n", strerror(errno));
        close(fd);
        exit(ret);
    }

    // Toggle EN high
    data.u.en = 1;
    ret = i2c_smbus_write_byte(fd, data.value);
    if (ret < 0) {
        fprintf(stderr, "Unable to write to device: %s.\n", strerror(errno));
        close(fd);
        exit(ret);
    }
    usleep(10);
    // Toggle EN low
    data.u.en = 0;
    ret = i2c_smbus_write_byte(fd, data.value);
    if (ret < 0) {
        fprintf(stderr, "Unable to write to device: %s.\n", strerror(errno));
        close(fd);
        exit(ret);
    }
    usleep(10);
}

static void print_help(char **argv)
{
    fprintf(stdout, "Usage:\n"
                    "\t%s -d [device path] -a [address] -s [0|1]\n",
                    argv[0]);
    exit(0);
}

int main(int argc, char **argv)
{
    int fd = -1;
    int opt = 0;
    uint8_t state = 0;
    uint8_t opt_set = 0;
    errno = 0;

    memset(device_path, 0, ARRAY_SZ(device_path));

    while ((opt = getopt(argc, argv, "d:a:s:")) != -1) {
        switch (opt) {
            case 'd':
                snprintf((char *)device_path, ARRAY_SZ(device_path), "%s", optarg);
                opt_set++;
                break;
            case 'a':
                device_addr = (uint8_t)strtoul(optarg, NULL, 0);
                opt_set++;
                break;
            case 's':
                state = (uint8_t)strtoul(optarg, NULL, 0);
                if (state > 1)
                    state = 1;
                opt_set++;
                break;
            case '?':
                if (optopt == 'd')
                    fprintf(stderr, "Device path must be given, e.g., /dev/i2c-0\n.");
                else if (optopt == 'a')
                    fprintf(stderr, "Device address must be given, e.g., 0x01.\n");
                else if (optopt == 's')
                    fprintf(stderr, "State must be given, either 0 or 1.\n");
                else if (isprint(optopt))
                    fprintf(stderr, "Unknown option `-%c.\n", optopt);
                else
                    fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
                exit(1);
                break;
            default:
                break;
        }
    }

    if (opt_set < 3) {
        print_help(argv);
        exit(0);
    }

    if ((fd = open((const char *)device_path, O_RDWR)) < 0) {
        fprintf(stderr, "Failed to open SMBUS for device %s: %s.\n",
                device_path, strerror(errno));
        exit(fd);
    }

    if (ioctl(fd, I2C_SLAVE, device_addr) < 0) {
        fprintf(stderr, "Bus access error: %s.\n", strerror(errno));
        close(fd);
        exit(1);
    }

    fprintf(stdout, "Turning backlight %s for device %s @0x%02x.\n",
            (state) ? "on" : "off", device_path, device_addr);
    toggle_backlight(fd, state);

    close(fd);
    return 0;
}
