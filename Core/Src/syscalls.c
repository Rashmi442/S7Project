/**
 ******************************************************************************
 * @file    syscalls.c
 * @brief   Minimal newlib syscall stubs for a bare-metal STM32CubeIDE
 *          project (no OS, no filesystem). Standard CubeIDE-generated file.
 ******************************************************************************
 */
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/times.h>

__attribute__((weak)) int _read(int file, char *ptr, int len) { (void)file; (void)ptr; (void)len; return 0; }
__attribute__((weak)) int _write(int file, char *ptr, int len)
{
    (void)file;
    (void)ptr;
    return len; /* no stdio backend wired up; retarget to USART if desired */
}

int _close(int file) { (void)file; return -1; }
int _fstat(int file, struct stat *st) { (void)file; st->st_mode = S_IFCHR; return 0; }
int _isatty(int file) { (void)file; return 1; }
int _lseek(int file, int ptr, int dir) { (void)file; (void)ptr; (void)dir; return 0; }
int _open(const char *path, int flags, ...) { (void)path; (void)flags; return -1; }
int _kill(int pid, int sig) { (void)pid; (void)sig; errno = EINVAL; return -1; }
void _exit(int status) { _kill(status, -1); while (1) { } }
int _getpid(void) { return 1; }

/* _sbrk() lives in sysmem.c (matches CubeIDE's default project split). */
