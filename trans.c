#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#define DEVICE_NAME "/dev/mem"
#define PHY_REG_BASE 0x0

int mem_upload_func(const char *dev_name, const char *fname, uint64_t size) {
    int device_fd = open(dev_name, O_RDWR | O_SYMC);
    if (device_fd == -1) {
        perror("Error opening %s", dev_name);
        return 1;
    }

    void 
}