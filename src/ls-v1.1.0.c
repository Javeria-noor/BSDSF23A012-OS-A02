#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>

// Function declarations
void simple_listing_mode(const char *dir);
void long_listing_mode(const char *dir);

int main(int argc, char *argv[]) {
    int opt;
    int long_listing = 0;

    // Parse command-line arguments
    while ((opt = getopt(argc, argv, "l")) != -1) {
        switch (opt) {
            case 'l':
                long_listing = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s [-l] [directory]\n", argv[0]);
                return 1;
        }
    }

    const char *target_dir = (optind < argc) ? argv[optind] : ".";

    if (long_listing)
        long_listing_mode(target_dir);
    else
        simple_listing_mode(target_dir);

    return 0;
}

// -------------------------
// Simple Listing (-l not used)
// -------------------------
void simple_listing_mode(const char *dir) {
    DIR *dp;
    struct dirent *entry;

    dp = opendir(dir);
    if (dp == NULL) {
        perror("Cannot open directory");
        return;
    }

    while ((entry = readdir(dp)) != NULL) {
        if (entry->d_name[0] != '.')
            printf("%s\n", entry->d_name);
    }

    closedir(dp);
}

// -------------------------
// Long Listing (-l option)
// -------------------------
void long_listing_mode(const char *dir) {
    DIR *dp;
    struct dirent *entry;
    struct stat st;
    char path[1024];
    char timebuf[64];

    dp = opendir(dir);
    if (dp == NULL) {
        perror("Cannot open directory");
        return;
    }

    while ((entry = readdir(dp)) != NULL) {
        if (entry->d_name[0] == '.')
            continue;

        snprintf(path, sizeof(path), "%s/%s", dir, entry->d_name);

        if (stat(path, &st) == -1) {
            perror("stat");
            continue;
        }

        // File type and permissions
        printf((S_ISDIR(st.st_mode)) ? "d" : "-");
        printf((st.st_mode & S_IRUSR) ? "r" : "-");
        printf((st.st_mode & S_IWUSR) ? "w" : "-");
        printf((st.st_mode & S_IXUSR) ? "x" : "-");
        printf((st.st_mode & S_IRGRP) ? "r" : "-");
        printf((st.st_mode & S_IWGRP) ? "w" : "-");
        printf((st.st_mode & S_IXGRP) ? "x" : "-");
        printf((st.st_mode & S_IROTH) ? "r" : "-");
        printf((st.st_mode & S_IWOTH) ? "w" : "-");
        printf((st.st_mode & S_IXOTH) ? "x" : "-");

        // Owner, group, size, and time
        struct passwd *pw = getpwuid(st.st_uid);
        struct group *gr = getgrgid(st.st_gid);
        strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", localtime(&st.st_mtime));

        printf(" %3lu %-8s %-8s %8ld %s %s\n",
               st.st_nlink,
               pw ? pw->pw_name : "?",
               gr ? gr->gr_name : "?",
               st.st_size,
               timebuf,
               entry->d_name);
    }

    closedir(dp);
}
