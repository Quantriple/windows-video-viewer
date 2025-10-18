#pragma once

#include "VideoFile.h"
#include "FileScanner.h"
#include "VideoManager.h"
#include "../resources/resource.h"
#include <windows.h>
#include <commctrl.h>
#include <vector>
#include <memory>
#include <string>

namespace VideoViewer {

/**
 * @brief 主窗口类
 * 负责创建和管理应用程序的主界面
 */
class MainWindow {
public:
    MainWindow();
    ~MainWindow();

    /**
     * @brief 创建主窗口
     * @param hInstance 应用程序实例句柄
     * @param nCmdShow 显示方式
     * @return 成功返回 true
     */
    bool create(HINSTANCE hInstance, int nCmdShow);

    /**
     * @brief 获取窗口句柄
     */
    HWND getHandle() const { return m_hwnd; }

    /**
     * @brief 消息循环
     * @return 程序退出码
     */
    int run();

private:
    // 窗口过程函数
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

    // 消息处理函数
    LRESULT onCreate();
    LRESULT onDestroy();
    LRESULT onSize(WPARAM wParam, LPARAM lParam);
    LRESULT onCommand(WPARAM wParam, LPARAM lParam);
    LRESULT onNotify(WPARAM wParam, LPARAM lParam);
    LRESULT onContextMenu(WPARAM wParam, LPARAM lParam);

    // UI 创建函数
    bool createControls();
    void createToolbar();
    void createListView();
    void createStatusBar();
    void createMenus();

    // 事件处理函数
    void onScanDirectory();
    void onRefresh();
    void onViewModeChange();
    void onSearch();
    void onSettings();
    void onAbout();
    void onExit();

    // ListView 事件处理
    LRESULT onListViewDoubleClick(LPNMITEMACTIVATE pnmia);
    LRESULT onListViewRightClick(LPNMITEMACTIVATE pnmia);
    LRESULT onListViewKeyDown(LPNMLVKEYDOWN pnkd);

    // 文件操作
    void playSelectedFile(int index);
    void deleteSelectedFile(int index);
    void showFileProperties(int index);
    void copyPathToClipboard(int index);
    void showInExplorer(int index);

    // UI 更新函数
    void updateFileList();
    void updateStatusBar(const std::string& text1, const std::string& text2, const std::string& text3);
    void resizeControls();
    void setStatus(const std::string& status);
    void showContextMenu(POINT pt);

    // 扫描回调函数
    void onScanProgress(int current, int total);
    void onScanCompleted(const std::vector<VideoFile>& files);

    // 视图模式和对话框
    void setViewMode(ViewMode mode);
    void showSearchDialog();
    void showSettingsDialog();
    void showAboutDialog();
    
    // 搜索对话框相关函数
    static INT_PTR CALLBACK SearchDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    void performSearch(const std::string& searchText, HWND hResultsList);
    void clearSearchResults(HWND hResultsList);
    
    // 设置对话框相关函数
    static INT_PTR CALLBACK SettingsDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    void loadSettings(HWND hDlg);
    void saveSettings(HWND hDlg);
    void addScanPath(HWND hDlg);
    void removeScanPath(HWND hDlg);
    void browseFolderForSettings(HWND hDlg, int editControlId);

    // 扫描和刷新
    void startDirectoryScan();
    void refreshFileList();

    // 工具函数
    int getSelectedItemIndex();
    VideoFile* getSelectedFile();
    std::string formatFileInfo(const VideoFile& file);

    // 窗口和控件句柄
    HWND m_hwnd;                    // 主窗口句柄
    HWND m_hToolbar;                // 工具栏句柄
    HWND m_hListView;               // 列表视图句柄
    HWND m_hStatusBar;              // 状态栏句柄
    HMENU m_hMenu;                  // 菜单句柄
    HMENU m_hContextMenu;           // 右键菜单句柄
    HINSTANCE m_hInstance;          // 应用程序实例句柄

    // 应用程序组件
    std::unique_ptr<FileScanner> m_fileScanner;     // 文件扫描器
    std::unique_ptr<VideoManager> m_videoManager;   // 视频管理器
    std::vector<VideoFile> m_videoFiles;            // 视频文件列表
    std::vector<std::filesystem::path> m_scanPaths; // 扫描路径列表
    AppSettings m_settings;                         // 应用程序设置

    // 窗口状态
    ViewMode m_viewMode;            // 当前视图模式
    bool m_isScanning;              // 是否正在扫描
    std::string m_currentStatus;    // 当前状态文本

    // 常量定义
    static constexpr int TOOLBAR_HEIGHT = 32;
    static constexpr int STATUSBAR_HEIGHT = 24;
    static constexpr int MIN_WINDOW_WIDTH = 800;
    static constexpr int MIN_WINDOW_HEIGHT = 600;

    // 控件 ID 定义
    enum ControlIDs {
        ID_TOOLBAR = 5000,
        ID_LISTVIEW,
        ID_STATUSBAR,
        
        // 右键菜单 ID
        ID_CONTEXT_PLAY = 6000,
        ID_CONTEXT_DELETE,
        ID_CONTEXT_PROPERTIES,
        ID_CONTEXT_COPY_PATH,
        ID_CONTEXT_SHOW_IN_EXPLORER
    };
};

} // namespace VideoViewer