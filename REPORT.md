### Q1. What is the crucial difference between the `stat()` and `lstat()` system calls?  
The `stat()` system call retrieves information about the file a path points to. If the path refers to a symbolic link, `stat()` follows that link and returns information about the *target* file.  
In contrast, `lstat()` retrieves information *about the symbolic link itself* rather than the file it points to.  

**In the context of the `ls` command:**  
It is more appropriate to use `lstat()` when you want to display details about symbolic links (for example, showing that a file is a link and where it points), instead of the linked target file.

---

### Q2. How does the `st_mode` field store file type and permissions, and how can bitwise operators be used?  
The `st_mode` field in the `struct stat` structure is an integer where:
- The **upper bits** encode the *file type* (regular file, directory, symbolic link, etc.).
- The **lower bits** encode the *file permission bits* (read, write, execute for user, group, others).

You can extract this information using **bitwise AND (`&`)** with predefined macros:
```c
if (st.st_mode & S_IFDIR)  // Checks if it's a directory
if (st.st_mode & S_IRUSR)  // Checks if the owner has read permission
if (st.st_mode & S_IXGRP)  // Checks if the group has execute permission
