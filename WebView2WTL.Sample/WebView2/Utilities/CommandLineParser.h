#pragma once

#include <string>
#include <map>
#include <optional>

class CommandLineParser
{
public:
    void AddOption(const std::string& name, const std::string& description)
    {
        m_options[name] = description;
    }

    bool Parse(int argc, wchar_t* argv[])
    {
        for (int i = 1; i < argc; ++i)
        {
            std::wstring arg = argv[i];
            if (arg.starts_with(L"--") || arg.starts_with(L"-"))
            {
                std::wstring option = arg.substr(arg.starts_with(L"--") ? 2 : 1);
                size_t pos = option.find(L'=');

                if (pos != std::wstring::npos)
                {
                    // Syntax: --option=value
                    m_values[option.substr(0, pos)] = option.substr(pos + 1);
                }
                else
                {
                    // Syntax: --option value (next argument is the value)
                    if (i + 1 < argc && !std::wstring(argv[i + 1]).starts_with(L"-"))
                    {
                        // Next argument doesn't start with -, treat it as the value
                        m_values[option] = argv[++i];
                    }
                    else
                    {
                        // No value following, store empty string
                        m_values[option] = L"";
                    }
                }
            }
        }
        return true;
    }

    bool HasOption(const std::wstring& name) const
    {
        return m_values.contains(name);
    }

    std::optional<std::wstring> GetValue(const std::wstring& name) const
    {
        auto it = m_values.find(name);
        return it != m_values.end() ? std::optional(it->second) : std::nullopt;
    }

    std::wstring GetValueOr(const std::wstring& name, const std::wstring& defaultValue) const
    {
        return GetValue(name).value_or(defaultValue);
    }

    std::string GetHelpMessage() const
    {
        std::string help = "Available options:\n";
        for (const auto& [name, description] : m_options)
            help += "  --" + name + " : " + description + "\n";
        return help;
    }

private:
    std::map<std::string, std::string> m_options;
    std::map<std::wstring, std::wstring> m_values;
};
