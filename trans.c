#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#define DEVICE_NAME "/dev/mem"
#define PHY_REG_BASE 0x0

int mem_upload_func(const char *dev_name, const char *fname, uint64_t size)
{
    int device_fd = open(dev_name, O_RDWR | O_SYMC);
    if (device_fd == -1)
    {
        perror("Error opening %s", dev_name);
        return 1;
    }

    void *map_base = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, device_fd, PHY_REG_BASE);
    if (map_base == MAP_FAILED)
    {
        perror("Error mapping register to user space!\n");
        close(device_fd);
        return 1;
    }

    FILE *file = fopen(fname, "wb");
    if (file == NULL)
    {
        perror("Error opening output file!\n");
        munmap(map_base, size);
        close(device_fd);
        printf("Can't open file: %s\n", fname);
        return 1;
    }
    else
    {
        fwrite(map_base, 1, size, file);
        fclose(file);
        munmap(map_base, size);
        close(device_fd);
        printf("Write success: %s\n", fname);
    }
    return 0;
}

int mem_download_func(const char *dev_name, const char *fname, uint64_t size)
{
    int device_fd = open(dev_name, O_RDWR | O_SYMC);
    if (device_fd == -1)
    {
        perror("Error opening %s", dev_name);
        return 1;
    }

    void *map_base = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, device_fd, PHY_REG_BASE);
    if (map_base == MAP_FAILED)
    {
        perror("Error mapping register to user space!\n");
        close(device_fd);
        return 1;
    }

    FILE *file = fopen(fname, "wb");
    if (file == NULL)
    {
        perror("Error opening output file!\n");
        munmap(map_base, size);
        close(device_fd);
        printf("Can't open file: %s\n", fname);
        return 1;
    }
    else
    {
        fread(map_base, 1, size, file);
        fclose(file);
        munmap(map_base, size);
        close(device_fd);
    }
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        printf("Usage: %s <operation> <filename> <size>\n", argv[0]);
        printf("Operation: d u\n");
        return 0;
    }

    const char op = argv[1][0];
    const char *filename = argv[2];
    uint64_t size = std::atoi(argv[3]);
    if (op == 'u')
    {
        mem_upload_func(DEVICE_NAME, filename, size);
    }
    els if (op == 'd')
    {
        mem_download_func(DEVICE_NAME, filename, size);
    }
    else
    {
        printf("Not supported operation.\n");
        return 0;
    }
    return 0;
}