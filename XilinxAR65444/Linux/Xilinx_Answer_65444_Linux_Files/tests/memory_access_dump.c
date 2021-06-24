#include <assert.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "../include/xdma-ioctl.h"

#define DEVICE_NAME_DEFAULT "/dev/xdma0_c2h_0"

static struct option const long_opts[] =
{
    {"device", required_argument, NULL, 'd'},
    {"address", required_argument, NULL, 'a'},
    {"size", required_argument, NULL, 's'},
    {"offset", required_argument, NULL, 'o'},
    {"count", required_argument, NULL, 'c'},
    {"file", required_argument, NULL, 'f'},
    {"help", no_argument, NULL, 'h'},
    {0, 0, 0, 0}
};

static int test_dma(char *devicename, uint32_t addr, char *filename);

static void usage(const char* name)
{
    int i = 0;
    printf("%s\n\n", name);
    printf("usage: %s [OPTIONS]\n\n", name);
    printf("Write using Multi Scatter Gather DMA, optionally read input from a binary input file.\n\n");

    printf("  -%c (--%s) device (defaults to %s)\n", long_opts[i].val, long_opts[i].name, DEVICE_NAME_DEFAULT); i++;
    printf("  -%c (--%s) address of the start address on the AXI bus\n", long_opts[i].val, long_opts[i].name); i++;
    printf("  -%c (--%s) size of a single transfer\n", long_opts[i].val, long_opts[i].name); i++;
    printf("  -%c (--%s) page offset of transfer\n", long_opts[i].val, long_opts[i].name); i++;
    printf("  -%c (--%s) number of transfers\n", long_opts[i].val, long_opts[i].name); i++;
    printf("  -%c (--%s) filename to read/write the data of the transfers\n", long_opts[i].val, long_opts[i].name); i++;
    printf("  -%c (--%s) be more verbose during test\n", long_opts[i].val, long_opts[i].name); i++;
    printf("  -%c (--%s) print usage help and exit\n", long_opts[i].val, long_opts[i].name); i++;
}

static uint32_t getopt_integer(char *optarg)
{
    int rc;
    uint32_t value;
    
    rc = sscanf(optarg, "0x%x", &value);
    if (rc <= 0)
        rc = sscanf(optarg, "%ul", &value);
    return value;
}

int main(int argc, char* argv[])
{
    int cmd_opt;
    char *device = DEVICE_NAME_DEFAULT;
    uint32_t address = 0;
    char *filename = NULL;


    while ((cmd_opt = getopt_long(argc, argv, "vhc:f:d:a:s:o:", long_opts, NULL)) != -1)
    {
        switch (cmd_opt)
        {
            /* device node name */
            case 'd':
                device = strdup(optarg);
                break;
            /* RAM address on the AXI bus in bytes */
            case 'a':
                address = getopt_integer(optarg);
                break;
            case 'f': // SANGJIN
                filename = strdup(optarg);
                break;
            /* print usage help and exit */
            case 'h':
            default:
                usage(argv[0]);
                exit(0);
                break;
        }
    }

    printf("device = %s, address = 0x%08x\n", device, address);
    test_dma(device, address, filename);
}

/* Subtract timespec t2 from t1
 *
 * Both t1 and t2 must already be normalized
 * i.e. 0 <= nsec < 1000000000 */
static void timespec_sub(struct timespec *t1, const struct timespec *t2)
{
    assert(t1->tv_nsec >= 0);
    assert(t1->tv_nsec < 1000000000);
    assert(t2->tv_nsec >= 0);
    assert(t2->tv_nsec < 1000000000);
    t1->tv_sec -= t2->tv_sec;
    t1->tv_nsec -= t2->tv_nsec;
    
    if (t1->tv_nsec >= 1000000000) {
        t1->tv_sec++;
        t1->tv_nsec -= 1000000000;
    } else if (t1->tv_nsec < 0) {
        t1->tv_sec--;
        t1->tv_nsec += 1000000000;
    }
}

static int test_dma(char *devicename, uint32_t addr, char *filename)
{
    int rc, file_fd, i;
    int fpga_fd = open(devicename, O_RDWR);
    char *buffer;
    uint32_t size = 4096;
    struct timespec ts_start, ts_end;
    struct xdma_memory_access_dump_ioctl args;
    struct stat stat_buf;

    assert(fpga_fd >= 0);

    posix_memalign((void **)&buffer, 4096 /*alignment*/, size);
    assert(buffer);
    printf("host memory buffer = %p\n", buffer);

    /* create file to write data to */
    if (filename) {
        file_fd = open(filename, O_RDWR | O_CREAT | O_TRUNC | O_SYNC, 0666);
        assert(file_fd >= 0);
    }

    args.pos = addr;
    args.buf = buffer; 
    args.size = size;
    
    //rc = clock_gettime(CLOCK_MONOTONIC, &ts_start);
    rc = ioctl(fpga_fd, IOCTL_XDMA_MEMORY_ACCESS_DUMP, &args);
    //rc = clock_gettime(CLOCK_MONOTONIC, &ts_end);
    //timespec_sub(&ts_end, &ts_start);

    if (file_fd >= 0)  {
        rc = write(file_fd, buffer, size);
        assert(rc == size);
    } 
  
    //printf("CLOCK_MONOTONIC reports %ld.%09ld seconds (total) for last transfer of %d bytes\n", ts_end.tv_sec, ts_end.tv_nsec, total_size);

    if (fpga_fd >= 0)
        close(fpga_fd);

    if (file_fd >= 0)
        close(file_fd);

    free(buffer);
}
