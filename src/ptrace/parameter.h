#pragma once

#include <vector>
#include <stdio.h>
#include <signal.h>
#include <poll.h>
#include <sys/user.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <syscall.h>
#include <linux/futex.h>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"

#include <sys/utsname.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/sysinfo.h>
#include <sys/ioctl.h>
#include <net/if.h>


namespace SAIL { namespace core {

enum ParameterIndex
{
    Ret,
    First,
    Second,
    Third,
    Fourth,
    Fifth,
    Sixth,
};

enum ParameterType
{
    pointer,
    lvalue,
    str,    // end of 0
    structp, // fixed size
    structpp, // fixed size
    pArray, // char * p[]
    null,
};

struct Parameter
{
    static Parameter lvalue() { return {ParameterType::lvalue, 0, 0}; }
    static Parameter pointer(ParameterIndex sizefrom) { return {ParameterType::pointer, sizefrom, 0}; }
    static Parameter str(long size) { return {ParameterType::str, size, 0}; }
    static Parameter structp(long fixed_size) { return {ParameterType::structp, fixed_size, 0}; }
    static Parameter structpp(long fixed_size) { return {ParameterType::structpp, fixed_size, 0}; }
    static Parameter pArray(long size) { return {ParameterType::pArray, size, 0}; }
    
    ParameterType type;
    long size;  // size of object pointed
    long value;

    std::shared_ptr<char> buf; // using for pointer, str, structp
    std::vector<std::string> value_vector; // using for pArray

    Parameter() : type(ParameterType::null) {}
    Parameter(ParameterType type, long size, long value) : type(type), size(size), value(value), buf(nullptr), value_vector() {}
};


struct RuleCheckMsg
{
    bool approval;
    int ruleID;
    std::string ruleName;
    std::string msg;
};

struct Systemcall {
    struct user_regs_struct call_regs;
    struct user_regs_struct ret_regs;
    Systemcall() {};
};

struct WarnInfo {
    // a WarnInfo is related to one specific systemcall
    // callID is that systemcall's index in history
    int callID;

    // TODO: add explanation to this warning
    // e.g. vector<int> breakRules;  show the rules that are broken;  
};

using Parameters = std::vector<Parameter>;
using SystemcallParaTable = std::vector<Parameters>;
using Histories = std::vector<std::pair<Systemcall, Parameters>>;
using RuleCheckMsgs = std::vector<RuleCheckMsg>;

using mystat = struct stat;
using mysigaction = struct sigaction;
using mysysinfo = struct sysinfo;

const SystemcallParaTable syscall_call_para_table = {
    {Parameter::lvalue(), Parameter::lvalue(), Parameter::pointer(ParameterIndex::Ret), Parameter::lvalue()}, // {0, "read"}
    {Parameter::lvalue(), Parameter::lvalue(), Parameter::pointer(ParameterIndex::Ret), Parameter::lvalue()}, // {1, "write"}
    {Parameter::lvalue(), Parameter::str(256), Parameter::lvalue(), Parameter::lvalue()}, // {2, "open"}
    {Parameter::lvalue(), Parameter::lvalue()}, // {3, "close"}
    {Parameter::lvalue(), Parameter::str(256), Parameter::structp(sizeof(mystat))},// {4, "stat"},
    {Parameter::lvalue(), Parameter::lvalue(), Parameter::structp(sizeof(mystat))},// {5, "fstat"},
    {Parameter::lvalue(), Parameter::str(256), Parameter::structp(sizeof(mystat))},// {6, "lstat"},
    {Parameter::lvalue(), Parameter::structp(sizeof(pollfd)), Parameter::lvalue(), Parameter::lvalue()},// {7, "poll"},
    {Parameter::lvalue(), Parameter::lvalue(), Parameter::lvalue(), Parameter::lvalue()},// {8, "lseek"},
    {Parameter::lvalue(), Parameter::lvalue(), Parameter::lvalue(), Parameter::lvalue(), Parameter::lvalue(), Parameter::lvalue(), Parameter::lvalue()},// {9, "mmap"},
    {Parameter::lvalue(), Parameter::lvalue(), Parameter::lvalue(), Parameter::lvalue()},// {10, "mprotect"},
    {Parameter::lvalue(), Parameter::lvalue(), Parameter::lvalue()},// {11, "munmap"},
    {Parameter::lvalue(), Parameter::lvalue()},// {12, "brk"},
    {Parameter::lvalue(), Parameter::lvalue(), Parameter::structp(sizeof(mysigaction)), Parameter::structp(sizeof(mysigaction))},// {13, "rt_sigaction"},
    {Parameter::lvalue(), Parameter::lvalue(), Parameter::structp(sizeof(sigset_t)), Parameter::structp(sizeof(sigset_t)), Parameter::lvalue()},// {14, "rt_sigprocmask"},
    {},// {15, "rt_sigreturn"},
    {Parameter::lvalue(), Parameter::lvalue(), Parameter::lvalue()},// {16, "ioctl"},
    {},// {17, "pread64"},
    {},// {18, "pwrite64"},
    {},// {19, "readv"},
    {},// {20, "writev"},
    {Parameter::lvalue(), Parameter::str(256), Parameter::lvalue()},// {21, "access"},
    {Parameter::lvalue(), Parameter::structp(sizeof(int[2]))},// {22, "pipe"},
    {Parameter::lvalue(), Parameter::lvalue(), Parameter::structp(sizeof(fd_set)), Parameter::structp(sizeof(fd_set)), Parameter::structp(sizeof(fd_set)), Parameter::structp(sizeof(timeval))},// {23, "select"},
    {},// {24, "sched_yield"},
    {},// {25, "mremap"},
    {},// {26, "msync"},
    {},// {27, "mincore"},
    {},// {28, "madvise"},
    {},// {29, "shmget"},
    {},// {30, "shmat"},
    {},// {31, "shmctl"},
    {Parameter::lvalue(), Parameter::lvalue()},// {32, "dup"},
    {Parameter::lvalue(), Parameter::lvalue(), Parameter::lvalue()},// {33, "dup2"},
    {},// {34, "pause"},
    {},// {35, "nanosleep"},
    {},// {36, "getitimer"},
    {},// {37, "alarm"},
    {},// {38, "setitimer"},
    {Parameter::lvalue()},// {39, "getpid"},
    {},// {40, "sendfile"},
    {Parameter::lvalue(), Parameter::lvalue(), Parameter::lvalue(), Parameter::lvalue()},// {41, "socket"},
    {Parameter::lvalue(), Parameter::lvalue(), Parameter::pointer(ParameterIndex::Third), Parameter::lvalue()}, // {42, "connect"},
    {},// {43, "accept"},
    {Parameter::lvalue(), Parameter::lvalue(), Parameter::pointer(ParameterIndex::Third), Parameter::lvalue(), Parameter::lvalue(), Parameter::lvalue(), Parameter::lvalue()},// {44, "sendto"},
    {Parameter::lvalue(), Parameter::lvalue(), Parameter::pointer(ParameterIndex::Third), Parameter::lvalue(), Parameter::lvalue(), Parameter::lvalue(), Parameter::lvalue()},// {45, "recvfrom"},
    {},// {46, "sendmsg"},
    {},// {47, "recvmsg"},
    {},// {48, "shutdown"},
    {},// {49, "bind"},
    {},// {50, "listen"},
    {Parameter::lvalue(), Parameter::lvalue(), Parameter::structp(sizeof(sockaddr)), Parameter::structp(sizeof(socklen_t))},// {51, "getsockname"},
    {Parameter::lvalue(), Parameter::lvalue(), Parameter::structp(sizeof(sockaddr)), Parameter::structp(sizeof(socklen_t))},// {52, "getpeername"},
    {Parameter::lvalue(), Parameter::lvalue(), Parameter::lvalue(), Parameter::lvalue(), Parameter::structp(sizeof(int[2]))},// {53, "socketpair"},
    {Parameter::lvalue(), Parameter::lvalue(), Parameter::lvalue(), Parameter::lvalue(), Parameter::pointer(ParameterIndex::Fifth), Parameter::lvalue()},// {54, "setsockopt"},
    {},// {55, "getsockopt"},
    {Parameter::lvalue(), Parameter::lvalue(), Parameter::lvalue(), Parameter::lvalue(), Parameter::lvalue(), Parameter::lvalue(), Parameter::lvalue()},// {56, "clone"},
    {},// {57, "fork"},
    {},// {58, "vfork"},
    {Parameter::lvalue(), Parameter::str(256), Parameter::pArray(-1), Parameter::pArray(-1)},// {59, "execve"},
    {},// {60, "exit"},
    {Parameter::lvalue(), Parameter::lvalue(), Parameter::structp(sizeof(int)), Parameter::lvalue(), Parameter::structp(sizeof(rusage))},// {61, "wait4"},
    {},// {62, "kill"},
    {Parameter::lvalue(), Parameter::structp(sizeof(utsname))},// {63, "uname"},
    {},// {64, "semget"},
    {},// {65, "semop"},
    {},// {66, "semctl"},
    {},// {67, "shmdt"},
    {},// {68, "msgget"},
    {},// {69, "msgsnd"},
    {},// {70, "msgrcv"},
    {},// {71, "msgctl"},
    {Parameter::lvalue(), Parameter::lvalue(), Parameter::lvalue()},// {72, "fcntl"},
    {},// {73, "flock"},
    {},// {74, "fsync"},
    {},// {75, "fdatasync"},
    {},// {76, "truncate"},
    {},// {77, "ftruncate"},
    {},// {78, "getdents"},
    {},// {79, "getcwd"},
    {},// {80, "chdir"},
    {},// {81, "fchdir"},
    {},// {82, "rename"},
    {},// {83, "mkdir"},
    {},// {84, "rmdir"},
    {},// {85, "creat"},
    {},// {86, "link"},
    {},// {87, "unlink"},
    {},// {88, "symlink"},
    {},// {89, "readlink"},
    {},// {90, "chmod"},
    {},// {91, "fchmod"},
    {},// {92, "chown"},
    {},// {93, "fchown"},
    {},// {94, "lchown"},
    {},// {95, "umask"},
    {},// {96, "gettimeofday"},
    {Parameter::lvalue(), Parameter::lvalue(), Parameter::structp(sizeof(rlimit))},// {97, "getrlimit"},
    {},// {98, "getrusage"},
    {Parameter::lvalue(), Parameter::structp(sizeof(mysysinfo))},// {99, "sysinfo"},
    {},// {100, "times"},
    {},// {101, "ptrace"},
    {Parameter::lvalue()},// {102, "getuid"},
    {},// {103, "syslog"},
    {Parameter::lvalue()},// {104, "getgid"},
    {},// {105, "setuid"},
    {},// {106, "setgid"},
    {Parameter::lvalue()},// {107, "geteuid"},
    {Parameter::lvalue()},// {108, "getegid"},
    {},// {109, "setpgid"},
    {Parameter::lvalue()},// {110, "getppid"},
    {},// {111, "getpgrp"},
    {},// {112, "setsid"},
    {},// {113, "setreuid"},
    {},// {114, "setregid"},
    {},// {115, "getgroups"},
    {},// {116, "setgroups"},
    {},// {117, "setresuid"},
    {},// {118, "getresuid"},
    {},// {119, "setresgid"},
    {},// {120, "getresgid"},
    {},// {121, "getpgid"},
    {},// {122, "setfsuid"},
    {},// {123, "setfsgid"},
    {},// {124, "getsid"},
    {},// {125, "capget"},
    {},// {126, "capset"},
    {},// {127, "rt_sigpending"},
    {},// {128, "rt_sigtimedwait"},
    {},// {129, "rt_sigqueueinfo"},
    {},// {130, "rt_sigsuspend"},
    {},// {131, "sigaltstack"},
    {},// {132, "utime"},
    {},// {133, "mknod"},
    {},// {134, "uselib"},
    {},// {135, "personality"},
    {},// {136, "ustat"},
    {},// {137, "statfs"},
    {},// {138, "fstatfs"},
    {},// {139, "sysfs"},
    {},// {140, "getpriority"},
    {},// {141, "setpriority"},
    {},// {142, "sched_setparam"},
    {},// {143, "sched_getparam"},
    {},// {144, "sched_setscheduler"},
    {},// {145, "sched_getscheduler"},
    {},// {146, "sched_get_priority_max"},
    {},// {147, "sched_get_priority_min"},
    {},// {148, "sched_rr_get_interval"},
    {},// {149, "mlock"},
    {},// {150, "munlock"},
    {},// {151, "mlockall"},
    {},// {152, "munlockall"},
    {},// {153, "vhangup"},
    {},// {154, "modify_ldt"},
    {},// {155, "pivot_root"},
    {},// {156, "_sysctl"},
    {},// {157, "prctl"},
    {Parameter::lvalue(), Parameter::lvalue(), Parameter::lvalue()},// {158, "arch_prctl"},
    {},// {159, "adjtimex"},
    {},// {160, "setrlimit"},
    {},// {161, "chroot"},
    {},// {162, "sync"},
    {},// {163, "acct"},
    {},// {164, "settimeofday"},
    {},// {165, "mount"},
    {},// {166, "umount2"},
    {},// {167, "swapon"},
    {},// {168, "swapoff"},
    {},// {169, "reboot"},
    {},// {170, "sethostname"},
    {},// {171, "setdomainname"},
    {},// {172, "iopl"},
    {},// {173, "ioperm"},
    {},// {174, "create_module"},
    {},// {175, "init_module"},
    {},// {176, "delete_module"},
    {},// {177, "get_kernel_syms"},
    {},// {178, "query_module"},
    {},// {179, "quotactl"},
    {},// {180, "nfsservctl"},
    {},// {181, "getpmsg"},
    {},// {182, "putpmsg"},
    {},// {183, "afs_syscall"},
    {},// {184, "tuxcall"},
    {},// {185, "security"},
    {},// {186, "gettid"},
    {},// {187, "readahead"},
    {},// {188, "setxattr"},
    {},// {189, "lsetxattr"},
    {},// {190, "fsetxattr"},
    {},// {191, "getxattr"},
    {},// {192, "lgetxattr"},
    {},// {193, "fgetxattr"},
    {},// {194, "listxattr"},
    {},// {195, "llistxattr"},
    {},// {196, "flistxattr"},
    {},// {197, "removexattr"},
    {},// {198, "lremovexattr"},
    {},// {199, "fremovexattr"},
    {},// {200, "tkill"},
    {},// {201, "time"},
    {Parameter::lvalue(), Parameter::structp(sizeof(int)), Parameter::lvalue(), Parameter::lvalue(), Parameter::structp(sizeof(timespec)), Parameter::structp(sizeof(int)), Parameter::lvalue()},// {202, "futex"},
    {},// {203, "sched_setaffinity"},
    {},// {204, "sched_getaffinity"},
    {},// {205, "set_thread_area"},
    {},// {206, "io_setup"},
    {},// {207, "io_destroy"},
    {},// {208, "io_getevents"},
    {},// {209, "io_submit"},
    {},// {210, "io_cancel"},
    {},// {211, "get_thread_area"},
    {},// {212, "lookup_dcookie"},
    {},// {213, "epoll_create"},
    {},// {214, "epoll_ctl_old"},
    {},// {215, "epoll_wait_old"},
    {},// {216, "remap_file_pages"},
    {},// {217, "getdents64"},
    {Parameter::lvalue(), Parameter::structp(sizeof(int))},// {218, "set_tid_address"},
    {},// {219, "restart_syscall"},
    {},// {220, "semtimedop"},
    {},// {221, "fadvise64"},
    {},// {222, "timer_create"},
    {},// {223, "timer_settime"},
    {},// {224, "timer_gettime"},
    {},// {225, "timer_getoverrun"},
    {},// {226, "timer_delete"},
    {},// {227, "clock_settime"},
    {},// {228, "clock_gettime"},
    {},// {229, "clock_getres"},
    {},// {230, "clock_nanosleep"},
    {},// {231, "exit_group"},
    {},// {232, "epoll_wait"},
    {},// {233, "epoll_ctl"},
    {Parameter::lvalue(), Parameter::lvalue(), Parameter::lvalue(), Parameter::lvalue()},// {234, "tgkill"},
    {},// {235, "utimes"},
    {},// {236, "vserver"},
    {},// {237, "mbind"},
    {},// {238, "set_mempolicy"},
    {},// {239, "get_mempolicy"},
    {},// {240, "mq_open"},
    {},// {241, "mq_unlink"},
    {},// {242, "mq_timedsend"},
    {},// {243, "mq_timedreceive"},
    {},// {244, "mq_notify"},
    {},// {245, "mq_getsetattr"},
    {},// {246, "kexec_load"},
    {},// {247, "waitid"},
    {},// {248, "add_key"},
    {},// {249, "request_key"},
    {},// {250, "keyctl"},
    {},// {251, "ioprio_set"},
    {},// {252, "ioprio_get"},
    {},// {253, "inotify_init"},
    {},// {254, "inotify_add_watch"},
    {},// {255, "inotify_rm_watch"},
    {},// {256, "migrate_pages"},
    {Parameter::lvalue(), Parameter::lvalue(), Parameter::str(256), Parameter::lvalue(), Parameter::lvalue()},// {257, "openat"},
    {},// {258, "mkdirat"},
    {},// {259, "mknodat"},
    {},// {260, "fchownat"},
    {},// {261, "futimesat"},
    {},// {262, "newfstatat"},
    {},// {263, "unlinkat"},
    {},// {264, "renameat"},
    {},// {265, "linkat"},
    {},// {266, "symlinkat"},
    {},// {267, "readlinkat"},
    {},// {268, "fchmodat"},
    {},// {269, "faccessat"},
    {},// {270, "pselect6"},
    {},// {271, "ppoll"},
    {},// {272, "unshare"},
    {Parameter::lvalue(), Parameter::lvalue(), Parameter::structpp(sizeof(robust_list_head)), Parameter::structp(sizeof(size_t))},// {273, "set_robust_list"},
    {Parameter::lvalue(), Parameter::structp(sizeof(robust_list_head)), Parameter::lvalue()},// {274, "get_robust_list"},
    {},// {275, "splice"},
    {},// {276, "tee"},
    {},// {277, "sync_file_range"},
    {},// {278, "vmsplice"},
    {},// {279, "move_pages"},
    {},// {280, "utimensat"},
    {},// {281, "epoll_pwait"},
    {},// {282, "signalfd"},
    {},// {283, "timerfd_create"},
    {},// {284, "eventfd"},
    {},// {285, "fallocate"},
    {},// {286, "timerfd_settime"},
    {},// {287, "timerfd_gettime"},
    {},// {288, "accept4"},
    {},// {289, "signalfd4"},
    {},// {290, "eventfd2"},
    {},// {291, "epoll_create1"},
    {},// {292, "dup3"},
    {Parameter::lvalue(), Parameter::structp(sizeof(int[2])), Parameter::lvalue()},// {293, "pipe2"},
    {},// {294, "inotify_init1"},
    {},// {295, "preadv"},
    {},// {296, "pwritev"},
    {},// {297, "rt_tgsigqueueinfo"},
    {},// {298, "perf_event_open"},
    {},// {299, "recvmmsg"},
    {},// {300, "fanotify_init"},
    {},// {301, "fanotify_mark"},
    {},// {302, "prlimit64"},
    {},// {303, "name_to_handle_at"},
    {},// {304, "open_by_handle_at"},
    {},// {305, "clock_adjtime"},
    {},// {306, "syncfs"},
    {},// {307, "sendmmsg"},
    {},// {308, "setns"},
    {},// {309, "getcpu"},
    {},// {310, "process_vm_readv"},
    {},// {311, "process_vm_writev"},
    {},// {312, "kcmp"},
    {},// {313, "finit_module"},
    {},// {314, "sched_setattr"},
    {},// {315, "sched_getattr"},
    {},// {316, "renameat2"},
    {},// {317, "seccomp"},
    {},// {318, "getrandom"},
    {},// {319, "memfd_create"},
    {},// {320, "kexec_file_load"},
    {},// {321, "bpf"},
    {},// {322, "execveat"},
    {},// {323, "userfaultfd"},
    {},// {324, "membarrier"},
    {},// {325, "mlock2"}
    };

}}


