#pragma once
#include <string>
#include <unordered_map>

class Config
{
public:
    bool load(const std::string& path);
    void save(const std::string& path) const;
    std::string get(const std::string& section, const std::string& key, const std::string& defaultValue = "") const;
    void set(const std::string& section, const std::string& key, const std::string& value);

private:
    std::unordered_map<std::string, std::string> values;
    std::string makeKey(const std::string& section, const std::string& key) const;
};