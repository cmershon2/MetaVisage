#include "utils/SingleInstance.h"
#include "utils/Logger.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <array>

namespace MetaVisage {

namespace {

constexpr const wchar_t* kMutexName = L"Global\\MetaVisageSingleInstanceMutex";
constexpr const wchar_t* kPipeName  = L"\\\\.\\pipe\\MetaVisageSingleInstance";
constexpr DWORD kPipeBufferSize = 4096;

} // namespace

SingleInstance::SingleInstance() = default;

SingleInstance::~SingleInstance() {
    Release();
}

bool SingleInstance::TryAcquire(const std::string& payload, RaiseCallback onRaise) {
    // CreateMutexW returns the same handle regardless of whether we created it
    // or opened an existing one; GetLastError() == ERROR_ALREADY_EXISTS is the
    // signal that another instance holds ownership.
    HANDLE mutex = CreateMutexW(nullptr, TRUE, kMutexName);
    if (!mutex) {
        MV_LOG_WARNING("SingleInstance: CreateMutexW failed");
        return true; // Fail open — don't block startup on Win32 weirdness.
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        // Poke the primary instance via the named pipe, then bail out.
        HANDLE pipe = CreateFileW(kPipeName, GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (pipe != INVALID_HANDLE_VALUE) {
            DWORD written = 0;
            WriteFile(pipe, payload.data(), static_cast<DWORD>(payload.size()), &written, nullptr);
            CloseHandle(pipe);
        }
        ReleaseMutex(mutex);
        CloseHandle(mutex);
        return false;
    }

    mutexHandle_ = mutex;
    onRaise_ = std::move(onRaise);
    stop_ = false;
    listener_ = std::thread(&SingleInstance::ListenLoop, this);
    return true;
}

void SingleInstance::Release() {
    stop_ = true;
    // CreateFileW to our own pipe so the blocking ConnectNamedPipe returns.
    HANDLE wake = CreateFileW(kPipeName, GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (wake != INVALID_HANDLE_VALUE) CloseHandle(wake);

    if (listener_.joinable()) listener_.join();

    if (mutexHandle_) {
        ReleaseMutex(static_cast<HANDLE>(mutexHandle_));
        CloseHandle(static_cast<HANDLE>(mutexHandle_));
        mutexHandle_ = nullptr;
    }
}

void SingleInstance::ListenLoop() {
    while (!stop_.load()) {
        HANDLE pipe = CreateNamedPipeW(
            kPipeName,
            PIPE_ACCESS_INBOUND,
            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
            1,
            kPipeBufferSize,
            kPipeBufferSize,
            0,
            nullptr);
        if (pipe == INVALID_HANDLE_VALUE) {
            MV_LOG_WARNING("SingleInstance: CreateNamedPipeW failed");
            return;
        }

        BOOL connected = ConnectNamedPipe(pipe, nullptr);
        if (!connected && GetLastError() != ERROR_PIPE_CONNECTED) {
            CloseHandle(pipe);
            continue;
        }
        if (stop_.load()) {
            DisconnectNamedPipe(pipe);
            CloseHandle(pipe);
            return;
        }

        std::array<char, kPipeBufferSize> buffer{};
        DWORD read = 0;
        std::string payload;
        while (ReadFile(pipe, buffer.data(), kPipeBufferSize, &read, nullptr) && read > 0) {
            payload.append(buffer.data(), read);
        }
        DisconnectNamedPipe(pipe);
        CloseHandle(pipe);

        if (onRaise_) onRaise_(payload);
    }
}

} // namespace MetaVisage
