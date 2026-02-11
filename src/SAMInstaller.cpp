/**
 * SAMInstaller.cpp
 *
 * Downloads and installs SAM 3 (Segment Anything Model 3) for ComfyUI
 *
 * License: GPL3
 */

#if defined(WIN32) && !defined(__linux__)
#define WINDOWS
#elif defined(__linux__) && !defined(WIN32)
#define POSIX
#endif

#include "SAMInstaller.h"

#include <filesystem>
#include <iostream>
#include <fstream>
#include <chrono>

#ifdef WINDOWS
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#else
#include <curl/curl.h>
#endif

namespace fs = std::filesystem;

// ============================================================================
// Constructor / Destructor
// ============================================================================

SAMInstaller::SAMInstaller() {
}

SAMInstaller::~SAMInstaller() {
    cancelInstallation();
    if (installThread && installThread->joinable()) {
        installThread->join();
    }
}

// ============================================================================
// Public Methods
// ============================================================================

bool SAMInstaller::installAll(const SAMInstallConfig& config) {
    if (installing.load()) {
        std::string errMsg = "Installation already in progress";
        setError(errMsg);
        std::lock_guard<std::mutex> lock(progressMutex);
        progress.state = SAMInstallProgress::State::FAILED;
        progress.errorMessage = errMsg;
        progress.status = "FAILED: " + errMsg;
        if (progressCallback) progressCallback(progress);
        printf("[SAMInstaller] %s\n", errMsg.c_str());
        return false;
    }

    if (config.installDir.empty()) {
        std::string errMsg = "Installation directory not specified";
        setError(errMsg);
        std::lock_guard<std::mutex> lock(progressMutex);
        progress.state = SAMInstallProgress::State::FAILED;
        progress.errorMessage = errMsg;
        progress.status = "FAILED: " + errMsg;
        if (progressCallback) progressCallback(progress);
        printf("[SAMInstaller] %s\n", errMsg.c_str());
        return false;
    }

    currentConfig = config;
    shouldCancel.store(false);
    installing.store(true);

    if (installThread && installThread->joinable()) {
        installThread->join();
    }
    installThread = std::make_unique<std::thread>(&SAMInstaller::installAllThread, this, config);

    return true;
}

void SAMInstaller::cancelInstallation() {
    shouldCancel.store(true);
}

SAMInstallProgress SAMInstaller::getProgress() const {
    std::lock_guard<std::mutex> lock(progressMutex);
    return progress;
}

void SAMInstaller::setProgressCallback(std::function<void(const SAMInstallProgress&)> callback) {
    std::lock_guard<std::mutex> lock(progressMutex);
    progressCallback = callback;
}

bool SAMInstaller::isSAMInstalled(const std::string& installDir) {
    // Fully installed = ComfyUI base + node directory + model weights
    return ComfyUIInstaller::isComfyUIInstalled(installDir)
        && isSAMNodeInstalled(installDir)
        && isSAMModelDownloaded(installDir);
}

bool SAMInstaller::isSAMNodeInstalled(const std::string& installDir) {
    // Check if ComfyUI-SAM3 custom node directory exists
    std::string nodePath = installDir + "/ComfyUI_windows_portable/ComfyUI/custom_nodes/ComfyUI-SAM3";
    if (fs::exists(nodePath) && fs::is_directory(nodePath)) {
        return true;
    }

    // Also check without the portable subdirectory (Linux-style install)
    nodePath = installDir + "/ComfyUI/custom_nodes/ComfyUI-SAM3";
    if (fs::exists(nodePath) && fs::is_directory(nodePath)) {
        return true;
    }

    // Direct custom_nodes check (if installDir is already the ComfyUI root)
    nodePath = installDir + "/custom_nodes/ComfyUI-SAM3";
    if (fs::exists(nodePath) && fs::is_directory(nodePath)) {
        return true;
    }

    return false;
}

bool SAMInstaller::isSAMModelDownloaded(const std::string& installDir) {
    // SAM 3 model is stored at models/sam3/sam3.pt relative to ComfyUI root
    // Check all possible ComfyUI directory layouts

    std::vector<std::string> modelPaths = {
        installDir + "/ComfyUI_windows_portable/ComfyUI/models/sam3/sam3.pt",
        installDir + "/ComfyUI/models/sam3/sam3.pt",
        installDir + "/models/sam3/sam3.pt"
    };

    for (const auto& path : modelPaths) {
        if (fs::exists(path)) {
            // Sanity check: model should be at least 1GB (actual ~3.4GB)
            try {
                auto fileSize = fs::file_size(path);
                if (fileSize > 1000000000LL) {
                    return true;
                }
            } catch (...) {
                // file_size can throw; if it does, file exists but we can't check size
                return true;
            }
        }
    }

    return false;
}

int64_t SAMInstaller::getRequiredDiskSpace() {
    return SAM3_MODEL_SIZE;  // ~3.5GB for model (auto-downloaded on first run)
}

std::string SAMInstaller::getLastError() const {
    std::lock_guard<std::mutex> lock(errorMutex);
    return lastError;
}

void SAMInstaller::clearError() {
    std::lock_guard<std::mutex> lock(errorMutex);
    lastError.clear();
}

// ============================================================================
// Installation Thread
// ============================================================================

void SAMInstaller::installAllThread(SAMInstallConfig config) {
    SAMInstallProgress prog;
    prog.state = SAMInstallProgress::State::CHECKING;
    prog.status = "Checking installation...";
    updateProgress(prog);

    // Step 1: Ensure ComfyUI base is installed
    std::string installDir = config.installDir;
    if (!ComfyUIInstaller::isComfyUIInstalled(installDir)) {
        prog.status = "Installing ComfyUI base first...";
        prog.percentComplete = 5.0f;
        updateProgress(prog);

        // Use ComfyUIInstaller to install base
        ComfyUIInstaller baseInstaller;
        InstallConfig baseConfig;
        baseConfig.installDir = installDir;
        baseConfig.tempDir = config.tempDir;
        baseConfig.connectionTimeout = config.connectionTimeout;
        baseConfig.downloadTimeout = config.downloadTimeout;
        baseConfig.installHunyuanVideo = false;
        baseConfig.installFluxSchnell = false;
        baseConfig.installStyleToVideo = false;

        // Forward progress from base installer
        baseInstaller.setProgressCallback([this](const InstallProgress& p) {
            SAMInstallProgress samProg;
            samProg.state = SAMInstallProgress::State::DOWNLOADING;
            samProg.status = "ComfyUI base: " + p.status;
            samProg.percentComplete = p.percentComplete * 0.5f;  // First 50% for base
            samProg.bytesDownloaded = p.bytesDownloaded;
            samProg.bytesTotal = p.bytesTotal;
            updateProgress(samProg);
        });

        if (!baseInstaller.installComfyUIBase(baseConfig)) {
            prog.state = SAMInstallProgress::State::FAILED;
            prog.errorMessage = "Failed to start ComfyUI base install: " + baseInstaller.getLastError();
            prog.status = prog.errorMessage;
            updateProgress(prog);
            installing.store(false);
            return;
        }

        // Wait for base installation to complete
        while (baseInstaller.isInstalling()) {
            if (shouldCancel.load()) {
                baseInstaller.cancelInstallation();
                prog.state = SAMInstallProgress::State::CANCELLED;
                prog.status = "Installation cancelled";
                updateProgress(prog);
                installing.store(false);
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        // Check if base install succeeded
        if (!ComfyUIInstaller::isComfyUIInstalled(installDir)) {
            prog.state = SAMInstallProgress::State::FAILED;
            prog.errorMessage = "ComfyUI base installation failed";
            prog.status = prog.errorMessage;
            updateProgress(prog);
            installing.store(false);
            return;
        }
    }

    if (shouldCancel.load()) {
        prog.state = SAMInstallProgress::State::CANCELLED;
        prog.status = "Installation cancelled";
        updateProgress(prog);
        installing.store(false);
        return;
    }

    // Step 2: Check if SAM3 node is already installed
    if (isSAMInstalled(installDir)) {
        prog.state = SAMInstallProgress::State::COMPLETE;
        prog.status = "SAM 3 already installed";
        prog.percentComplete = 100.0f;
        updateProgress(prog);
        installing.store(false);
        return;
    }

    // Step 3: Clone ComfyUI-SAM3 custom node
    prog.state = SAMInstallProgress::State::INSTALLING_NODES;
    prog.status = "Cloning ComfyUI-SAM3 custom node...";
    prog.percentComplete = 5.0f;
    updateProgress(prog);

    // Find the custom_nodes directory
    std::string nodesDir;
    std::string comfyDir;
    std::string portablePath = installDir + "/ComfyUI_windows_portable/ComfyUI";
    if (fs::exists(portablePath)) {
        nodesDir = portablePath + "/custom_nodes";
        comfyDir = portablePath;
    } else if (fs::exists(installDir + "/ComfyUI")) {
        nodesDir = installDir + "/ComfyUI/custom_nodes";
        comfyDir = installDir + "/ComfyUI";
    } else {
        nodesDir = installDir + "/custom_nodes";
        comfyDir = installDir;
    }

    createDirectories(nodesDir);

    std::string targetDir = nodesDir + "/ComfyUI-SAM3";
    if (!cloneRepository(NODE_COMFYUI_SAM3, targetDir)) {
        prog.state = SAMInstallProgress::State::FAILED;
        prog.errorMessage = "Failed to clone ComfyUI-SAM3: " + getLastError();
        prog.status = prog.errorMessage;
        updateProgress(prog);
        installing.store(false);
        return;
    }

    // Step 3b: Patch SAM3 node to print detection count
    prog.status = "Patching SAM3 node for detection reporting...";
    prog.percentComplete = 8.0f;
    updateProgress(prog);
    patchSAM3Node(targetDir);

    if (shouldCancel.load()) {
        prog.state = SAMInstallProgress::State::CANCELLED;
        prog.status = "Installation cancelled";
        updateProgress(prog);
        installing.store(false);
        return;
    }

    // Step 4: Install Python dependencies (requirements.txt)
    prog.status = "Installing SAM 3 Python dependencies...";
    prog.percentComplete = 15.0f;
    updateProgress(prog);

    installRequirements(targetDir, comfyDir);

    // Step 4b: Run install.py if it exists
    prog.status = "Running SAM 3 install script...";
    prog.percentComplete = 20.0f;
    updateProgress(prog);

    runInstallScript(targetDir, comfyDir);

    if (shouldCancel.load()) {
        prog.state = SAMInstallProgress::State::CANCELLED;
        prog.status = "Installation cancelled";
        updateProgress(prog);
        installing.store(false);
        return;
    }

    // Step 5: Download SAM 3 model (~3.5GB)
    if (!isSAMModelDownloaded(installDir)) {
        prog.state = SAMInstallProgress::State::DOWNLOADING;
        prog.status = "Downloading SAM 3 model (~3.5GB)...";
        prog.percentComplete = 25.0f;
        updateProgress(prog);

        // Determine model destination path
        std::string modelDir = comfyDir + "/models/sam3";
        createDirectories(modelDir);
        std::string modelPath = modelDir + "/" + SAM3_MODEL_FILENAME;

        if (!downloadModel(SAM3_MODEL_URL, modelPath)) {
            prog.state = SAMInstallProgress::State::FAILED;
            prog.errorMessage = "Failed to download SAM 3 model: " + getLastError();
            prog.status = prog.errorMessage;
            updateProgress(prog);
            installing.store(false);
            return;
        }
    }

    // Step 6: Verify installation
    prog.state = SAMInstallProgress::State::VERIFYING;
    prog.status = "Verifying installation...";
    prog.percentComplete = 98.0f;
    updateProgress(prog);

    if (isSAMInstalled(installDir)) {
        prog.state = SAMInstallProgress::State::COMPLETE;
        prog.status = "SAM 3 installed successfully.";
        prog.percentComplete = 100.0f;
        updateProgress(prog);
    } else {
        prog.state = SAMInstallProgress::State::FAILED;
        prog.errorMessage = "Installation verification failed";
        prog.status = prog.errorMessage;
        updateProgress(prog);
    }

    installing.store(false);
}

// ============================================================================
// Git Operations
// ============================================================================

std::string SAMInstaller::findGitExecutable() {
#ifdef WINDOWS
    // Check common Git installation paths
    std::vector<std::string> gitPaths = {
        "C:/Program Files/Git/cmd/git.exe",
        "C:/Program Files (x86)/Git/cmd/git.exe",
        "C:/msys64/usr/bin/git.exe",
        "git"  // Rely on PATH
    };

    for (const auto& path : gitPaths) {
        if (path == "git" || fs::exists(path)) {
            return path;
        }
    }
    return "git";
#else
    return "git";
#endif
}

bool SAMInstaller::cloneRepository(const std::string& url, const std::string& targetDir) {
    // Check if already cloned
    if (fs::exists(fs::path(targetDir) / ".git")) {
        return true;
    }

    // Create parent directory
    fs::path target(targetDir);
    if (target.has_parent_path()) {
        createDirectories(target.parent_path().string());
    }

    std::string gitExe = findGitExecutable();

#ifdef WINDOWS
    std::string cmdStr = "\"" + gitExe + "\" clone --depth 1 " + url + " \"" + targetDir + "\"";

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    ZeroMemory(&pi, sizeof(pi));

    char cmdLine[4096];
    strncpy(cmdLine, cmdStr.c_str(), sizeof(cmdLine) - 1);
    cmdLine[sizeof(cmdLine) - 1] = '\0';

    if (!CreateProcessA(NULL, cmdLine, NULL, NULL, FALSE,
                        CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        setError("Failed to execute git clone: " + cmdStr);
        return false;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    if (exitCode != 0) {
        setError("git clone failed with exit code " + std::to_string(exitCode));
    }
    return exitCode == 0;
#else
    std::string command = "\"" + gitExe + "\" clone --depth 1 " + url + " \"" + targetDir + "\"";
    int result = system(command.c_str());
    if (result != 0) {
        setError("git clone failed with exit code " + std::to_string(result));
    }
    return result == 0;
#endif
}

// ============================================================================
// Install Script
// ============================================================================

bool SAMInstaller::runInstallScript(const std::string& nodeDir, const std::string& comfyDir) {
    std::string installScript = nodeDir + "/install.py";
    if (!fs::exists(installScript)) {
        // No install script - that's OK, some nodes don't need one
        return true;
    }

#ifdef WINDOWS
    // Find ComfyUI embedded Python
    std::string pythonExe;
    std::string portableParent = fs::path(comfyDir).parent_path().string();
    std::string embeddedPython = portableParent + "/python_embeded/python.exe";

    if (fs::exists(embeddedPython)) {
        pythonExe = embeddedPython;
    } else {
        pythonExe = "python";
    }

    std::string cmdStr = "\"" + pythonExe + "\" \"" + installScript + "\"";

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    ZeroMemory(&pi, sizeof(pi));

    char cmdLine[4096];
    strncpy(cmdLine, cmdStr.c_str(), sizeof(cmdLine) - 1);
    cmdLine[sizeof(cmdLine) - 1] = '\0';

    if (!CreateProcessA(NULL, cmdLine, NULL, NULL, FALSE,
                        CREATE_NO_WINDOW, NULL, nodeDir.c_str(), &si, &pi)) {
        setError("Failed to run install.py");
        return false;
    }

    // Wait up to 5 minutes for install script
    DWORD waitResult = WaitForSingleObject(pi.hProcess, 300000);
    if (waitResult == WAIT_TIMEOUT) {
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        setError("install.py timed out");
        return false;
    }

    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return exitCode == 0;
#else
    std::string command = "cd \"" + nodeDir + "\" && python3 \"" + installScript + "\"";
    int result = system(command.c_str());
    return result == 0;
#endif
}

// ============================================================================
// Requirements Install
// ============================================================================

bool SAMInstaller::installRequirements(const std::string& nodeDir, const std::string& comfyDir) {
    std::string requirementsFile = nodeDir + "/requirements.txt";
    if (!fs::exists(requirementsFile)) {
        return true;
    }

#ifdef WINDOWS
    // Find ComfyUI embedded Python
    std::string pythonExe;
    std::string portableParent = fs::path(comfyDir).parent_path().string();
    std::string embeddedPython = portableParent + "/python_embeded/python.exe";

    if (fs::exists(embeddedPython)) {
        pythonExe = embeddedPython;
    } else {
        pythonExe = "python";
    }

    std::string cmdStr = "\"" + pythonExe + "\" -m pip install -r \"" + requirementsFile + "\"";

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    ZeroMemory(&pi, sizeof(pi));

    char cmdLine[4096];
    strncpy(cmdLine, cmdStr.c_str(), sizeof(cmdLine) - 1);
    cmdLine[sizeof(cmdLine) - 1] = '\0';

    if (!CreateProcessA(NULL, cmdLine, NULL, NULL, FALSE,
                        CREATE_NO_WINDOW, NULL, nodeDir.c_str(), &si, &pi)) {
        setError("Failed to run pip install requirements");
        return false;
    }

    // Wait up to 10 minutes for pip install
    DWORD waitResult = WaitForSingleObject(pi.hProcess, 600000);
    if (waitResult == WAIT_TIMEOUT) {
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        setError("pip install requirements timed out");
        return false;
    }

    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    if (exitCode != 0) {
        setError("pip install requirements failed with exit code " + std::to_string(exitCode));
    }
    return exitCode == 0;
#else
    std::string command = "cd \"" + nodeDir + "\" && python3 -m pip install -r \"" + requirementsFile + "\"";
    int result = system(command.c_str());
    if (result != 0) {
        setError("pip install requirements failed with exit code " + std::to_string(result));
    }
    return result == 0;
#endif
}

// ============================================================================
// Model Download
// ============================================================================

std::string SAMInstaller::formatBytes(int64_t bytes) {
    if (bytes < 1024) return std::to_string(bytes) + " B";
    if (bytes < 1048576) return std::to_string(bytes / 1024) + " KB";
    if (bytes < 1073741824LL) return std::to_string(bytes / 1048576) + " MB";
    char buf[64];
    snprintf(buf, sizeof(buf), "%.1f GB", bytes / 1073741824.0);
    return buf;
}

#ifdef WINDOWS

bool SAMInstaller::downloadModel(const std::string& url, const std::string& destPath) {
    // Parse URL
    std::wstring wurl(url.begin(), url.end());

    URL_COMPONENTS urlComp;
    ZeroMemory(&urlComp, sizeof(urlComp));
    urlComp.dwStructSize = sizeof(urlComp);
    wchar_t hostName[256] = {0};
    wchar_t urlPath[2048] = {0};
    urlComp.lpszHostName = hostName;
    urlComp.dwHostNameLength = sizeof(hostName) / sizeof(wchar_t);
    urlComp.lpszUrlPath = urlPath;
    urlComp.dwUrlPathLength = sizeof(urlPath) / sizeof(wchar_t);

    if (!WinHttpCrackUrl(wurl.c_str(), 0, 0, &urlComp)) {
        setError("Failed to parse URL: " + url);
        return false;
    }

    DWORD flags = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    INTERNET_PORT port = urlComp.nPort;

    // Check for partial download (resume support)
    int64_t existingSize = 0;
    if (fs::exists(destPath)) {
        existingSize = static_cast<int64_t>(fs::file_size(destPath));
        // If already complete, skip
        if (existingSize > 1000000000LL) {
            return true;
        }
    }

    HINTERNET hSession = WinHttpOpen(L"EWOCvj2-SAMInstaller/1.0",
                                     WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                     WINHTTP_NO_PROXY_NAME,
                                     WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) {
        setError("WinHttpOpen failed");
        return false;
    }

    // Set timeouts
    int timeout = currentConfig.connectionTimeout;
    WinHttpSetTimeouts(hSession, timeout, timeout, timeout,
                       currentConfig.downloadTimeout > 0 ? currentConfig.downloadTimeout : 0);

    HINTERNET hConnect = WinHttpConnect(hSession, hostName, port, 0);
    if (!hConnect) {
        setError("WinHttpConnect failed");
        WinHttpCloseHandle(hSession);
        return false;
    }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", urlPath,
                                            NULL, WINHTTP_NO_REFERER,
                                            WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hRequest) {
        setError("WinHttpOpenRequest failed");
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    // Enable automatic redirect following
    DWORD optionValue = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
    WinHttpSetOption(hRequest, WINHTTP_OPTION_REDIRECT_POLICY, &optionValue, sizeof(optionValue));

    // Add Range header for resume
    if (existingSize > 0) {
        std::wstring rangeHeader = L"Range: bytes=" + std::to_wstring(existingSize) + L"-";
        WinHttpAddRequestHeaders(hRequest, rangeHeader.c_str(), -1,
                                 WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);
    }

    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                            WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
        setError("WinHttpSendRequest failed");
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    if (!WinHttpReceiveResponse(hRequest, NULL)) {
        setError("WinHttpReceiveResponse failed");
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    // Check status code
    DWORD statusCode = 0;
    DWORD statusSize = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusSize,
                        WINHTTP_NO_HEADER_INDEX);

    if (statusCode != 200 && statusCode != 206) {
        setError("HTTP error " + std::to_string(statusCode) + " downloading model");
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    // Get content length
    int64_t totalSize = SAM3_MODEL_SIZE;  // fallback estimate
    wchar_t contentLenBuf[64] = {0};
    DWORD contentLenSize = sizeof(contentLenBuf);
    if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CONTENT_LENGTH,
                            WINHTTP_HEADER_NAME_BY_INDEX, contentLenBuf,
                            &contentLenSize, WINHTTP_NO_HEADER_INDEX)) {
        totalSize = _wtoi64(contentLenBuf);
        if (statusCode == 206) {
            totalSize += existingSize;
        }
    }

    // Open output file (append if resuming, otherwise create)
    std::ios_base::openmode mode = std::ios::binary;
    if (existingSize > 0 && statusCode == 206) {
        mode |= std::ios::app;
    } else {
        existingSize = 0;  // Server doesn't support resume, start over
    }
    std::ofstream outFile(destPath, mode);
    if (!outFile.is_open()) {
        setError("Failed to open output file: " + destPath);
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    // Download loop with progress
    char buffer[65536];
    DWORD bytesRead = 0;
    int64_t totalDownloaded = existingSize;
    auto lastProgressUpdate = std::chrono::steady_clock::now();

    while (WinHttpReadData(hRequest, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        if (shouldCancel.load()) {
            outFile.close();
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            setError("Download cancelled");
            return false;
        }

        outFile.write(buffer, bytesRead);
        totalDownloaded += bytesRead;

        // Update progress every 0.5 seconds
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration<float>(now - lastProgressUpdate).count() > 0.5f) {
            SAMInstallProgress prog;
            prog.state = SAMInstallProgress::State::DOWNLOADING;
            prog.bytesDownloaded = totalDownloaded;
            prog.bytesTotal = totalSize;
            if (totalSize > 0) {
                prog.percentComplete = 25.0f + (static_cast<float>(totalDownloaded) / totalSize) * 70.0f;
            }
            prog.status = "Downloading SAM 3 model: " + formatBytes(totalDownloaded) + " / " + formatBytes(totalSize);
            updateProgress(prog);
            lastProgressUpdate = now;
        }

        bytesRead = 0;
    }

    outFile.close();
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    // Verify downloaded size
    if (totalDownloaded < 1000000000LL) {
        setError("Downloaded file too small (" + formatBytes(totalDownloaded) + "), expected ~3.5GB");
        return false;
    }

    return true;
}

#else  // Linux / POSIX

static size_t curlWriteCallback(void* ptr, size_t size, size_t nmemb, void* userdata) {
    std::ofstream* outFile = static_cast<std::ofstream*>(userdata);
    size_t totalBytes = size * nmemb;
    outFile->write(static_cast<char*>(ptr), totalBytes);
    return totalBytes;
}

struct CurlProgressData {
    SAMInstaller* installer;
    int64_t existingSize;
};

static int curlProgressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow,
                                curl_off_t /*ultotal*/, curl_off_t /*ulnow*/) {
    CurlProgressData* data = static_cast<CurlProgressData*>(clientp);
    int64_t totalDownloaded = data->existingSize + dlnow;
    int64_t totalSize = data->existingSize + dltotal;

    if (totalSize > 0) {
        SAMInstallProgress prog;
        prog.state = SAMInstallProgress::State::DOWNLOADING;
        prog.bytesDownloaded = totalDownloaded;
        prog.bytesTotal = totalSize;
        prog.percentComplete = 25.0f + (static_cast<float>(totalDownloaded) / totalSize) * 70.0f;
        prog.status = "Downloading SAM 3 model: " + SAMInstaller::formatBytes(totalDownloaded) +
                      " / " + SAMInstaller::formatBytes(totalSize);
        data->installer->updateProgress(prog);
    }

    return 0;  // 0 = continue, non-zero = abort
}

bool SAMInstaller::downloadModel(const std::string& url, const std::string& destPath) {
    // Check for partial download
    int64_t existingSize = 0;
    if (fs::exists(destPath)) {
        existingSize = static_cast<int64_t>(fs::file_size(destPath));
        if (existingSize > 1000000000LL) {
            return true;  // Already downloaded
        }
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        setError("Failed to initialize curl");
        return false;
    }

    std::ios_base::openmode mode = std::ios::binary;
    if (existingSize > 0) {
        mode |= std::ios::app;
    }
    std::ofstream outFile(destPath, mode);
    if (!outFile.is_open()) {
        setError("Failed to open output file: " + destPath);
        curl_easy_cleanup(curl);
        return false;
    }

    CurlProgressData progressData;
    progressData.installer = this;
    progressData.existingSize = existingSize;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &outFile);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, curlProgressCallback);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &progressData);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

    if (existingSize > 0) {
        curl_easy_setopt(curl, CURLOPT_RESUME_FROM_LARGE, static_cast<curl_off_t>(existingSize));
    }

    if (currentConfig.connectionTimeout > 0) {
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, static_cast<long>(currentConfig.connectionTimeout));
    }

    CURLcode res = curl_easy_perform(curl);
    outFile.close();

    if (res != CURLE_OK) {
        setError("curl download failed: " + std::string(curl_easy_strerror(res)));
        curl_easy_cleanup(curl);
        return false;
    }

    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_easy_cleanup(curl);

    if (httpCode != 200 && httpCode != 206) {
        setError("HTTP error " + std::to_string(httpCode) + " downloading model");
        return false;
    }

    // Verify size
    int64_t finalSize = static_cast<int64_t>(fs::file_size(destPath));
    if (finalSize < 1000000000LL) {
        setError("Downloaded file too small (" + formatBytes(finalSize) + "), expected ~3.5GB");
        return false;
    }

    return true;
}

#endif  // WINDOWS / POSIX

// ============================================================================
// SAM3 Node Patching
// ============================================================================

bool SAMInstaller::patchSAM3Node(const std::string& nodeDir) {
    // Patch inference_reconstructor.py to print detection count after text prompt
    // so our C++ poll loop can detect zero-detection and abort early.
    std::string filePath = nodeDir + "/nodes/inference_reconstructor.py";
    if (!fs::exists(filePath)) {
        printf("[SAMInstaller] inference_reconstructor.py not found, skipping patch\n");
        return false;
    }

    // Read file
    std::ifstream inFile(filePath);
    if (!inFile.is_open()) {
        setError("Failed to open inference_reconstructor.py for patching");
        return false;
    }
    std::string content((std::istreambuf_iterator<char>(inFile)),
                         std::istreambuf_iterator<char>());
    inFile.close();

    // Already patched?
    if (content.find("[EWOCVJ2_SAM3_DETECT]") != std::string::npos) {
        printf("[SAMInstaller] inference_reconstructor.py already patched\n");
        return true;
    }

    // Find the text prompt section in _apply_prompt:
    //     model.add_prompt(
    //         session_id=session_id,
    //         frame_idx=prompt.frame_idx,
    //         obj_id=prompt.obj_id,
    //         text=text,
    //     )
    // Replace with version that captures return value and prints detection count.

    std::string searchStr =
        "model.add_prompt(\n"
        "            session_id=session_id,\n"
        "            frame_idx=prompt.frame_idx,\n"
        "            obj_id=prompt.obj_id,\n"
        "            text=text,\n"
        "        )";

    std::string replaceStr =
        "_result = model.add_prompt(\n"
        "            session_id=session_id,\n"
        "            frame_idx=prompt.frame_idx,\n"
        "            obj_id=prompt.obj_id,\n"
        "            text=text,\n"
        "        )\n"
        "        _n_det = 0\n"
        "        if _result and \"outputs\" in _result:\n"
        "            _outs = _result[\"outputs\"]\n"
        "            if \"obj_ids\" in _outs and _outs[\"obj_ids\"] is not None:\n"
        "                _n_det = len(_outs[\"obj_ids\"])\n"
        "        print(f\"[EWOCVJ2_SAM3_DETECT] count={_n_det}\", flush=True)";

    size_t pos = content.find(searchStr);
    if (pos == std::string::npos) {
        printf("[SAMInstaller] Could not find text prompt pattern in inference_reconstructor.py\n");
        printf("[SAMInstaller] The SAM3 node version may have changed - patch skipped\n");
        return false;
    }

    content.replace(pos, searchStr.length(), replaceStr);

    // Write patched file
    std::ofstream outFile(filePath);
    if (!outFile.is_open()) {
        setError("Failed to write patched inference_reconstructor.py");
        return false;
    }
    outFile << content;
    outFile.close();

    printf("[SAMInstaller] Successfully patched inference_reconstructor.py for detection reporting\n");
    return true;
}

// ============================================================================
// Utility
// ============================================================================

bool SAMInstaller::createDirectories(const std::string& path) {
    try {
        fs::create_directories(path);
        return true;
    } catch (const std::exception& e) {
        setError("Failed to create directory: " + path + " - " + e.what());
        return false;
    }
}

bool SAMInstaller::fileExists(const std::string& path) {
    return fs::exists(path);
}

void SAMInstaller::updateProgress(const SAMInstallProgress& newProgress) {
    std::lock_guard<std::mutex> lock(progressMutex);
    progress = newProgress;
    if (progressCallback) {
        progressCallback(progress);
    }
}

void SAMInstaller::setError(const std::string& error) {
    std::lock_guard<std::mutex> lock(errorMutex);
    lastError = error;
    std::cerr << "[SAMInstaller] Error: " << error << std::endl;
}
