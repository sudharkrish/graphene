#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/eventfd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "ioctl-dummy-driver/dummy.h"

int device_fd = 0;

int efd_test() {
    int efd;  // Eventfd file descriptor
    uint64_t efd_ctr;
    struct pollfd poll_fd;
    int poll_ret = 0;
    int retval;
    struct dummy_efd_info efd_info;

    // Create eventfd
    efd = eventfd(100, 0);
    if (efd < 0) {
        printf("\nUnable to create eventfd! Exiting...\n");
        return efd;
    }

    efd_info.pid = getpid();
    efd_info.efd = efd;
    printf("Sending pid=%d, efd=%d to driver\n", efd_info.pid, efd_info.efd);

    if ((retval = ioctl(device_fd, DUMMY_IOCTL_EFD, &efd_info))) {
        perror("ioctl");
        printf("%s, %d: Error ret from ioctl=%d\n", __func__, __LINE__, retval);
        return retval;
    } else {
        printf("Received pid=%d, efd=%d to driver\n", efd_info.pid, efd_info.efd);
    }

    poll_fd.fd     = efd;
    poll_fd.events = POLLIN;

    while (1) {
        poll_ret = poll(&poll_fd, 1, 5000);

        if (poll_ret == 0) {
            printf("Poll timed out. Exiting.\n");
            break;
        }

        if (poll_ret < 0) {
            perror("error from poll");
            retval = 1;
            break;
        }

        if (poll_fd.revents & POLLIN) {
            poll_fd.revents = 0;
            errno           = 0;
            read(poll_fd.fd, &efd_ctr, sizeof(efd_ctr));
            printf("efd = %d, count: %lu, errno=%d\n", poll_fd.fd, efd_ctr, errno);
            break;
        }
    }

    printf("\nClosing eventfd. Exiting...");
    close(efd);

    return retval;
}

int dummy_print_test(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        struct dummy_print arg;
        arg.str  = argv[i];
        arg.size = strlen(argv[i]);

        if (ioctl(device_fd, DUMMY_IOCTL_PRINT, &arg)) {
            perror("ioctl");
            printf("%s: Error ret from ioctl\n", __func__);
            return 1;
        } else
            fprintf(stderr, "wrote %s to kernel\n", argv[i]);
    }

    return 0;
}

int main(int argc, char** argv) {
    device_fd = open("/dev/dummy", O_RDWR);
    if (device_fd < 0) {
        perror("open");
        return 1;
    }

    if (argc < 2) {
        printf("usage is ./test-ioctl test string\n");
        return -1;
    } else
        dummy_print_test(argc, argv);

    efd_test();

    close(device_fd);

    return 0;
}
