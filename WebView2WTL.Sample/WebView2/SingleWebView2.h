#pragma once

class SingleWebView2 
{
public:
    std::map<std::string, int32_t>  m_country;
    SingleWebView2() {
        std::ifstream ifs("country.txt");
        std::string city, population;
        while (getline(ifs, city)) {
            getline(ifs, population);
            m_country[city] = stoi(population);
        }
    }

    wil::com_ptr<ICoreWebView2Environment> get_webViewEnvironment();
    void set_webViewEnvironment(wil::com_ptr<ICoreWebView2Environment> CoreWebView2Environment);


public:
    SingleWebView2(SingleWebView2 const&) = delete;
    SingleWebView2& operator=(SingleWebView2 const&) = delete;
    static SingleWebView2& get() 
    {
        static SingleWebView2 db;
        return db;
    }
    int32_t get_population(const std::string& name) { return m_country[name]; 
    }
private:
    wil::com_ptr<ICoreWebView2Environment>	m_webViewEnvironment = nullptr;
};