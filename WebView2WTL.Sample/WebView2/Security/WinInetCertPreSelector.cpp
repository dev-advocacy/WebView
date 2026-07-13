#include "pch.h"
#include "WinInetCertPreSelector.h"
#include "../Logger/logger.h"
#include "../Utilities/Utility.h"

namespace webview::net
{
	// -----------------------------------------------------------------------
	// WinInetCertPreSelector implementation
	// -----------------------------------------------------------------------

	std::string WinInetCertPreSelector::Run(const std::wstring& url,
											const std::wstring& certSubjectFilter)
	{
		SelectedCertInfo info;
		HttpsDownloader  dl;
		std::string      body = dl.Download(url, certSubjectFilter, &info);

		if (info.certContext)
		{
			std::lock_guard<std::mutex> lock(m_mutex);

			auto it = FindByEndpointNoLock(info.host, info.port);
			if (it != m_certInfos.end())
			{
				*it = std::move(info);
				m_lastSelectedIndex = static_cast<size_t>(std::distance(m_certInfos.begin(), it));
			}
			else
			{
				m_certInfos.emplace_back(std::move(info));
				m_lastSelectedIndex = m_certInfos.size() - 1;
			}
		}

		return body;
	}

	bool WinInetCertPreSelector::HasMatchFor(const std::wstring& host, INTERNET_PORT port) const noexcept
	{
		if (!m_enabled.load(std::memory_order_acquire))
			return false;

		std::lock_guard lock(m_mutex);
		const auto it = FindByEndpointNoLock(host, port);
		return (it != m_certInfos.end());
	}

	std::wstring WinInetCertPreSelector::GetSubjectFor(const std::wstring& host, INTERNET_PORT port) const
	{
		std::lock_guard lock(m_mutex);
		const auto it = FindByEndpointNoLock(host, port);
		return (it != m_certInfos.end()) ? it->subject : L"";
	}

	std::wstring WinInetCertPreSelector::GetIssuerFor(const std::wstring& host, INTERNET_PORT port) const
	{
		std::lock_guard lock(m_mutex);
		const auto it = FindByEndpointNoLock(host, port);
		return (it != m_certInfos.end()) ? it->issuer : L"";
	}

	const CERT_CONTEXT* WinInetCertPreSelector::GetCertContextFor(const std::wstring& host, INTERNET_PORT port) const noexcept
	{
		std::lock_guard lock(m_mutex);
		const auto it = FindByEndpointNoLock(host, port);
		return (it != m_certInfos.end()) ? it->certContext.get() : nullptr;
	}

	std::wstring WinInetCertPreSelector::GetPemEncodingFor(const std::wstring& host, INTERNET_PORT port) const
	{
		std::lock_guard lock(m_mutex);
		const auto it = FindByEndpointNoLock(host, port);
		if (it == m_certInfos.end() || !it->certContext)
			return {};
		return ClientCertificateSelector::GetPemEncoding(it->certContext.get());
	}

	const CERT_CONTEXT* WinInetCertPreSelector::GetCertContext() const noexcept
	{
		std::lock_guard lock(m_mutex);
		const SelectedCertInfo* info = GetLastSelectedNoLock();
		return info ? info->certContext.get() : nullptr;
	}

	std::wstring WinInetCertPreSelector::GetSubject() const
	{
		std::lock_guard lock(m_mutex);
		const SelectedCertInfo* info = GetLastSelectedNoLock();
		return info ? info->subject : L"";
	}

	std::wstring WinInetCertPreSelector::GetIssuer() const
	{
		std::lock_guard lock(m_mutex);
		const SelectedCertInfo* info = GetLastSelectedNoLock();
		return info ? info->issuer : L"";
	}

	std::wstring WinInetCertPreSelector::GetHost() const
	{
		std::lock_guard lock(m_mutex);
		const SelectedCertInfo* info = GetLastSelectedNoLock();
		return info ? info->host : L"";
	}

	INTERNET_PORT WinInetCertPreSelector::GetPort() const noexcept
	{
		std::lock_guard lock(m_mutex);
		const SelectedCertInfo* info = GetLastSelectedNoLock();
		return info ? info->port : 0;
	}

	size_t WinInetCertPreSelector::Count() const noexcept
	{
		std::lock_guard lock(m_mutex);
		return m_certInfos.size();
	}

	void WinInetCertPreSelector::Clear() noexcept
	{
		std::lock_guard lock(m_mutex);
		m_certInfos.clear();
		m_lastSelectedIndex.reset();
	}

	WinInetCertPreSelector::CertStoreIterator WinInetCertPreSelector::FindByEndpointNoLock(const std::wstring& host, INTERNET_PORT port)
	{
		return std::find_if(m_certInfos.begin(), m_certInfos.end(),
			[&host, port](const SelectedCertInfo& item)
			{
				return item.port == port && item.host == host;
			});
	}

	WinInetCertPreSelector::CertStoreConstIterator WinInetCertPreSelector::FindByEndpointNoLock(const std::wstring& host, INTERNET_PORT port) const
	{
		return std::find_if(m_certInfos.begin(), m_certInfos.end(),
			[&host, port](const SelectedCertInfo& item)
			{
				return item.port == port && item.host == host;
			});
	}

	const SelectedCertInfo* WinInetCertPreSelector::GetLastSelectedNoLock() const noexcept
	{
		if (!m_lastSelectedIndex.has_value())
			return nullptr;
		if (*m_lastSelectedIndex >= m_certInfos.size())
			return nullptr;
		return &m_certInfos[*m_lastSelectedIndex];
	}

} // namespace webview::net
