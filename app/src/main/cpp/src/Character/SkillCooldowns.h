// Monotonic v83 skill cooldowns. AGPL-3.0-or-later.
#pragma once
#include <chrono>
#include <unordered_map>
namespace ms {
class SkillCooldowns {
public:
    using Clock=std::chrono::steady_clock;
    void set(int id,int seconds,Clock::time_point now=Clock::now()) {
        if(seconds<=0) until_.erase(id);else until_[id]=now+std::chrono::seconds(seconds);
    }
    bool active(int id,Clock::time_point now=Clock::now()) const {auto it=until_.find(id);return it!=until_.end() && now<it->second;}
private: std::unordered_map<int,Clock::time_point> until_;
};
}
