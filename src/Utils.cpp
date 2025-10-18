#include "Utils.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <codecvt>
#include <locale>
#include <ctime>
#include <shlobj.h>
#include <shellapi.h>
#include <commdlg.h>
#include <functional>

namespace VideoViewer {
namespace Utils {

// 支持的视频文件扩展名
const std::vector<std::string> VIDEO_EXTENSIONS = {
    ".mp4", ".mkv", ".avi", ".mov", ".wmv", ".flv", ".webm", 
    ".m4v", ".3gp", ".3g2", ".asf", ".divx", ".f4v", ".m2ts", 
    ".mts", ".ogv", ".rm", ".rmvb", ".vob", ".xvid"
};

std::wstring StringToWString(const std::string& str) {
    if (str.empty()) return std::wstring();
    
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

std::string WStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

std::string FormatFileSize(std::uintmax_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unit < 4) {
        size /= 1024.0;
        unit++;
    }
    
    std::ostringstream oss;
    if (unit == 0) {
        oss << static_cast<std::uintmax_t>(size) << " " << units[unit];
    } else {
        oss << std::fixed << std::setprecision(1) << size << " " << units[unit];
    }
    
    return oss.str();
}

std::string FormatFileTime(const std::filesystem::file_time_type& fileTime) {
    try {
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            fileTime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
        auto time_t = std::chrono::system_clock::to_time_t(sctp);
        
        struct tm* timeinfo = std::localtime(&time_t);
        if (timeinfo == nullptr) {
            return "未知时间";
        }
        
        char buffer[80];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
        return std::string(buffer);
    }
    catch (...) {
        return "未知时间";
    }
}

bool IsVideoFile(const std::filesystem::path& filePath) {
    std::string ext = GetLowerExtension(filePath);
    return std::find(VIDEO_EXTENSIONS.begin(), VIDEO_EXTENSIONS.end(), ext) != VIDEO_EXTENSIONS.end();
}

std::string GetLowerExtension(const std::filesystem::path& filePath) {
    std::string ext = filePath.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}

bool CreateDirectoryIfNotExists(const std::filesystem::path& dirPath) {
    try {
        if (!std::filesystem::exists(dirPath)) {
            return std::filesystem::create_directories(dirPath);
        }
        return true;
    }
    catch (const std::filesystem::filesystem_error&) {
        return false;
    }
}

std::filesystem::path GetAppDirectory() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    std::filesystem::path appPath(path);
    return appPath.parent_path();
}

std::filesystem::path GetDocumentsDirectory() {
    wchar_t* path = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &path))) {
        std::filesystem::path documentsPath(path);
        CoTaskMemFree(path);
        return documentsPath;
    }
    return std::filesystem::path();
}

std::filesystem::path GetLocalAppDataDirectory() {
    wchar_t* path = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &path))) {
        std::filesystem::path localAppDataPath(path);
        CoTaskMemFree(path);
        return localAppDataPath;
    }
    return std::filesystem::path();
}

std::string GetWindowsErrorString(DWORD errorCode) {
    LPSTR messageBuffer = nullptr;
    size_t size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&messageBuffer, 0, NULL);
    
    std::string message(messageBuffer, size);
    LocalFree(messageBuffer);
    
    // 移除末尾的换行符
    while (!message.empty() && (message.back() == '\r' || message.back() == '\n')) {
        message.pop_back();
    }
    
    return message;
}

void ShowErrorMessage(HWND hwnd, const std::string& message, const std::string& title) {
    MessageBoxA(hwnd, message.c_str(), title.c_str(), MB_OK | MB_ICONERROR);
}

void ShowInfoMessage(HWND hwnd, const std::string& message, const std::string& title) {
    MessageBoxA(hwnd, message.c_str(), title.c_str(), MB_OK | MB_ICONINFORMATION);
}

bool ShowConfirmDialog(HWND hwnd, const std::string& message, const std::string& title) {
    int result = MessageBoxA(hwnd, message.c_str(), title.c_str(), MB_YESNO | MB_ICONQUESTION);
    return result == IDYES;
}

std::filesystem::path SelectFolderDialog(HWND hwnd, const std::string& title) {
    printf("=== SelectFolderDialog 开始 ===\n");
    printf("父窗口句柄: %p\n", hwnd);
    printf("对话框标题: %s\n", title.c_str());
    
    // 检查 COM 初始化状态
    HRESULT comCheck = CoInitialize(NULL);
    if (comCheck == S_OK) {
        printf("COM 未初始化，现在初始化\n");
        CoUninitialize();
    } else if (comCheck == S_FALSE) {
        printf("COM 已经初始化\n");
        CoUninitialize();
    } else {
        printf("COM 初始化检查失败: 0x%08X\n", comCheck);
    }
    
    printf("开始创建 IFileOpenDialog 实例\n");
    IFileOpenDialog* pFileOpen = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
                                 IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));
    
    if (FAILED(hr)) {
        printf("SelectFolderDialog: CoCreateInstance 失败, HRESULT = 0x%08X\n", hr);
        return std::filesystem::path();
    }
    
    printf("SelectFolderDialog: IFileOpenDialog 创建成功\n");
    
    // 设置选项为选择文件夹
    DWORD dwOptions;
    hr = pFileOpen->GetOptions(&dwOptions);
    if (SUCCEEDED(hr)) {
        hr = pFileOpen->SetOptions(dwOptions | FOS_PICKFOLDERS);
        if (SUCCEEDED(hr)) {
            printf("SelectFolderDialog: 设置 FOS_PICKFOLDERS 选项成功\n");
        } else {
            printf("SelectFolderDialog: 设置 FOS_PICKFOLDERS 选项失败, HRESULT = 0x%08X\n", hr);
        }
    } else {
        printf("SelectFolderDialog: GetOptions 失败, HRESULT = 0x%08X\n", hr);
    }
    
    // 设置标题
    if (SUCCEEDED(hr)) {
        std::wstring wTitle = StringToWString(title);
        hr = pFileOpen->SetTitle(wTitle.c_str());
        if (SUCCEEDED(hr)) {
            printf("SelectFolderDialog: 设置标题成功: %s\n", title.c_str());
        } else {
            printf("SelectFolderDialog: 设置标题失败, HRESULT = 0x%08X\n", hr);
        }
    }
    
    // 显示对话框
    if (SUCCEEDED(hr)) {
        printf("SelectFolderDialog: 准备显示对话框\n");
        hr = pFileOpen->Show(hwnd);
        if (SUCCEEDED(hr)) {
            printf("SelectFolderDialog: 对话框显示成功，用户已选择\n");
        } else if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
            printf("SelectFolderDialog: 用户取消了对话框\n");
        } else {
            printf("SelectFolderDialog: 显示对话框失败, HRESULT = 0x%08X\n", hr);
        }
    }
    
    // 获取结果
    if (SUCCEEDED(hr)) {
        IShellItem* pItem = nullptr;
        hr = pFileOpen->GetResult(&pItem);
        if (SUCCEEDED(hr)) {
            PWSTR pszFilePath = nullptr;
            hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
            if (SUCCEEDED(hr)) {
                std::filesystem::path result(pszFilePath);
                printf("SelectFolderDialog: 获取选择路径成功: %ls\n", pszFilePath);
                CoTaskMemFree(pszFilePath);
                pItem->Release();
                pFileOpen->Release();
                return result;
            } else {
                printf("SelectFolderDialog: GetDisplayName 失败, HRESULT = 0x%08X\n", hr);
            }
            pItem->Release();
        } else {
            printf("SelectFolderDialog: GetResult 失败, HRESULT = 0x%08X\n", hr);
        }
    }
    
    pFileOpen->Release();
    printf("SelectFolderDialog: 返回空路径\n");
    return std::filesystem::path();
}

std::size_t HashString(const std::string& str) {
    return std::hash<std::string>{}(str);
}

bool MoveToRecycleBin(const std::filesystem::path& filePath) {
    std::wstring wPath = filePath.wstring();
    wPath.push_back(L'\0'); // 双空字符结尾
    
    SHFILEOPSTRUCTW fileOp = {};
    fileOp.wFunc = FO_DELETE;
    fileOp.pFrom = wPath.c_str();
    fileOp.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT;
    
    return SHFileOperationW(&fileOp) == 0;
}

bool OpenWithDefaultProgram(const std::filesystem::path& filePath) {
    std::wstring wPath = filePath.wstring();
    HINSTANCE result = ShellExecuteW(NULL, L"open", wPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
    return reinterpret_cast<intptr_t>(result) > 32;
}

bool OpenWithProgram(const std::filesystem::path& filePath, const std::filesystem::path& programPath) {
    std::wstring wFilePath = filePath.wstring();
    std::wstring wProgramPath = programPath.wstring();
    
    // 构建参数字符串
    std::wstring parameters = L"\"" + wFilePath + L"\"";
    
    HINSTANCE result = ShellExecuteW(NULL, L"open", wProgramPath.c_str(), 
                                    parameters.c_str(), NULL, SW_SHOWNORMAL);
    return reinterpret_cast<intptr_t>(result) > 32;
}

} // namespace Utils
} // namespace VideoViewer