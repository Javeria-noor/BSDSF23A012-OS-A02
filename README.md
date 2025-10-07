# BSDSF23A012-OS-A02
# Operating Systems Programming Assignment 02
**Student Name:** Javeria Noor  
**Roll No:** BSDSF23A012 
**Project:** Re-engineering the `ls` Utility  

## Project Overview
This project is a step-by-step re-engineering of the standard Unix `ls` command.  
Starting from a basic implementation (`ls-v1.0.0`), the project adds multiple advanced features, mimicking GNU/Linux `ls` behavior.  

**Objectives:**
- Learn system-level programming using C.
- Understand file metadata, permissions, and system calls (`stat`, `lstat`, `getpwuid`, `getgrgid`).
- Implement output formatting including long listing and column display.
- Apply dynamic memory, sorting algorithms, recursion, and ANSI color codes.
- Practice professional Git workflow with branches, commits, tags, and releases.

---

## Features and Versions

### v1.0.0 – Initial Setup
- Base project setup.
- Simple `ls` functionality.
- Verified compilation with Makefile.

### v1.1.0 – Long Listing Format (-l)
- Displays file metadata in `ls -l` style.
- Uses `stat()`, `getpwuid()`, `getgrgid()`, and `ctime()`.
- Shows permissions, owner/group, size, and modification time.

### v1.2.0 – Column Display (Down Then Across)
- Default multi-column output.
- Dynamically adjusts columns based on terminal width.
- Prints filenames **down then across**.

### v1.3.0 – Horizontal Column Display (-x)
- Adds horizontal row-major layout with `-x` option.
- Files print left-to-right, wrapping to next line as needed.

### v1.4.0 – Alphabetical Sort
- Sorts directory contents alphabetically.
- Applies to default, `-l`, and `-x` displays.

### v1.5.0 – Colorized Output
- Uses ANSI escape codes to color files by type:
  - **Directory:** Blue
  - **Executable:** Green
  - **Archive (.tar, .gz, .zip):** Red
  - **Symbolic Links:** Pink
  - **Special files:** Reverse Video

### v1.6.0 – Recursive Listing (-R)
- Implements recursive directory traversal with `-R`.
- Lists subdirectories with full paths.
- Mimics standard `ls -R` behavior.

---

## Project Structure
ROLL_NO-OS-A02/
├── src/
│ └── ls-v1.0.0.c
├── bin/ # Compiled executables
├── obj/ # Object files (optional)
├── man/ # For bonus man page task
├── Makefile # Build automation
├── README.md # This file
└── REPORT.md # Assignment report with Q&A


