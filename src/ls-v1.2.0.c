#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>

void column_display_mode(const char *dir);
void long_listing_mode(const char *dir);

int main(int argc, char *argv[]) {
    int opt;
    int long_listing = 0;

    while ((opt = getopt(argc, argv, "l")) != -1) {
        switch (opt) {
            case 'l': long_listing = 1; break;
            default:
                fprintf(stderr, "Usage: %s [-l] [directory]\n", argv[0]);
                return 1;
        }
    }

    const char *target_dir = (optind < argc) ? argv[optind] : ".";

    if (long_listing)
        long_listing_mode(target_dir);
    else
        column_display_mode(target_dir);

    return 0;
}

// ------------------------------
// COLUMN DISPLAY MODE (Feature 3)
// ------------------------------
void column_display_mode(const char *dir) {
    DIR *dp = opendir(dir);
    if (!dp) { perror("Cannot open directory"); return; }

    struct dirent *entry;
    size_t count = 0, capacity = 100, max_len = 0;
    char **names = malloc(capacity * sizeof(char *));
    if (!names) { perror("malloc"); closedir(dp); return; }

    while ((entry = readdir(dp)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        if (count >= capacity) {
            capacity *= 2;
            names = realloc(names, capacity * sizeof(char *));
        }
        names[count] = strdup(entry->d_name);
        size_t len = strlen(entry->d_name);
        if (len > max_len) max_len = len;
        count++;
    }
    closedir(dp);

    struct winsize w;
    int term_width = 80;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0)
        term_width = w.ws_col;

    int spacing = 2;
    int col_width = max_len + spacing;
    int num_cols = term_width / col_width;
    if (num_cols < 1) num_cols = 1;
    int num_rows = (count + num_cols - 1) / num_cols;

    for (int r = 0; r < num_rows; r++) {
        for (int c = 0; c < num_cols; c++) {
            int idx = c * num_rows + r;
            if (idx < count)
                printf("%-*s", col_width, names[idx]);
        }
        printf("\n");
    }

    for (size_t i = 0; i < count; i++) free(names[i]);
    free(names);
}

// ------------------------------
// LONG LISTING MODE (From v1.1.0)
// ------------------------------
void long_listing_mode(const char *dir) {
    DIR *dp = opendir(dir);
    if (!dp) { perror("Cannot open directory"); return; }

    struct dirent *entry;
    struct stat st;
    char path[1024];

    while ((entry = readdir(dp)) != NULL) {
        if (entry->d_name[0] == '.') continue;

        snprintf(path, sizeof(path), "%s/%s", dir, entry->d_name);
        if (stat(path, &st) == -1) { perror("stat"); continue; }

        printf("%c", S_ISDIR(st.st_mode) ? 'd' : '-');
        printf("%c", (st.st_mode & S_IRUSR) ? 'r' : '-');
        printf("%c", (st.st_mode & S_IWUSR) ? 'w' : '-');
        printf("%c", (st.st_mode & S_IXUSR) ? 'x' : '-');
        printf(" %5ld %s %s %8ld %s\n",
               st.st_nlink,
               getpwuid(st.st_uid)->pw_name,
               getgrgid(st.st_gid)->gr_name,
               st.st_size,
               entry->d_name);
    }
    closedir(dp);
}
