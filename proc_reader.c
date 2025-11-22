#include "proc_reader.h"

/* We already have stdio, stdlib, string, unistd, dirent, fcntl, ctype
 * included from proc_reader.h
 */

/* ---------- Helper: check if string is all digits ---------- */
int is_number(const char* str) {
    if (str == NULL || *str == '\0') return 0;

    const unsigned char* p = (const unsigned char*)str;
    while (*p) {
        if (!isdigit(*p)) return 0;
        p++;
    }
    return 1;
}

/* ---------- Helper: print first n lines of a file ---------- */
static void print_first_n_lines(const char *path, int n) {
    FILE *f = fopen(path, "r");
    if (!f) {
        perror("fopen");
        return;
    }

    char line[512];
    int count = 0;
    while (count < n && fgets(line, sizeof(line), f) != NULL) {
        printf("%s", line);
        count++;
    }

    fclose(f);
}

/* ---------- Option 1: List process directories ---------- */

int list_process_directories(void) {
    DIR *dir = opendir("/proc");
    if (!dir) {
        perror("opendir");
        return -1;
    }

    printf("Listing all process directories in /proc...\n");
    printf("PID      Type\n");
    printf("---      ----\n");

    struct dirent *entry;
    int count = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (is_number(entry->d_name)) {
            printf("%-8s process\n", entry->d_name);
            count++;
        }
    }

    closedir(dir);

    printf("Found %d process directories\n", count);
    printf("SUCCESS: Process directories listed!\n");
    return 0;
}

/* ---------- Option 2: Read process information ---------- */

int read_process_info(const char* pid_str) {
    int pid = atoi(pid_str);

    char status_path[256];
    char cmdline_path[256];

    snprintf(status_path, sizeof(status_path), "/proc/%d/status", pid);
    snprintf(cmdline_path, sizeof(cmdline_path), "/proc/%d/cmdline", pid);

    printf("Reading information for PID %d...\n\n", pid);

    FILE *status = fopen(status_path, "r");
    if (!status) {
        perror("fopen status");
        return -1;
    }

    printf("--- Process Information for PID %d ---\n", pid);

    char line[512];
    const char *keys[] = {
        "Name:", "Umask:", "State:", "Tgid:", "Ngid:",
        "Pid:", "PPid:", "TracerPid:", "Uid:", "Gid:"
    };
    size_t num_keys = sizeof(keys)/sizeof(keys[0]);

    while (fgets(line, sizeof(line), status) != NULL) {
        for (size_t i = 0; i < num_keys; i++) {
            if (strncmp(line, keys[i], strlen(keys[i])) == 0) {
                printf("%s", line);
                break;
            }
        }
    }

    fclose(status);

    printf("\n--- Command Line ---\n");

    int fd = open(cmdline_path, O_RDONLY);
    if (fd >= 0) {
        char buf[1024];
        ssize_t bytes;

        while ((bytes = read(fd, buf, sizeof(buf))) > 0) {
            for (ssize_t i = 0; i < bytes; i++) {
                if (buf[i] == '\0') buf[i] = ' ';
            }
            buf[bytes] = '\0';
            printf("%s", buf);
        }

        printf("\n");
        close(fd);
    } else {
        perror("open cmdline");
    }

    printf("SUCCESS: Process information read!\n");
    return 0;
}

/* ---------- Option 3: Show system information ---------- */

int show_system_info(void) {
    printf("Reading system information...\n\n");

    printf("--- CPU Information (first 10 lines) ---\n");
    print_first_n_lines("/proc/cpuinfo", 10);
    printf("\n");

    printf("--- Memory Information (first 10 lines) ---\n");
    print_first_n_lines("/proc/meminfo", 10);

    printf("SUCCESS: System information displayed!\n");
    return 0;
}

/* ---------- Helper: read file using raw system calls ---------- */

int read_file_with_syscalls(const char* filename) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return -1;
    }

    char buf[1024];
    ssize_t bytes;

    while ((bytes = read(fd, buf, sizeof(buf))) > 0) {
        if (write(STDOUT_FILENO, buf, bytes) < 0) {
            perror("write");
            close(fd);
            return -1;
        }
    }

    if (bytes < 0) {
        perror("read");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

/* ---------- Helper: read file using stdio library ---------- */

int read_file_with_library(const char* filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        perror("fopen");
        return -1;
    }

    char line[512];
    while (fgets(line, sizeof(line), f) != NULL) {
        printf("%s", line);
    }

    fclose(f);
    return 0;
}

/* ---------- Option 4: Compare file methods ---------- */

void compare_file_methods(void) {
    const char *filename = "/proc/version";

    printf("Comparing file operation methods...\n");
    printf("Comparing file reading methods for: %s\n\n", filename);

    printf("=== Method 1: Using System Calls ===\n");
    read_file_with_syscalls(filename);
    printf("\n\n");

    printf("=== Method 2: Using Library Functions ===\n");
    read_file_with_library(filename);
    printf("\n");

    printf("\nNOTE: Run with strace to compare syscalls.\n");
}
