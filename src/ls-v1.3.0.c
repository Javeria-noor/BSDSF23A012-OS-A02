#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

// Display mode flags
enum { DEFAULT, LONG_LIST, HORIZONTAL };
int display_mode = DEFAULT;
int recursive_flag = 0; // new for -R

// Color codes from v1.5.0
#define COLOR_RESET "\033[0m"
#define COLOR_BLUE  "\033[0;34m"
#define COLOR_GREEN "\033[0;32m"
#define COLOR_RED   "\033[0;31m"
#define COLOR_PINK  "\033[0;35m"
#define COLOR_REV   "\033[7m"

// Function to print a file with color based on type
void print_file_colored(const char *path, const char *name) {
    struct stat st;
    if (lstat(path, &st) == -1) {
        perror("lstat");
        return;
    }

    if (S_ISDIR(st.st_mode)) printf(COLOR_BLUE "%s" COLOR_RESET "\n", name);
    else if (st.st_mode & S_IXUSR) printf(COLOR_GREEN "%s" COLOR_RESET "\n", name);
    else if (strstr(name, ".tar") || strstr(name, ".gz") || strstr(name, ".zip"))
        printf(COLOR_RED "%s" COLOR_RESET "\n", name);
    else if (S_ISLNK(st.st_mode)) printf(COLOR_PINK "%s" COLOR_RESET "\n", name);
    else if (S_ISCHR(st.st_mode) || S_ISBLK(st.st_mode) || S_ISSOCK(st.st_mode) || S_ISFIFO(st.st_mode))
        printf(COLOR_REV "%s" COLOR_RESET "\n", name);
    else printf("%s\n", name);
}

// Alphabetical comparison for qsort
int cmpstring(const void *a, const void *b) {
    const char *pa = *(const char **)a;
    const char *pb = *(const char **)b;
    return strcmp(pa, pb);
}

// Horizontal display (simplified)
void horizontal_display(char **files, int n) {
    int i;
    for (i = 0; i < n; i++) printf("%-20s", files[i]);
    printf("\n");
}

// Recursive ls function
void do_ls(const char *dirname) {
    DIR *dir = opendir(dirname);
    if (!dir) {
        fprintf(stderr, "Cannot open directory '%s': %s\n", dirname, strerror(errno));
        return;
    }

    printf("\n%s:\n", dirname);

    struct dirent *dp;
    char **files = NULL;
    int n_files = 0, capacity = 16;
    files = malloc(capacity * sizeof(char *));
    while ((dp = readdir(dir)) != NULL) {
        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) continue;
        if (n_files >= capacity) files = realloc(files, capacity *= 2 * sizeof(char *));
        files[n_files++] = strdup(dp->d_name);
    }
    closedir(dir);

    qsort(files, n_files, sizeof(char *), cmpstring);

    // Display
    for (int i = 0; i < n_files; i++) {
        char fullpath[1024];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", dirname, files[i]);

        if (display_mode == LONG_LIST) print_file_colored(fullpath, files[i]); // long-list
        else if (display_mode == HORIZONTAL) horizontal_display(&files[i], 1); // horizontal
        else print_file_colored(fullpath, files[i]); // default
    }

    // Recursive call for directories
    if (recursive_flag) {
        for (int i = 0; i < n_files; i++) {
            char fullpath[1024];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", dirname, files[i]);
            struct stat st;
            if (lstat(fullpath, &st) == 0 && S_ISDIR(st.st_mode))
                do_ls(fullpath);
        }
    }

    for (int i = 0; i < n_files; i++) free(files[i]);
    free(files);
}

int main(int argc, char *argv[]) {
    int opt;
    while ((opt = getopt(argc, argv, "lRx")) != -1) {
        switch (opt) {
            case 'l': display_mode = LONG_LIST; break;
            case 'x': display_mode = HORIZONTAL; break;
            case 'R': recursive_flag = 1; break;
            default: fprintf(stderr, "Usage: %s [-lRx] [dir]\n", argv[0]); exit(EXIT_FAILURE);
        }
    }

    const char *dir = (optind < argc) ? argv[optind] : ".";
    do_ls(dir);
    return 0;
}
