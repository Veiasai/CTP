#include "utils.h"
#include <dirent.h>
#include <unistd.h>
#include <fstream>

const std::set<std::string> NONEEDFILENAME{".", "..", "0", "1", "2"};

namespace SAIL
{
namespace utils
{
UtilsImpl::UtilsImpl(std::shared_ptr<CustomPtrace> _cp) : cp(_cp),
     syscall_call_para_table(core::syscall_call_para_table), syscall_assist(core::syscall_assist)
{
    // TODO: optimize here
    for (auto & sysc : syscall_assist)
    {
        syscall_assist_r.insert(std::make_pair(sysc.second, sysc.first));
    }
}

int UtilsImpl::readStrFrom(int tid, const char *p, char *buf, size_t s)
{
    for (int i = 0; i < s; i += sizeof(long))
    {
        long val = this->cp->peekData(tid, (long)p + i);
        char *c = (char *)&val;
        for (int j = 0; j < 8; j++)
        {
            buf[i + j] = c[j];
            if (c[j] == '\0')
            {
                return 0;
            }
        }
    }
    return -1;
}

// if \0 appears in buf?
int UtilsImpl::readBytesFrom(int tid, const char *p, char *buf, size_t s)
{
    size_t count = 0;
    while (s - count > 8)
    {
        *(long *)(buf + count) = this->cp->peekData(tid, (long)p + count);
        // spdlog::debug("[tid: {}] [readBytesFrom] [{}]", tid, buf+count);
        count += 8;
    }

    if (s - count > 0)
    {
        long data = this->cp->peekData(tid, (long)p + count);
        char *bdata = (char *)&data;
        // spdlog::debug("[tid: {}] [readBytesFrom] [{}]", tid, bdata);
        for (int i = 0; count + i < s; i++)
        {
            buf[count + i] = bdata[i];
        }
    }
    return 0;
}

int UtilsImpl::getFilenameByFd(int tid, int fd, std::string &filename)
{
    std::string command = "lsof -p " + std::to_string(tid) + " | awk '{print $4, $9}' > tmpforfd";
    int r = system(command.c_str());
    if (r != 0)
    {
        spdlog::error("[tid: {}] [getFilenameByFd] lsof error", tid);
        return -1;
    }
    std::fstream fs;
    fs.open("tmpforfd", std::fstream::in);
    while (!fs.eof())
    {
        std::string _fd;
        std::string _name;
        fs >> _fd >> _name;
        if (_fd.substr(0, _fd.length() - 1) == std::to_string(fd))
        {
            // frop r/w mode char
            filename = _name;
            fs.close();
            return 0;
        }
    }
    // don't find fd required
    spdlog::error("[tid: {}] [getFilenameByFd] fd doesn't exist", tid);
    return -1;
}

int UtilsImpl::getFilenamesByProc(int tid, std::set<std::string> &fileset)
{
    DIR *dir;
    struct dirent *ent;
    std::string dirpath = "/proc/" + std::to_string(tid) + "/fd/";
    if ((dir = opendir(dirpath.c_str())) != NULL)
    {
        while ((ent = readdir(dir)) != NULL)
        {
            std::string filename = ent->d_name;

            if (NONEEDFILENAME.find(filename) == NONEEDFILENAME.end())
            {
                char buf[512] = {'\0'};
                filename = dirpath + ent->d_name;
                readlink(filename.c_str(), buf, 512);
                fileset.insert(std::string(buf));
            }
        }
        closedir(dir);
    }
    else
    {
        spdlog::error("[tid: {}] [getFilenamesByProc] opendir error", tid);
        return -1;
    }

    return 0;
}

int UtilsImpl::strset2file(const std::string &filename, const std::set<std::string> &fileset)
{
    std::fstream fs;

    fs.open(filename.c_str(), std::ios::out);

    if (!fs.is_open())
    {
        fs.clear();
        return -1;
    }

    for (auto it = fileset.begin(); it != fileset.end(); it++)
    {
        fs << *it << std::endl;
    }

    fs.close();

    return 0;
}

int UtilsImpl::handleEscape(const std::string &str, std::string &regStr)
{
    for (int i = 0; i < str.size(); i++)
    {
        switch (str[i]) {
            case '.':
                regStr.append("\\.");
                break;
            case '^':
                regStr.append("\\^");
                break;
            case '$':
                regStr.append("\\$");
                break;
            case '*':
                regStr.append("\\*");
                break;
            case '+':
                regStr.append("\\+");
                break;
            case '?':
                regStr.append("\\?");
                break;
            case '\\':
                regStr.append("\\\\");
                break;
            case '(':
                regStr.append("\\(");
                break;
            case ')':
                regStr.append("\\)");
                break;
            default:
                regStr.push_back(str[i]);
        }
    }
    return 0;
}

int UtilsImpl::formatBytes(const std::vector<unsigned char> &vc, std::string &formattedBytes)
{
    formattedBytes = "[";
    for (auto byte : vc)
    {
        formattedBytes += "0x";
        int ib = (int)byte;
        formattedBytes += (ib / 16 >= 10) ? (char)('A' + (ib / 16 - 10)) : (char)('0' + ib / 16);
        formattedBytes += (ib % 16 >= 10) ? (char)('A' + (ib % 16 - 10)) : (char)('0' + ib % 16);
        formattedBytes += ", ";
    }
    // drop last 2 redundant chars (, )
    formattedBytes.pop_back();
    formattedBytes.pop_back();
    formattedBytes += "]";
    return 0;
}

int UtilsImpl::formatBytes(const std::string &str, std::string &formattedBytes)
{
    formattedBytes = "[";
    for (auto byte : str)
    {
        formattedBytes += "0x";
        int ib = (int)byte;
        formattedBytes += (ib / 16 >= 10) ? (char)('A' + (ib / 16 - 10)) : (char)('0' + ib / 16);
        formattedBytes += (ib % 16 >= 10) ? (char)('A' + (ib % 16 - 10)) : (char)('0' + ib % 16);
        formattedBytes += ", ";
    }
    // drop last 2 redundant chars (, )
    formattedBytes.pop_back();
    formattedBytes.pop_back();
    formattedBytes += "]";
    return 0;
}

int UtilsImpl::sysname2num(const std::string & name, long & id)
{
    auto it = this->syscall_assist_r.find(name);
    if (it != this->syscall_assist_r.end())
    {
        id = it->second;
        return 0;
    }
    else
    {
        return -1;
    }
}

int UtilsImpl::sysnum2str(long id, std::string & name)
{
    auto it = this->syscall_assist.find(id);
    if (it != this->syscall_assist.end())
    {
        name = it->second;
        return 0;
    }
    else
    {
        return -1;
    }
}

int UtilsImpl::sysnum2parav(long id, core::Parameters& param)
{
    if (id < 0 || id >= syscall_call_para_table.size())
    {
        return -1;
    }
    else
    {
        param = syscall_call_para_table[id];
        return 0;
    }
}

long CustomPtraceImpl::peekUser(int tid, long addr)
{
    return ptrace(PTRACE_PEEKUSER, tid, addr, NULL);
}

long CustomPtraceImpl::getRegs(int tid, user_regs_struct *regs)
{
    return ptrace(PTRACE_GETREGS, tid, NULL, regs);
}

long CustomPtraceImpl::peekData(int tid, long addr)
{
    return ptrace(PTRACE_PEEKDATA, tid, addr, NULL);
}

long CustomPtraceImpl::attach(int tid)
{
    return ptrace(PTRACE_ATTACH, tid, NULL, NULL);
}

} // namespace utils
} // namespace SAIL