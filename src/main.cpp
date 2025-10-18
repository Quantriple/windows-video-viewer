#include "MainWindow.h"
#include "Utils.h"
#include <windows.h>
#include <objbase.h>
#include <memory>

using namespace VideoViewer;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // 分配控制台用于调试
    AllocConsole();
    
    // 设置控制台编码为UTF-8
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    
    freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
    freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);
    
    // 设置控制台字体以支持中文显示
    CONSOLE_FONT_INFOEX cfi;
    cfi.cbSize = sizeof(cfi);
    cfi.nFont = 0;
    cfi.dwFontSize.X = 0;
    cfi.dwFontSize.Y = 16;
    cfi.FontFamily = FF_DONTCARE;
    cfi.FontWeight = FW_NORMAL;
    wcscpy_s(cfi.FaceName, L"Consolas");
    SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &cfi);
    
    printf("VideoViewer 启动中...\n");
    
    try {
        // 初始化 COM
        printf("初始化 COM...\n");
        HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        if (FAILED(hr)) {
            printf("COM 初始化失败: 0x%08X\n", hr);
            MessageBoxA(nullptr, "COM 初始化失败", "错误", MB_OK | MB_ICONERROR);
            return -1;
        }
        printf("COM 初始化成功\n");
        
        // 创建主窗口
        printf("创建主窗口...\n");
        VideoViewer::MainWindow mainWindow;
        if (!mainWindow.create(hInstance, nCmdShow)) {
            printf("窗口创建失败\n");
            MessageBoxA(nullptr, "窗口创建失败", "错误", MB_OK | MB_ICONERROR);
            CoUninitialize();
            return -1;
        }
        printf("主窗口创建成功\n");
        
        // 运行消息循环
        printf("开始消息循环...\n");
        int result = mainWindow.run();
        printf("消息循环结束，返回值: %d\n", result);
        
        // 清理 COM
        CoUninitialize();
        
        return result;
    }
    catch (const std::exception& e) {
        printf("程序异常: %s\n", e.what());
        std::string errorMsg = "程序异常: ";
        errorMsg += e.what();
        MessageBoxA(nullptr, errorMsg.c_str(), "错误", MB_OK | MB_ICONERROR);
        CoUninitialize();
        return -1;
    }
    catch (...) {
        printf("未知异常\n");
        MessageBoxA(nullptr, "未知异常", "错误", MB_OK | MB_ICONERROR);
        CoUninitialize();
        return -1;
    }
}