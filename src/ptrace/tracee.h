#pragma once

#include <sys/user.h>
#include <vector>
#include <memory>

#include "utils.h"

namespace SAIL { namespace core {

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
    // e.g. vector<int> breakRules;  show the rules that be breaked;  
};

class Tracee
{
public:
    virtual ~Tracee() {};
    virtual void trap() = 0;
    virtual const std::vector<Systemcall> & getHistory() = 0;
    virtual const std::vector<WarnInfo> & getReport() = 0;
};

class TraceeImpl : public Tracee
{
private:
    int tid;
    volatile bool iscalling;
    std::vector<Systemcall> history;
    std::vector<WarnInfo> report;
    std::shared_ptr<utils::Utils> up;
    std::shared_ptr<utils::CustomPtrace> cp;
    // file
    void open();
    void read();
    void write();

    // net
    void connect();
    void recvfrom();
    void sendto();

    // clone
    void clone();
public:
    TraceeImpl(int tid, std::shared_ptr<utils::Utils> up, std::shared_ptr<utils::CustomPtrace> cp);
    virtual ~TraceeImpl() {};
    virtual void trap();
    virtual const std::vector<Systemcall> & getHistory();
    virtual const std::vector<WarnInfo> & getReport();
};

}}
