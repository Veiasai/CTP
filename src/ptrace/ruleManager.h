#pragma once

#include <vector>
#include <map>
#include <yaml-cpp/yaml.h>
#include <memory>

#include "rule.h"


namespace SAIL { namespace rule {

class RuleManager
{
public:
    virtual ~RuleManager() {};
    virtual std::vector<core::RuleCheckMsg> check(int syscallNumber, const core::SyscallParameter & sp) = 0;
};

class YamlRuleManager : public RuleManager
{
private:
    // map from syscall number to rules applied
    std::map<int, std::vector<std::unique_ptr<Rule>>> rules;
public:
    YamlRuleManager(const YAML::Node & yaml);
    virtual ~YamlRuleManager() {};
    virtual std::vector<core::RuleCheckMsg> check(int syscall, const core::SyscallParameter & sp) override;
};

}}