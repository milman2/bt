#include "BTContext.h"

namespace bt
{

    // BTContext 구현
    BTContext::BTContext()
    {
        start_time_ = std::chrono::steady_clock::now();
    }

    void BTContext::set_data(const std::string& key, const std::any& value)
    {
        data_[key] = value;
    }

    std::any BTContext::get_data(const std::string& key) const
    {
        auto it = data_.find(key);
        if (it != data_.end())
        {
            return it->second;
        }
        return std::any{};
    }

    bool BTContext::has_data(const std::string& key) const
    {
        return data_.find(key) != data_.end();
    }

    void BTContext::remove_data(const std::string& key)
    {
        data_.erase(key);
    }

} // namespace bt
