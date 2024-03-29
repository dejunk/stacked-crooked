#include <iomanip>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/user.h>
#include <sys/reg.h>
#include <sys/syscall.h>
#include <stddef.h>
#include <stdio.h>
#include <iostream>
#include <cassert>
#include <cstring>
#include <string>
#include <stdexcept>


//#define ISLINUX32(x)      (linux_call_type((x)->cs) == LINUX32)
#define SYSCALL_NUM(x)        (x)->orig_rax
#define SET_RETURN_CODE(x, v) (x)->rax = (v)
#define RETURN_CODE(x)        (ISLINUX32(x) ? (long)(int)(x)->rax : (x)->rax)
#define ARGUMENT_0(x)     ((x)->rdi)
#define ARGUMENT_1(x)     ((x)->rsi)
#define ARGUMENT_2(x)     ((x)->rdx)
#define ARGUMENT_3(x)     ((x)->rcx)
#define ARGUMENT_4(x)     ((x)->r8)
#define ARGUMENT_5(x)     ((x)->r9)
#define SET_ARGUMENT_0(x, v)  (x)->rdi = (v)
#define SET_ARGUMENT_1(x, v)  (x)->rsi = (v)
#define SET_ARGUMENT_2(x, v)  (x)->rdx = (v)
#define SET_ARGUMENT_3(x, v)  (x)->rcx = (v)
#define SET_ARGUMENT_4(x, v)  (x)->r8 = (v)
#define SET_ARGUMENT_5(x, v)  (x)->r9 = (v)


enum
{
    offset_orig_rax = offsetof(user_regs_struct, orig_rax),
    offset_arg0 = offsetof(user_regs_struct, rdi),
    offset_arg1 = offsetof(user_regs_struct, rsi),
    offset_arg2 = offsetof(user_regs_struct, rdx),
    offset_arg3 = offsetof(user_regs_struct, rcx),
    offset_arg4 = offsetof(user_regs_struct, r8),
    offset_arg5 = offsetof(user_regs_struct, r9),
    offset_ret = offsetof(user_regs_struct, rax)
};


long get_arg0(const user_regs_struct & regs) { return regs.rdi; }
long get_arg1(const user_regs_struct & regs) { return regs.rsi; }
long get_arg2(const user_regs_struct & regs) { return regs.rdx; }
long get_arg3(const user_regs_struct & regs) { return regs.rcx; }
long get_arg4(const user_regs_struct & regs) { return regs.r8;  }
long get_arg5(const user_regs_struct & regs) { return regs.r9;  }


long get_arg0(pid_t child) { return ptrace(PTRACE_PEEKUSER, child, offset_arg0, NULL); }
long get_arg1(pid_t child) { return ptrace(PTRACE_PEEKUSER, child, offset_arg1, NULL); }
long get_arg2(pid_t child) { return ptrace(PTRACE_PEEKUSER, child, offset_arg2, NULL); }
long get_arg3(pid_t child) { return ptrace(PTRACE_PEEKUSER, child, offset_arg3, NULL); }
long get_arg4(pid_t child) { return ptrace(PTRACE_PEEKUSER, child, offset_arg4, NULL); }
long get_arg5(pid_t child) { return ptrace(PTRACE_PEEKUSER, child, offset_arg5, NULL); }


void getdata(pid_t child, long addr, char * str, int len)
{
    char * laddr;
    int i, j;
    union u
    {
        long val;
        char chars[sizeof(long)];
    } data;
    i = 0;
    j = len / sizeof(long);
    laddr = str;
    while (i < j)
    {
        data.val = ptrace(PTRACE_PEEKDATA, child, addr + i * sizeof(long), NULL);
        memcpy(laddr, data.chars, sizeof(long));
        ++i;
        laddr += sizeof(long);
    }
    j = len % sizeof(long);
    if (j != 0)
    {
        data.val = ptrace(PTRACE_PEEKDATA, child, addr + i * sizeof(long), NULL);
        memcpy(laddr, data.chars, j);
    }
    str[len] = '\0';
}


void putdata(pid_t child, long addr, char * str, int len)
{
    char * laddr;
    int i, j;
    union u
    {
        long val;
        char chars[sizeof(long)];
    } data;
    i = 0;
    j = len / sizeof(long);
    laddr = str;
    while (i < j)
    {
        memcpy(data.chars, laddr, sizeof(long));
        ptrace(PTRACE_POKEDATA, child,
               addr + i * sizeof(long), data.val);
        ++i;
        laddr += sizeof(long);
    }
    j = len % sizeof(long);
    if (j != 0)
    {
        memcpy(data.chars, laddr, j);
        ptrace(PTRACE_POKEDATA, child,
               addr + i * sizeof(long), data.val);
    }
}

const char * linux_syscallnames_64[] =
{
    "read", /* 0 */
    "write", /* 1 */
    "open", /* 2 */
    "close", /* 3 */
    "stat", /* 4 */
    "fstat", /* 5 */
    "lstat", /* 6 */
    "poll", /* 7 */
    "lseek", /* 8 */
    "mmap", /* 9 */
    "mprotect", /* 10 */
    "munmap", /* 11 */
    "brk", /* 12 */
    "rt_sigaction", /* 13 */
    "rt_sigprocmask", /* 14 */
    "rt_sigreturn", /* 15 */
    "ioctl", /* 16 */
    "pread64", /* 17 */
    "pwrite64", /* 18 */
    "readv", /* 19 */
    "writev", /* 20 */
    "access", /* 21 */
    "pipe", /* 22 */
    "select", /* 23 */
    "sched_yield", /* 24 */
    "mremap", /* 25 */
    "msync", /* 26 */
    "mincore", /* 27 */
    "madvise", /* 28 */
    "shmget", /* 29 */
    "shmat", /* 30 */
    "shmctl", /* 31 */
    "dup", /* 32 */
    "dup2", /* 33 */
    "pause", /* 34 */
    "nanosleep", /* 35 */
    "getitimer", /* 36 */
    "alarm", /* 37 */
    "setitimer", /* 38 */
    "getpid", /* 39 */
    "sendfile", /* 40 */
    "socket", /* 41 */
    "connect", /* 42 */
    "accept", /* 43 */
    "sendto", /* 44 */
    "recvfrom", /* 45 */
    "sendmsg", /* 46 */
    "recvmsg", /* 47 */
    "shutdown", /* 48 */
    "bind", /* 49 */
    "listen", /* 50 */
    "getsockname", /* 51 */
    "getpeername", /* 52 */
    "socketpair", /* 53 */
    "setsockopt", /* 54 */
    "getsockopt", /* 55 */
    "clone", /* 56 */
    "fork", /* 57 */
    "vfork", /* 58 */
    "execve", /* 59 */
    "exit", /* 60 */
    "wait4", /* 61 */
    "kill", /* 62 */
    "uname", /* 63 */
    "semget", /* 64 */
    "semop", /* 65 */
    "semctl", /* 66 */
    "shmdt", /* 67 */
    "msgget", /* 68 */
    "msgsnd", /* 69 */
    "msgrcv", /* 70 */
    "msgctl", /* 71 */
    "fcntl", /* 72 */
    "flock", /* 73 */
    "fsync", /* 74 */
    "fdatasync", /* 75 */
    "truncate", /* 76 */
    "ftruncate", /* 77 */
    "getdents", /* 78 */
    "getcwd", /* 79 */
    "chdir", /* 80 */
    "fchdir", /* 81 */
    "rename", /* 82 */
    "mkdir", /* 83 */
    "rmdir", /* 84 */
    "creat", /* 85 */
    "link", /* 86 */
    "unlink", /* 87 */
    "symlink", /* 88 */
    "readlink", /* 89 */
    "chmod", /* 90 */
    "fchmod", /* 91 */
    "chown", /* 92 */
    "fchown", /* 93 */
    "lchown", /* 94 */
    "umask", /* 95 */
    "gettimeofday", /* 96 */
    "getrlimit", /* 97 */
    "getrusage", /* 98 */
    "sysinfo", /* 99 */
    "times", /* 100 */
    "ptrace", /* 101 */
    "getuid", /* 102 */
    "syslog", /* 103 */
    "getgid", /* 104 */
    "setuid", /* 105 */
    "setgid", /* 106 */
    "geteuid", /* 107 */
    "getegid", /* 108 */
    "setpgid", /* 109 */
    "getppid", /* 110 */
    "getpgrp", /* 111 */
    "setsid", /* 112 */
    "setreuid", /* 113 */
    "setregid", /* 114 */
    "getgroups", /* 115 */
    "setgroups", /* 116 */
    "setresuid", /* 117 */
    "getresuid", /* 118 */
    "setresgid", /* 119 */
    "getresgid", /* 120 */
    "getpgid", /* 121 */
    "setfsuid", /* 122 */
    "setfsgid", /* 123 */
    "getsid", /* 124 */
    "capget", /* 125 */
    "capset", /* 126 */
    "rt_sigpending", /* 127 */
    "rt_sigtimedwait", /* 128 */
    "rt_sigqueueinfo", /* 129 */
    "rt_sigsuspend", /* 130 */
    "sigaltstack", /* 131 */
    "utime", /* 132 */
    "mknod", /* 133 */
    "uselib", /* 134 */
    "personality", /* 135 */
    "ustat", /* 136 */
    "statfs", /* 137 */
    "fstatfs", /* 138 */
    "sysfs", /* 139 */
    "getpriority", /* 140 */
    "setpriority", /* 141 */
    "sched_setparam", /* 142 */
    "sched_getparam", /* 143 */
    "sched_setscheduler", /* 144 */
    "sched_getscheduler", /* 145 */
    "sched_get_priority_max", /* 146 */
    "sched_get_priority_min", /* 147 */
    "sched_rr_get_interval", /* 148 */
    "mlock", /* 149 */
    "munlock", /* 150 */
    "mlockall", /* 151 */
    "munlockall", /* 152 */
    "vhangup", /* 153 */
    "modify_ldt", /* 154 */
    "pivot_root", /* 155 */
    "_sysctl", /* 156 */
    "prctl", /* 157 */
    "arch_prctl", /* 158 */
    "adjtimex", /* 159 */
    "setrlimit", /* 160 */
    "chroot", /* 161 */
    "sync", /* 162 */
    "acct", /* 163 */
    "settimeofday", /* 164 */
    "mount", /* 165 */
    "umount2", /* 166 */
    "swapon", /* 167 */
    "swapoff", /* 168 */
    "reboot", /* 169 */
    "sethostname", /* 170 */
    "setdomainname", /* 171 */
    "iopl", /* 172 */
    "ioperm", /* 173 */
    "create_module", /* 174 */
    "init_module", /* 175 */
    "delete_module", /* 176 */
    "get_kernel_syms", /* 177 */
    "query_module", /* 178 */
    "quotactl", /* 179 */
    "nfsservctl", /* 180 */
    "getpmsg", /* 181 */
    "putpmsg", /* 182 */
    "afs_syscall", /* 183 */
    "tuxcall", /* 184 */
    "security", /* 185 */
    "gettid", /* 186 */
    "readahead", /* 187 */
    "setxattr", /* 188 */
    "lsetxattr", /* 189 */
    "fsetxattr", /* 190 */
    "getxattr", /* 191 */
    "lgetxattr", /* 192 */
    "fgetxattr", /* 193 */
    "listxattr", /* 194 */
    "llistxattr", /* 195 */
    "flistxattr", /* 196 */
    "removexattr", /* 197 */
    "lremovexattr", /* 198 */
    "fremovexattr", /* 199 */
    "tkill", /* 200 */
    "time", /* 201 */
    "futex", /* 202 */
    "sched_setaffinity", /* 203 */
    "sched_getaffinity", /* 204 */
    "set_thread_area", /* 205 */
    "io_setup", /* 206 */
    "io_destroy", /* 207 */
    "io_getevents", /* 208 */
    "io_submit", /* 209 */
    "io_cancel", /* 210 */
    "get_thread_area", /* 211 */
    "lookup_dcookie", /* 212 */
    "epoll_create", /* 213 */
    "epoll_ctl_old", /* 214 */
    "epoll_wait_old", /* 215 */
    "remap_file_pages", /* 216 */
    "getdents64", /* 217 */
    "set_tid_address", /* 218 */
    "restart_syscall", /* 219 */
    "semtimedop", /* 220 */
    "fadvise64", /* 221 */
    "timer_create", /* 222 */
    "timer_settime", /* 223 */
    "timer_gettime", /* 224 */
    "timer_getoverrun", /* 225 */
    "timer_delete", /* 226 */
    "clock_settime", /* 227 */
    "clock_gettime", /* 228 */
    "clock_getres", /* 229 */
    "clock_nanosleep", /* 230 */
    "exit_group", /* 231 */
    "epoll_wait", /* 232 */
    "epoll_ctl", /* 233 */
    "tgkill", /* 234 */
    "utimes", /* 235 */
    "vserver", /* 236 */
    "mbind", /* 237 */
    "set_mempolicy", /* 238 */
    "get_mempolicy", /* 239 */
    "mq_open", /* 240 */
    "mq_unlink", /* 241 */
    "mq_timedsend", /* 242 */
    "mq_timedreceive", /* 243 */
    "mq_notify", /* 244 */
    "mq_getsetattr", /* 245 */
    "kexec_load", /* 246 */
    "waitid", /* 247 */
    "add_key", /* 248 */
    "request_key", /* 249 */
    "keyctl", /* 250 */
    "ioprio_set", /* 251 */
    "ioprio_get", /* 252 */
    "inotify_init", /* 253 */
    "inotify_add_watch", /* 254 */
    "inotify_rm_watch", /* 255 */
    "migrate_pages", /* 256 */
    "openat", /* 257 */
    "mkdirat", /* 258 */
    "mknodat", /* 259 */
    "fchownat", /* 260 */
    "futimesat", /* 261 */
    "newfstatat", /* 262 */
    "unlinkat", /* 263 */
    "renameat", /* 264 */
    "linkat", /* 265 */
    "symlinkat", /* 266 */
    "readlinkat", /* 267 */
    "fchmodat", /* 268 */
    "faccessat", /* 269 */
    "pselect6", /* 270 */
    "ppoll", /* 271 */
    "unshare", /* 272 */
    "set_robust_list", /* 273 */
    "get_robust_list", /* 274 */
    "splice", /* 275 */
    "tee", /* 276 */
    "sync_file_range", /* 277 */
    "vmsplice", /* 278 */
    "move_pages", /* 279 */
    "utimensat", /* 280 */
    "ORE_getcpu", /* 0 */
    "epoll_pwait", /* 281 */
    "signalfd", /* 282 */
    "timerfd_create", /* 283 */
    "eventfd", /* 284 */
    "fallocate", /* 285 */
    "timerfd_settime", /* 286 */
    "timerfd_gettime", /* 287 */
    "paccept", /* 288 */
    "signalfd4", /* 289 */
    "eventfd2", /* 290 */
    "epoll_create1", /* 291 */
    "dup3", /* 292 */
    "pipe2", /* 293 */
    "inotify_init1", /* 294 */
    NULL
};


void print(const user_regs_struct & regs)
{
    std::cout << "regs.r15: 0x" << std::hex << std::setw(8) << std::setfill('0') << regs.r15 << std::endl;
    std::cout << "regs.r14: 0x" << std::hex << std::setw(8) << std::setfill('0') << regs.r14 << std::endl;
    std::cout << "regs.r13: 0x" << std::hex << std::setw(8) << std::setfill('0') << regs.r13 << std::endl;
    std::cout << "regs.r12: 0x" << std::hex << std::setw(8) << std::setfill('0') << regs.r12 << std::endl;
    std::cout << "regs.rbp: 0x" << std::hex << std::setw(8) << std::setfill('0') << regs.rbp << std::endl;
    std::cout << "regs.rbx: 0x" << std::hex << std::setw(8) << std::setfill('0') << regs.rbx << std::endl;
    std::cout << "regs.r11: 0x" << std::hex << std::setw(8) << std::setfill('0') << regs.r11 << std::endl;
    std::cout << "regs.r10: 0x" << std::hex << std::setw(8) << std::setfill('0') << regs.r10 << std::endl;
    std::cout << "regs.r9: 0x" << std::hex << std::setw(8) << std::setfill('0') << regs.r9 << std::endl;
    std::cout << "regs.r8: 0x" << std::hex << std::setw(8) << std::setfill('0') << regs.r8 << std::endl;
    std::cout << "regs.rax: 0x" << std::hex << std::setw(8) << std::setfill('0') << regs.rax << std::endl;
    std::cout << "regs.rcx: 0x" << std::hex << std::setw(8) << std::setfill('0') << regs.rcx << std::endl;
    std::cout << "regs.rdx: 0x" << std::hex << std::setw(8) << std::setfill('0') << regs.rdx << std::endl;
    std::cout << "regs.rsi: 0x" << std::hex << std::setw(8) << std::setfill('0') << regs.rsi << std::endl;
    std::cout << "regs.rdi: 0x" << std::hex << std::setw(8) << std::setfill('0') << regs.rdi << std::endl;
    std::cout << "regs.orig_rax: 0x" << std::hex << std::setw(8) << std::setfill('0') << regs.orig_rax << std::endl;
    std::cout << "regs.rip: 0x" << std::hex << std::setw(8) << std::setfill('0') << regs.rip << std::endl;
    std::cout << "regs.cs: 0x" << std::hex << std::setw(8) << std::setfill('0') << regs.cs << std::endl;
    std::cout << "regs.eflags: 0x" << std::hex << std::setw(8) << std::setfill('0') << regs.eflags << std::endl;
    std::cout << "regs.rsp: 0x" << std::hex << std::setw(8) << std::setfill('0') << regs.rsp << std::endl;
    std::cout << "regs.ss: 0x" << std::hex << std::setw(8) << std::setfill('0') << regs.ss << std::endl;
    std::cout << "regs.fs_base: 0x" << std::hex << std::setw(8) << std::setfill('0') << regs.fs_base << std::endl;
    std::cout << "regs.gs_base: 0x" << std::hex << std::setw(8) << std::setfill('0') << regs.gs_base << std::endl;
    std::cout << "regs.ds: 0x" << std::hex << std::setw(8) << std::setfill('0') << regs.ds << std::endl;
    std::cout << "regs.es: 0x" << std::hex << std::setw(8) << std::setfill('0') << regs.es << std::endl;
    std::cout << "regs.fs: 0x" << std::hex << std::setw(8) << std::setfill('0') << regs.fs << std::endl;
    std::cout << "regs.gs: 0x" << std::hex << std::setw(8) << std::setfill('0') << regs.gs << std::endl;
}


void check(int n)
{
    assert(n == 0);
}




#define assert_eq(a, b) \
    if (a != b) std::cout << #a << "!=" << #b << " (a = " << a << ", b = " << b << ")" << std::endl;


void Abort() {}
#define ASSERT_FALSE(Condition) if (Condition) { std::cout << __FILE__ << ":" << __LINE__ << ": Should be false: " << #Condition << std::endl; Abort(); }
#define ASSERT_TRUE(Condition) if (!(Condition)) { std::cout << __FILE__ << ":" << __LINE__ << ": Should be true: " << #Condition << std::endl; Abort(); }
const char *byte_to_binary(int x)
{
    static char b[9];
    b[0] = '\0';

    int z;
    for (z = 128; z > 0; z >>= 1)
    {
        strcat(b, ((x & z) == z) ? "1" : "0");
    }

    return b;
}

pid_t child = 0;
bool allowed(const user_regs_struct & regs)
{

    switch (regs.orig_rax)
    {
        case SYS_access:
        case SYS_arch_prctl:
        case SYS_brk:
        case SYS_close:
        case SYS_exit_group:
        case SYS_fstat:
        case SYS_mmap:
        case SYS_mprotect:
        case SYS_munmap:
        case SYS_read:
        case SYS_write:
        {
            return true;
        }
        case SYS_open:
        {
            assert(get_arg1(regs) == 0);
            return true;
        }
        case SYS_execve:
        default:
        {
            return false;
        }
    };
}

int main(int, char ** argv)
{
    child = fork();


    if (child == 0)
    {
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        execl(argv[1], argv[1], NULL);
    }
    else
    {
        int inside_syscall = 0;
        while (1)
        {
            pid_t status;
            wait(&status);
            if (WIFEXITED(status))
            {
                break;
            }

            user_regs_struct regs = user_regs_struct();
            check(ptrace(PTRACE_GETREGS, child, NULL, &regs));

            static bool first_execve = true;
            if (first_execve)
            {
                assert(regs.orig_rax == SYS_execve);
                if (regs.orig_rax == SYS_execve)
                {
                    ptrace(PTRACE_SYSCALL, child, NULL, NULL);
                    first_execve = false;
                    continue;
                }
            }

            if (inside_syscall == 0)
            {
                if (allowed(regs))
                {
                    inside_syscall = 1;
                }
                else
                {
                    abort();
                }
            }
            else
            {
                inside_syscall = 0;
                std::cout << "/" << linux_syscallnames_64[regs.orig_rax] << std::endl;
            }
            ptrace(PTRACE_SYSCALL, child, NULL, NULL);
        }
    }
    return 0;
}
