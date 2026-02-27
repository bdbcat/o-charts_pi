 
#pragma once

#include <windows.h>
#include <string>
#include <stdexcept>

class WinJobObject
{
public:
    WinJobObject()
    {
        m_job = CreateJobObject(nullptr, nullptr);
        if (!m_job)
            throw std::runtime_error("CreateJobObject failed");

        JOBOBJECT_EXTENDED_LIMIT_INFORMATION info = {};
        info.BasicLimitInformation.LimitFlags =
            JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;

        if (!SetInformationJobObject(
                m_job,
                JobObjectExtendedLimitInformation,
                &info,
                sizeof(info)))
        {
            CloseHandle(m_job);
            throw std::runtime_error("SetInformationJobObject failed");
        }
    }

    ~WinJobObject()
    {
        if (m_job)
            CloseHandle(m_job);   // 🔥 Triggers automatic process kill
    }

    // Non-copyable
    WinJobObject(const WinJobObject&) = delete;
    WinJobObject& operator=(const WinJobObject&) = delete;

    // Movable
    WinJobObject(WinJobObject&& other) noexcept
        : m_job(other.m_job)
    {
        other.m_job = nullptr;
    }

    WinJobObject& operator=(WinJobObject&& other) noexcept
    {
        if (this != &other)
        {
            if (m_job)
                CloseHandle(m_job);

            m_job = other.m_job;
            other.m_job = nullptr;
        }
        return *this;
    }

    // Launch process and assign to job
    PROCESS_INFORMATION Launch(const std::wstring& exePath,
                               const std::wstring& arguments = L"",
                               DWORD creationFlags = CREATE_NO_WINDOW)
    {
        STARTUPINFOW si = {};
        si.cb = sizeof(si);

        PROCESS_INFORMATION pi = {};

        std::wstring exe = exePath;

        // Strip accidental surrounding quotes
        if (exe.length() >= 2 && exe.front() == L'"' && exe.back() == L'"') {
            exe = exe.substr(1, exe.length() - 2);
        }

        // Build properly quoted command line
        std::wstring cmd = L"\"" + exe + L"\"";

        if (!arguments.empty())
        {
            cmd += L" ";
            cmd += arguments;
        }

        if (!CreateProcessW(
                exe.c_str(),      // safer: explicit executable
                cmd.data(),           // full command line
                nullptr,
                nullptr,
                FALSE,
                creationFlags,
                nullptr,
                nullptr,
                &si,
                &pi))
        {
            DWORD err = GetLastError();

            wchar_t* buffer = nullptr;
            FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                               FORMAT_MESSAGE_FROM_SYSTEM |
                               FORMAT_MESSAGE_IGNORE_INSERTS,
                           nullptr, err, 0, (LPWSTR)&buffer, 0, nullptr);

            std::wstring msg = buffer ? buffer : L"Unknown error";
            LocalFree(buffer);

            throw std::runtime_error("CreateProcess failed. Error " +
                                     std::to_string(err));
         }

        if (!AssignProcessToJobObject(m_job, pi.hProcess))
        {
            TerminateProcess(pi.hProcess, 1);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            throw std::runtime_error("AssignProcessToJobObject failed");
        }

        return pi;
    }




private:
    HANDLE m_job = nullptr;
};
