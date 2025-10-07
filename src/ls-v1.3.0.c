/* ls-v1.3.0.c - combined v1.6.0 ready file:
   features: alphabetical sort, -x horizontal, -l long, colorized, -R recursive
*/

#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>       // getopt, optind
#include <dirent.h>       // opendir, readdir
#include <sys/stat.h>     // stat, lstat, S_ISDIR, S_ISLNK, S_ISSOCK, S_ISCHR, S_ISBLK, S_ISFIFO
#include <sys/ioctl.h>    // ioctl, winsize
#include <errno.h>
#include <time.h>
#include <limits.h>       // PATH_MAX
#include <ctype.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

/* ANSI colors */
#define COLOR_RESET   "\033[0m"
#define COLOR_BLUE    "\033[0;34m"
#define COLOR_GREEN   "\033[0;32m"
#define COLOR_RED     "\033[0;31m"
#define COLOR_PINK    "\033[0;35m"
#define COLOR_REVERSE "\033[7m"

/* Safe strdup for portability */
static char *strdup_safe(const char *s) {
    if (!s) return NULL;
    size_t n = strlen(s) + 1;
    char *d = malloc(n);
    if (!d) { perror("malloc"); exit(EXIT_FAILURE); }
    memcpy(d, s, n);
    return d;
}

/* qsort compare for strings */
static int cmp_str(const void *a, const void *b) {
    const char *sa = *(const char **)a;
    const char *sb = *(const char **)b;
    return strcmp(sa, sb);
}

/* get terminal width (fallback 80) */
static int get_term_width(void) {
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0) return w.ws_col;
    return 80;
}

/* Print filename with color based on type (no newline) */
static void print_colored_name(const char *dirpath, const char *name) {
    char full[PATH_MAX];
    struct stat st;
    int ok;

    if (snprintf(full, sizeof(full), "%s/%s", dirpath, name) >= (int)sizeof(full)) {
        /* path too long, just print name plain */
        printf("%s", name);
        return;
    }

    ok = (lstat(full, &st) == 0);
    if (!ok) {
        printf("%s", name);
        return;
    }

    if (S_ISDIR(st.st_mode))      printf(COLOR_BLUE "%s" COLOR_RESET, name);
    else if (S_ISLNK(st.st_mode)) printf(COLOR_PINK "%s" COLOR_RESET, name);
    else if (S_ISCHR(st.st_mode) || S_ISBLK(st.st_mode) || S_ISSOCK(st.st_mode) || S_ISFIFO(st.st_mode))
                                  printf(COLOR_REVERSE "%s" COLOR_RESET, name);
    else if (strstr(name, ".tar") || strstr(name, ".gz") || strstr(name, ".zip"))
                                  printf(COLOR_RED "%s" COLOR_RESET, name);
    else if (st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))
                                  printf(COLOR_GREEN "%s" COLOR_RESET, name);
    else                           printf("%s", name);
}

/* Long listing (simple): permissions char, size, mtime, name (colored) */
static void display_long(char **names, int n, const char *dirpath) {
    struct stat st;
    char full[PATH_MAX], tbuf[64];
    for (int i = 0; i < n; ++i) {
        if (snprintf(full, sizeof(full), "%s/%s", dirpath, names[i]) >= (int)sizeof(full)) {
            printf("? %10s  %s\n", "-", names[i]);
            continue;
        }
        if (lstat(full, &st) == -1) {
            perror("lstat");
            continue;
        }
        /* type */
        char t = S_ISDIR(st.st_mode) ? 'd' : (S_ISLNK(st.st_mode) ? 'l' : '-');
        /* basic perms (owner only for brevity) */
        char p[11] = "----------";
        p[0] = t;
        p[1] = (st.st_mode & S_IRUSR) ? 'r' : '-';
        p[2] = (st.st_mode & S_IWUSR) ? 'w' : '-';
        p[3] = (st.st_mode & S_IXUSR) ? 'x' : '-';
        /* size and mtime */
        strftime(tbuf, sizeof(tbuf), "%b %d %H:%M", localtime(&st.st_mtime));
        printf("%s %8ld %s ", p, (long)st.st_size, tbuf);
        print_colored_name(dirpath, names[i]);
        printf("\n");
    }
}

/* Vertical (one per line, colored) */
static void display_vertical(char **names, int n, const char *dirpath) {
    for (int i = 0; i < n; ++i) {
        print_colored_name(dirpath, names[i]);
        printf("\n");
    }
}

/* Horizontal (row-major) padded columns */
static void display_horizontal(char **names, int n, const char *dirpath) {
    int termw = get_term_width();
    int maxlen = 0;
    for (int i = 0; i < n; ++i) {
        int L = (int)strlen(names[i]);
        if (L > maxlen) maxlen = L;
    }
    int colw = maxlen + 2;
    if (colw <= 0) colw = 1;
    int pos = 0;
    for (int i = 0; i < n; ++i) {
        /* print name colored, then pad to colw */
        /* we'll capture colored output into a buffer by printing to stdout then pad approximate */
        /* simpler: print colored name, then print spaces to reach colw (note color codes don't count) */
        /* so we pad based on raw name length (not terminal-visible length when colors present). */
        print_colored_name(dirpath, names[i]);
        int pad = colw - (int)strlen(names[i]);
        if (pad < 1) pad = 1;
        for (int s = 0; s < pad; ++s) putchar(' ');
        pos += colw;
        if (pos + colw > termw) { putchar('\n'); pos = 0; }
    }
    if (pos != 0) putchar('\n');
}

/* Read directory entries into dynamic array (skips . and .. and hidden files) */
static char **read_dir_sorted(const char *dirpath, int *out_n) {
    DIR *dp = opendir(dirpath);
    if (!dp) {
        perror(dirpath);
        *out_n = 0;
        return NULL;
    }
    struct dirent *de;
    int cap = 64, n = 0;
    char **arr = malloc(cap * sizeof(char *));
    if (!arr) { perror("malloc"); closedir(dp); exit(EXIT_FAILURE); }

    while ((de = readdir(dp)) != NULL) {
        /* skip . and .. */
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) continue;
        /* skip hidden (optional) - comment out if you want to include hidden files */
        if (de->d_name[0] == '.') continue;
        if (n >= cap) {
            cap *= 2;
            char **tmp = realloc(arr, cap * sizeof(char *));
            if (!tmp) { perror("realloc"); closedir(dp); exit(EXIT_FAILURE); }
            arr = tmp;
        }
        arr[n++] = strdup_safe(de->d_name);
    }
    closedir(dp);
    if (n > 0) qsort(arr, n, sizeof(char *), cmp_str);
    *out_n = n;
    return arr;
}

/* Core recursive listing */
static void do_ls(const char *dirpath, int long_flag, int horiz_flag, int recursive_flag) {
    int n;
    char **names = read_dir_sorted(dirpath, &n);
    if (!names) return;

    /* print directory header for recursive mode (like ls -R) */
    if (recursive_flag) printf("%s:\n", dirpath);

    /* choose display */
    if (long_flag) display_long(names, n, dirpath);
    else if (horiz_flag) display_horizontal(names, n, dirpath);
    else display_vertical(names, n, dirpath);

    /* recurse into subdirectories */
    if (recursive_flag) {
        for (int i = 0; i < n; ++i) {
            char full[PATH_MAX];
            struct stat st;
            if (snprintf(full, sizeof(full), "%s/%s", dirpath, names[i]) >= (int)sizeof(full)) continue;
            if (lstat(full, &st) == -1) continue;
            if (S_ISDIR(st.st_mode)) {
                /* avoid infinite loops (though . and .. were skipped) */
                do_ls(full, long_flag, horiz_flag, recursive_flag);
            }
        }
    }

    for (int i = 0; i < n; ++i) free(names[i]);
    free(names);
}

/* main: parse -l, -x, -R */
int main(int argc, char *argv[]) {
    int opt;
    int long_flag = 0, horiz_flag = 0, recursive_flag = 0;

    while ((opt = getopt(argc, argv, "lRx")) != -1) {
        switch (opt) {
            case 'l': long_flag = 1; break;
            case 'R': recursive_flag = 1; break;
            case 'x': horiz_flag = 1; break;
            default:
                fprintf(stderr, "Usage: %s [-lRx] [dir]\n", argv[0]);
                return 2;
        }
    }

    const char *dirpath = (optind < argc) ? argv[optind] : ".";
    do_ls(dirpath, long_flag, horiz_flag, recursive_flag);
    return 0;
}
