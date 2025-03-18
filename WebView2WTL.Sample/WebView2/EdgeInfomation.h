#pragma once

class EdgeInfomation
{
public:
	EdgeInfomation(std::wstring name, std::wstring version, std::wstring location) : m_name(name), m_version(version), m_install_location(location) {}
	std::wstring m_name;
	std::wstring m_version;
	std::wstring m_install_location;
};