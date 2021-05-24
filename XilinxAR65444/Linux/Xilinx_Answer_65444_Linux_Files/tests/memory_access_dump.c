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

#define DEVICE_NAME_DEFAULT "/dev/xdma0_h2c_0"

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

static int test_dma(char *devicename, uint32_t addr, uint32_t offset, uint32_t count, char **filename_arr, int filename_arr_length);

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
    uint32_t offset = 0;
    uint32_t count = 1;
    char *filename = NULL;
    char **filename_arr = NULL;
    int cur_filename_idx = 0;

    // SANGJIN
    // initialize filename array with MAX_FILE_SUPPORTED
    filename_arr = malloc(sizeof(char *) * MAX_FILE_SUPPORTED);

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
            case 'o':
                offset = getopt_integer(optarg) & 4095;
                break;
            case 'c':
                count = getopt_integer(optarg);
                break;
            case 'f': // SANGJIN
                if (cur_filename_idx == 100)
                    perror("reached maximum file number");
                filename = strdup(optarg);
                filename_arr[cur_filename_idx] = filename;
                cur_filename_idx += 1;
                break;
            /* print usage help and exit */
            case 'h':
            default:
                usage(argv[0]);
                exit(0);
                break;
        }
    }

    printf("device = %s, address = 0x%08x, total_file_cnt = %d\n", device, address, cur_filename_idx);
    test_dma(device, address, offset, count, filename_arr, cur_filename_idx);
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

static int test_dma(char *devicename, uint32_t addr, uint32_t offset, uint32_t count, char **filename_arr, int filename_arr_length)
{
    int rc, file_fd, i;
    int fpga_fd = open(devicename, O_RDWR);
    char *buffer, *filename;
    uint32_t size, total_size = 0;
    struct timespec ts_start, ts_end;
    struct xdma_multiple_sgdma_ioctl args;
    struct stat stat_buf;

    assert(fpga_fd >= 0);
    args.cnt = filename_arr_length; 

    for (i = 0; i < filename_arr_length; i++) {
        filename = filename_arr[i];
        if (filename) {
            file_fd = open(filename, O_RDONLY);
            assert(file_fd >= 0);
        }

        if (file_fd >= 0) {
            fstat(file_fd, &stat_buf);
            size = stat_buf.st_size;
            posix_memalign((void **)&buffer, 4096, size);
            rc = read(file_fd, buffer, size);
            
            if (rc != size)
                perror("read(file_fd)");
            
            assert(rc == size);
            close(file_fd);
            
            args.va_arr[i] = buffer;
            args.size_arr[i] = size;
            total_size += size;
        }
    }
    args.pos = addr;
    
    rc = clock_gettime(CLOCK_MONOTONIC, &ts_start);
    rc = ioctl(fpga_fd, IOCTL_XDMA_MEMORY_ACCESS_DUMP, &args);
    rc = clock_gettime(CLOCK_MONOTONIC, &ts_end);
    timespec_sub(&ts_end, &ts_start);
  
    printf("CLOCK_MONOTONIC reports %ld.%09ld seconds (total) for last transfer of %d bytes\n", ts_end.tv_sec, ts_end.tv_nsec, total_size);

    if (fpga_fd >= 0)
        close(fpga_fd);

    if (file_fd >= 0)
        close(file_fd);

    for (i = 0; i < filename_arr_length; i++)
        free(args.va_arr[i]);

    free(filename_arr);
}

