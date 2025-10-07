#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

enum display_mode { DEFAULT_VERTICAL, LONG_LISTING, HORIZONTAL };

// ------------------- ANSI Color Codes -------------------
#define COLOR_RESET     "\033[0m"
#define COLOR_BLUE      "\033[0;34m"
#define COLOR_GREEN     "\033[0;32m"
#define COLOR_RED       "\033[0;31m"
#define COLOR_PINK      "\033[0;35m"
#define COLOR_REVERSE   "\033[7m"

// ------------------- Function prototypes -------------------
void print_vertical(char **files, int count);
void print_long_listing(char **files, int count);
void print_horizontal(char **files, int count);
void do_ls(int n_files, char **files, enum display_mode mode);
void print_colored(const char *filename, mode_t st_mode);

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

    while ((opt = getopt(argc, argv, "lx")) != -1) {
        switch(opt) {
            case 'l': mode = LONG_LISTING; break;
            case 'x': mode = HORIZONTAL; break;
            default:
                fprintf(stderr, "Usage: %s [-l] [-x] [file...]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    int n_files = argc - optind;
    char **files;
    int dynamic = 0;

    if (n_files == 0) {
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

// ------------------- Colored Print -------------------
void print_colored(const char *filename, mode_t st_mode) {
    struct stat st;
    if (lstat(filename, &st) == -1) { perror("lstat"); return; }

    const char *color = COLOR_RESET;

    if (S_ISDIR(st.st_mode)) color = COLOR_BLUE;
    else if (st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) color = COLOR_GREEN;
    else if (strstr(filename, ".tar") || strstr(filename, ".gz") || strstr(filename, ".zip")) color = COLOR_RED;
    else if (S_ISLNK(st.st_mode)) color = COLOR_PINK;
    else if (S_ISCHR(st.st_mode) || S_ISBLK(st.st_mode) || S_ISSOCK(st.st_mode) || S_ISFIFO(st.st_mode))
        color = COLOR_REVERSE;

    printf("%s%s%s", color, filename, COLOR_RESET);
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
            if (idx < count) {
                printf("%-*s", max_len, files[idx]);
            }
        }
        printf("\n");
    }
}

void print_long_listing(char **files, int count) {
    for (int i = 0; i < count; i++) {
        struct stat st;
        if (stat(files[i], &st) == -1) { perror("stat"); continue; }
        print_colored(files[i], st.st_mode);
        printf("\n");
    }
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
        print_colored(files[i], 0); // st_mode 0 just for non-long listing
        printf("%-*s", max_len, ""); // padding
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
