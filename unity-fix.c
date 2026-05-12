#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdarg.h>

/*
 * unity-fix.c - LD_PRELOAD workaround for Unity 4.x "Player is already running" bug
 *
 * The bug: Unity 4.x (pre-5.4.0) scans /proc for other processes and compares
 * their executable paths to detect duplicate instances. Chromium/CEF/Electron
 * processes list "/proc/self/exe" as their command line. When Unity reads this
 * from ITS OWN /proc/self perspective, it resolves to its own executable,
 * causing a false "already running" detection.
 *
 * This library intercepts /proc reads and replaces "/proc/self/exe" with
 * a harmless fake path so Unity doesn't mistake other processes for itself.
 *
 * Build:
 *   64-bit: gcc -shared -fPIC -ldl -o unity-fix-64.so unity-fix.c
 *   32-bit: gcc -m32 -shared -fPIC -ldl -o unity-fix-32.so unity-fix.c
 *
 * Usage:
 *   LD_PRELOAD=/path/to/unity-fix-32.so ./Game binary
 *
 * Affected games include: Shadowrun Returns/Dragonfall/HK, Infinifactory,
 * Motorsport Manager, Creeper World 3, Poly Bridge, and many other
 * Unity 4.x titles.
 */

/* --------------------------------------------------------------- *
 * fopen/fread intercept (stdio path)                              *
 * --------------------------------------------------------------- */
static FILE *opened_proc_file = NULL;

FILE *fopen(const char *pathname, const char *mode) {
  static FILE *(*real_fopen)(const char *, const char *) = NULL;
  if (!real_fopen)
    real_fopen = dlsym(RTLD_NEXT, "fopen");
  opened_proc_file = NULL;

  FILE *ret = real_fopen(pathname, mode);

  if (ret && strncmp(pathname, "/proc/", 6) == 0)
    opened_proc_file = ret;

  return ret;
}

FILE *fopen64(const char *pathname, const char *mode) {
  static FILE *(*real_fopen64)(const char *, const char *) = NULL;
  if (!real_fopen64)
    real_fopen64 = dlsym(RTLD_NEXT, "fopen64");
  opened_proc_file = NULL;

  FILE *ret = real_fopen64(pathname, mode);

  if (ret && strncmp(pathname, "/proc/", 6) == 0)
    opened_proc_file = ret;

  return ret;
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
  static size_t (*real_fread)(void *, size_t, size_t, FILE *) = NULL;
  if (!real_fread)
    real_fread = dlsym(RTLD_NEXT, "fread");

  size_t ret = real_fread(ptr, size, nmemb, stream);
  if (ret > 0 && stream == opened_proc_file &&
      strncmp(ptr, "/proc/self/exe", 14) == 0)
    memcpy(ptr, "/dev/null-FAKE00", 16);
  return ret;
}

/* --------------------------------------------------------------- *
 * open/read intercept (direct I/O path)                            *
 * --------------------------------------------------------------- */
static int opened_proc_fd = -1;

int open(const char *pathname, int flags, ...) {
  static int (*real_open)(const char *, int, ...) = NULL;
  if (!real_open)
    real_open = dlsym(RTLD_NEXT, "open");

  int mode = 0;
  if (flags & O_CREAT) {
    va_list ap;
    va_start(ap, flags);
    mode = va_arg(ap, int);
    va_end(ap);
  }

  int fd = real_open(pathname, flags, mode);
  if (fd >= 0 && strncmp(pathname, "/proc/", 6) == 0)
    opened_proc_fd = fd;
  return fd;
}

int open64(const char *pathname, int flags, ...) {
  static int (*real_open64)(const char *, int, ...) = NULL;
  if (!real_open64)
    real_open64 = dlsym(RTLD_NEXT, "open64");

  int mode = 0;
  if (flags & O_CREAT) {
    va_list ap;
    va_start(ap, flags);
    mode = va_arg(ap, int);
    va_end(ap);
  }

  int fd = real_open64(pathname, flags, mode);
  if (fd >= 0 && strncmp(pathname, "/proc/", 6) == 0)
    opened_proc_fd = fd;
  return fd;
}

ssize_t read(int fd, void *buf, size_t count) {
  static ssize_t (*real_read)(int, void *, size_t) = NULL;
  if (!real_read)
    real_read = dlsym(RTLD_NEXT, "read");

  ssize_t ret = real_read(fd, buf, count);
  if (ret > 0 && fd == opened_proc_fd &&
      strncmp(buf, "/proc/self/exe", 14) == 0)
    memcpy(buf, "/dev/null-FAKE00", 16);
  return ret;
}

/* --------------------------------------------------------------- *
 * close intercept (reset tracked fd)                               *
 * --------------------------------------------------------------- */
int close(int fd) {
  static int (*real_close)(int) = NULL;
  if (!real_close)
    real_close = dlsym(RTLD_NEXT, "close");

  if (fd == opened_proc_fd)
    opened_proc_fd = -1;

  return real_close(fd);
}

/* --------------------------------------------------------------- *
 * readlink intercept (symlink resolution path)                     *
 * --------------------------------------------------------------- */
ssize_t readlink(const char *pathname, char *buf, size_t bufsiz) {
  static ssize_t (*real_readlink)(const char *, char *, size_t) = NULL;
  if (!real_readlink)
    real_readlink = dlsym(RTLD_NEXT, "readlink");

  ssize_t ret = real_readlink(pathname, buf, bufsiz);
  if (ret > 0 && strstr(pathname, "/proc/") != NULL &&
      strncmp(buf, "/proc/self/exe", 14) == 0) {
    const char *fake = "/dev/null-FAKE00";
    size_t flen = strlen(fake);
    size_t copy = flen < bufsiz ? flen : bufsiz;
    memcpy(buf, fake, copy);
    ret = (ssize_t)copy;
  }
  return ret;
}
