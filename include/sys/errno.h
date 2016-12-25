/*
 * Copyright (c) 2016 Simon Schmidt
 *
 * All or some portions of this file are derived from specifications of UNIX
 * error codes as seen in other UNIX-systems and UNIX-like systems such as Linux,
 * BSD, research-UNIX version 7, and System V. For the fundamental source, that
 * inspired the structure of this file, I credit:
 *  - Bell Labs (AT&T/Unix System Laboratories, Inc.), the team that worked on
 *                             UNIX especially Dennis Ritchie and Ken Thompson.
 *  - CSRG at University of California, Berkeley.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#pragma once

/* UNIX error numbers. */
#define EPERM    1 /* Operation not permitted */
#define ENOENT   2 /* No such file or directory */
#define ESRCH    3 /* No such process */
#define EINTR    4 /* Interrupted system call */
#define EIO      5 /* Input/output error */
#define ENXIO    6 /* Device not configured */
#define E2BIG    7 /* Argument list too long */
#define ENOEXEC  8 /* Exec format error */
#define EBADF    9 /* Bad file descriptor */
#define ECHILD  10 /* No child processes */
#define EAGAIN  11 /* Try again */ 	
#define ENOMEM  12 /* Cannot allocate memory */
#define EACCES  13 /* Permission denied */
#define EFAULT  14 /* Bad address */
#define ENOTBLK 15 /* Block device required */
#define EBUSY   16 /* Device busy */
#define EEXIST  17 /* File exists */
#define EXDEV   18 /* Cross-device link */
#define ENODEV  19 /* Operation not supported by device */
#define ENOTDIR 20 /* Not a directory */
#define EISDIR  21 /* Is a directory */
#define EINVAL  22 /* Invalid argument */
#define ENFILE  23 /* Too many open files in system */
#define EMFILE  24 /* Too many open files */
#define ENOTTY  25 /* Inappropriate ioctl for device */
#define ETXTBSY 26 /* Text file busy */
#define EFBIG   27 /* File too large */
#define ENOSPC  28 /* No space left on device */
#define ESPIPE  29 /* Illegal seek */
#define EROFS   30 /* Read-only file system */
#define EMLINK  31 /* Too many links */
#define EPIPE   32 /* Broken pipe */

/* math software */
#define EDOM    33 /*Numerical argument out of domain */
#define ERANGE  34 /* Result too large */

/* addenda to the UNIX error numbers. */
#define ENOSYS        100 /* Function not implemented */
#define	ELOOP         101 /* Too many levels of symbolic links */
#define	ENAMETOOLONG  102 /* File name too long */


#define	EDEADLK	35 /* Resource deadlock avoided */
#define	ENOLCK  77 /* No locks available */

/* non-blocking and interrupt i/o */
#define	EWOULDBLOCK   EAGAIN /* Operation would block */
#define	EINPROGRESS   36 /* Operation now in progress */
#define	EALREADY      37 /* Operation already in progress */


