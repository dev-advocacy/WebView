#include "pch.h"
#include "RegisterMessages.h"

namespace Messaging
{

std::wstring CRegisteredMessages::ProcessGuidString()
{
    GUID guid{};
    if (FAILED(::CoCreateGuid(&guid)))
        return L"578EC919-665F-4363-94CB-EB62343EFB26";

    wchar_t buffer[39] = {};
    if (::StringFromGUID2(guid, buffer, static_cast<int>(std::size(buffer))) <= 0)
        return L"578EC919-665F-4363-94CB-EB62343EFB26";

    return buffer;
}

UINT CRegisteredMessages::RegisterPerProcess(PCWSTR variableName)
{
    const std::wstring messageName = std::wstring(variableName) + L"_" + ProcessGuidString();
    return ::RegisterWindowMessageW(messageName.c_str());
}

bool CRegisteredMessages::Initialize()
{
    std::call_once(s_initFlag, []()
        {
            s_navigateCallback = RegisterPerProcess(L"MSG_NAVIGATE_CALLBACK");
            s_runAsyncCallback = RegisterPerProcess(L"MSG_RUN_ASYNC_CALLBACK");
            s_runFunctor = RegisterPerProcess(L"WM_RUN_FUNCTOR");

            s_initialized = (s_navigateCallback != 0) &&
                (s_runAsyncCallback != 0) &&
                (s_runFunctor != 0);
        });

    return s_initialized;
}

bool CRegisteredMessages::IsInitialized()
{
    return s_initialized;
}

UINT CRegisteredMessages::NavigateCallback()
{
    return s_navigateCallback;
}

UINT CRegisteredMessages::RunAsyncCallback()
{
    return s_runAsyncCallback;
}

UINT CRegisteredMessages::RunFunctor()
{
    return s_runFunctor;
}

} // namespace Messaging