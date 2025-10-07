#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

enum display_mode { DEFAULT_VERTICAL, LONG_LISTING, HORIZONTAL };

// ------------------- Function prototypes -------------------
void print_vertical(char **files, int count);
void print_long_listing(char **files, int count);
void print_horizontal(char **files, int count);
void do_ls(int n_files, char **files, enum display_mode mode);

// Comparison function for qsort
int cmp_str(const void *a, const void *b) {
    char *str1 = *(char **)a;
    char *str2 = *(char **)b;
    return strcmp(str1, str2);
}

// Custom strdup to avoid implicit declaration
char *my_strdup(const char *s) {
    size_t len = strlen(s);
    char *p = malloc(len + 1);
    if (!p) { perror("malloc"); exit(EXIT_FAILURE); }
    strcpy(p, s);
    return p;
}

// ------------------- Main -------------------
int main(int argc, char *argv[]) {
    enum display_mode mode = DEFAULT_VERTICAL;
    int opt;

    // Parse command-line options
    while ((opt = getopt(argc, argv, "lx")) != -1) {
        switch (opt) {
            case 'l':
                mode = LONG_LISTING;
                break;
            case 'x':
                mode = HORIZONTAL;
                break;
            default:
                fprintf(stderr, "Usage: %s [-l] [-x] [file...]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    int n_files = argc - optind;
    char **files;
    int dynamic = 0;

    if (n_files == 0) {
        // Read current directory
        DIR *dirp = opendir(".");
        if (!dirp) { perror("opendir"); exit(EXIT_FAILURE); }

        struct dirent *dp;
        int capacity = 64;
        files = malloc(capacity * sizeof(char *));
        if (!files) { perror("malloc"); exit(EXIT_FAILURE); }

        n_files = 0;
        dynamic = 1;

        while ((dp = readdir(dirp)) != NULL) {
            if (dp->d_name[0] == '.') continue; // skip hidden files
            if (n_files >= capacity) {
                capacity *= 2;
                files = realloc(files, capacity * sizeof(char *));
                if (!files) { perror("realloc"); exit(EXIT_FAILURE); }
            }
            files[n_files++] = my_strdup(dp->d_name);
        }
        closedir(dirp);
    } else {
        files = &argv[optind];
    }

    // Alphabetical sort
    qsort(files, n_files, sizeof(char *), cmp_str);

    // Display
    do_ls(n_files, files, mode);

    // Free memory if dynamically allocated
    if (dynamic) {
        for (int i = 0; i < n_files; i++) free(files[i]);
        free(files);
    }

    return 0;
}

// ------------------- Printing Functions -------------------
void print_vertical(char **files, int count) {
    int max_len = 0;
    for (int i = 0; i < count; i++) {
        int len = strlen(files[i]);
        if (len > max_len) max_len = len;
    }
    max_len += 2;

    int cols = 80 / max_len;
    if (cols == 0) cols = 1;
    int rows = (count + cols - 1) / cols;

    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            int idx = c * rows + r;
            if (idx < count)
                printf("%-*s", max_len, files[idx]);
        }
        printf("\n");
    }
}

void print_long_listing(char **files, int count) {
    for (int i = 0; i < count; i++)
        printf("-rw-r--r-- 1 user group 1234 Oct 7 00:00 %s\n", files[i]);
}

void print_horizontal(char **files, int count) {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int term_width = w.ws_col;

    int max_len = 0;
    for (int i = 0; i < count; i++) {
        int len = strlen(files[i]);
        if (len > max_len) max_len = len;
    }
    max_len += 2;

    int pos = 0;
    for (int i = 0; i < count; i++) {
        printf("%-*s", max_len, files[i]);
        pos += max_len;
        if (pos + max_len > term_width) {
            printf("\n");
            pos = 0;
        }
    }
    if (pos != 0) printf("\n");
}

// ------------------- do_ls -------------------
void do_ls(int n_files, char **files, enum display_mode mode) {
    switch(mode) {
        case LONG_LISTING:
            print_long_listing(files, n_files);
            break;
        case HORIZONTAL:
            print_horizontal(files, n_files);
            break;
        case DEFAULT_VERTICAL:
        default:
            print_vertical(files, n_files);
            break;
    }
}
