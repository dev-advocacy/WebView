#pragma once


class CRegisteredMessages
{
public:
    static bool Initialize();
    static bool IsInitialized();

    static UINT NavigateCallback();
    static UINT RunAsyncCallback();
    static UINT RunFunctor();

private:
    static std::wstring ProcessGuidString();
    static UINT RegisterPerProcess(PCWSTR variableName);

private:
    static inline std::once_flag s_initFlag;
    static inline bool s_initialized = false;

    static inline UINT s_navigateCallback = 0;
    static inline UINT s_runAsyncCallback = 0;
    static inline UINT s_runFunctor = 0;
};