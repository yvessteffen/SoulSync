#include "Config.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

std::string Config::makeKey(const std::string& section, const std::string& key) const
{
    return section + "." + key;
}

bool Config::load(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cout << "Config not found at " << path << ", using defaults\n";
        return false;
    }

    std::string line;
    std::string currentSection;

    while (std::getline(file, line))
    {
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) continue;
        line = line.substr(start);

        if (line[0] == '#' || line[0] == ';') continue;

        if (line[0] == '[')
        {
            size_t end = line.find(']');
            if (end != std::string::npos)
                currentSection = line.substr(1, end - 1);
            continue;
        }

        // key=value
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key   = line.substr(0, eq);
        std::string value = line.substr(eq + 1);

        auto trim = [](std::string& s) {
            size_t start = s.find_first_not_of(" \t\r\n");
            size_t end   = s.find_last_not_of(" \t\r\n");
            s = (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
        };
        trim(key);
        trim(value);

        values[makeKey(currentSection, key)] = value;
    }

    return true;
}

void Config::save(const std::string& path) const
{
    std::unordered_map<std::string, std::vector<std::pair<std::string,std::string>>> sections;
    for (auto& [k, v] : values)
    {
        size_t dot = k.find('.');
        std::string section = k.substr(0, dot);
        std::string key     = k.substr(dot + 1);
        sections[section].emplace_back(key, v);
    }

    std::ofstream file(path);
    for (auto& [section, pairs] : sections)
    {
        file << "[" << section << "]\n";
        for (auto& [key, val] : pairs)
            file << key << " = " << val << "\n";
        file << "\n";
    }
}

std::string Config::get(const std::string& section, const std::string& key, const std::string& defaultValue) const
{
    auto it = values.find(makeKey(section, key));
    return it != values.end() ? it->second : defaultValue;
}

void Config::set(const std::string& section, const std::string& key, const std::string& value)
{
    values[makeKey(section, key)] = value;
}