#include "MainWindow.h"
#include "Utils.h"
#include "resource.h"
#include <commctrl.h>
#include <commdlg.h>
#include <shlobj.h>
#include <algorithm>
#include <sstream>

#pragma comment(lib, "comctl32.lib")

namespace VideoViewer {

MainWindow::MainWindow() 
    : m_hwnd(nullptr)
    , m_hToolbar(nullptr)
    , m_hListView(nullptr)
    , m_hStatusBar(nullptr)
    , m_fileScanner(std::make_unique<FileScanner>())
    , m_videoManager(std::make_unique<VideoManager>())
    , m_viewMode(ViewMode::List)
    , m_isScanning(false) {
    
    // 初始化设置
    m_settings.scanPaths.push_back(Utils::GetDocumentsDirectory() / "Videos");
    m_settings.playerPath = "";
    m_settings.cachePath = Utils::GetLocalAppDataDirectory() / "VideoViewer" / "Cache";
    m_settings.sortColumn = "name";
    m_settings.sortAscending = true;
}

MainWindow::~MainWindow() {
    if (m_fileScanner && m_fileScanner->isScanning()) {
        m_fileScanner->cancelScan();
    }
}

bool MainWindow::create(HINSTANCE hInstance, int nCmdShow) {
    printf("MainWindow::create 开始\n");
    printf("hInstance: %p\n", hInstance);
    m_hInstance = hInstance;
    
    // 注册窗口类
    printf("注册窗口类...\n");
    WNDCLASSW wc = {};
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = nullptr;
    wc.lpszClassName = L"VideoViewerMainWindow";
    
    ATOM classAtom = RegisterClassW(&wc);
    if (!classAtom) {
        DWORD error = GetLastError();
        printf("注册窗口类失败，错误代码: %lu\n", error);
        if (error != ERROR_CLASS_ALREADY_EXISTS) {
            MessageBoxA(nullptr, "注册窗口类失败", "错误", MB_OK | MB_ICONERROR);
            return false;
        } else {
            printf("窗口类已存在，继续创建窗口\n");
        }
    } else {
        printf("窗口类注册成功，ATOM: %u\n", classAtom);
    }
    
    // 创建主窗口
    printf("创建窗口...\n");
    m_hwnd = CreateWindowW(
        L"VideoViewerMainWindow",
        L"Windows 视频快速阅览器",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1200, 800,
        nullptr,
        nullptr,
        hInstance,
        this
    );
    
    if (!m_hwnd) {
        DWORD error = GetLastError();
        printf("创建主窗口失败，错误代码: %lu\n", error);
        char errorMsg[256];
        sprintf_s(errorMsg, "创建主窗口失败，错误代码: %lu", error);
        MessageBoxA(nullptr, errorMsg, "错误", MB_OK | MB_ICONERROR);
        return false;
    }
    
    printf("主窗口创建成功，句柄: %p\n", m_hwnd);
    
    printf("显示窗口...\n");
    ShowWindow(m_hwnd, nCmdShow);
    UpdateWindow(m_hwnd);
    printf("窗口显示完成\n");
    
    return true;
}

LRESULT CALLBACK MainWindow::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    MainWindow* pThis = nullptr;
    
    if (message == WM_NCCREATE) {
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        pThis = reinterpret_cast<MainWindow*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        if (pThis) {
            pThis->m_hwnd = hwnd;
        }
    } else {
        pThis = reinterpret_cast<MainWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }
    
    if (pThis) {
        return pThis->handleMessage(message, wParam, lParam);
    }
    
    return DefWindowProc(hwnd, message, wParam, lParam);
}

LRESULT MainWindow::handleMessage(UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE:
            return onCreate();
            
        case WM_SIZE:
            return onSize(wParam, lParam);
            
        case WM_COMMAND:
            return onCommand(wParam, lParam);
            
        case WM_NOTIFY:
            return onNotify(wParam, lParam);
            
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
            
        default:
            return DefWindowProc(m_hwnd, message, wParam, lParam);
    }
}

LRESULT MainWindow::onCreate() {
    printf("onCreate 开始\n");
    
    // 初始化通用控件
    printf("初始化通用控件...\n");
    INITCOMMONCONTROLSEX icex = {};
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES;
    if (!InitCommonControlsEx(&icex)) {
        printf("初始化通用控件失败\n");
    } else {
        printf("通用控件初始化成功\n");
    }
    
    // 创建工具栏
    printf("创建工具栏...\n");
    createToolbar();
    
    // 创建列表视图
    printf("创建列表视图...\n");
    createListView();
    
    // 创建状态栏
    printf("创建状态栏...\n");
    createStatusBar();
    
    // 创建菜单
    printf("创建菜单...\n");
    createMenus();
    
    // 设置初始状态
    printf("设置初始状态...\n");
    updateStatusBar("就绪 - 点击扫描按钮开始", "", "");
    
    printf("onCreate 完成\n");
    return 0;
}

LRESULT MainWindow::onSize(WPARAM wParam, LPARAM lParam) {
    int width = LOWORD(lParam);
    int height = HIWORD(lParam);
    // 调整工具栏大小
    if (m_hToolbar) {
        SendMessage(m_hToolbar, TB_AUTOSIZE, 0, 0);
    }
    
    // 调整状态栏大小
    if (m_hStatusBar) {
        SendMessage(m_hStatusBar, WM_SIZE, 0, 0);
    }
    
    // 计算列表视图的位置和大小
    if (m_hListView) {
        RECT toolbarRect = {};
        RECT statusRect = {};
        
        if (m_hToolbar) {
            GetWindowRect(m_hToolbar, &toolbarRect);
        }
        
        if (m_hStatusBar) {
            GetWindowRect(m_hStatusBar, &statusRect);
        }
        
        int toolbarHeight = toolbarRect.bottom - toolbarRect.top;
        int statusHeight = statusRect.bottom - statusRect.top;
        
        SetWindowPos(m_hListView, nullptr,
                    0, toolbarHeight,
                    width, height - toolbarHeight - statusHeight,
                    SWP_NOZORDER);
    }
    
    return 0;
}

LRESULT MainWindow::onCommand(WPARAM wParam, LPARAM lParam) {
    WORD id = LOWORD(wParam);
    WORD notifyCode = HIWORD(wParam);
    HWND hwndCtl = reinterpret_cast<HWND>(lParam);
    
    printf("=== onCommand 被调用 ===\n");
    printf("命令ID: %d (0x%04X)\n", id, id);
    printf("通知代码: %d\n", notifyCode);
    printf("控件句柄: %p\n", hwndCtl);
    
    switch (id) {
        case ID_FILE_SCAN:
            printf("检测到扫描按钮点击 (ID_FILE_SCAN = %d)\n", ID_FILE_SCAN);
            startDirectoryScan();
            break;
            
        case ID_FILE_REFRESH:
            refreshFileList();
            break;
            
        case ID_VIEW_LIST:
            setViewMode(ViewMode::List);
            break;
            
        case ID_VIEW_DETAILS:
            setViewMode(ViewMode::Details);
            break;
            
        case ID_TOOLS_SEARCH:
            showSearchDialog();
            break;
            
        case ID_TOOLS_SETTINGS:
            showSettingsDialog();
            break;
            
        case ID_HELP_ABOUT:
            showAboutDialog();
            break;
            
        case ID_FILE_EXIT:
            PostMessage(m_hwnd, WM_CLOSE, 0, 0);
            break;
            
        default:
            return DefWindowProc(m_hwnd, WM_COMMAND, MAKEWPARAM(id, notifyCode), reinterpret_cast<LPARAM>(hwndCtl));
    }
    
    return 0;
}

LRESULT MainWindow::onNotify(WPARAM wParam, LPARAM lParam) {
    LPNMHDR pnmh = reinterpret_cast<LPNMHDR>(lParam);
    if (pnmh->hwndFrom == m_hListView) {
        switch (pnmh->code) {
            case NM_DBLCLK:
                return onListViewDoubleClick(reinterpret_cast<LPNMITEMACTIVATE>(pnmh));
                
            case NM_RCLICK:
                return onListViewRightClick(reinterpret_cast<LPNMITEMACTIVATE>(pnmh));
                
            case LVN_KEYDOWN:
                return onListViewKeyDown(reinterpret_cast<LPNMLVKEYDOWN>(pnmh));
        }
    }
    
    return 0;
}

void MainWindow::createToolbar() {
    m_hToolbar = CreateWindowEx(0, TOOLBARCLASSNAME, nullptr,
                               WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS,
                               0, 0, 0, 0,
                               m_hwnd, nullptr, m_hInstance, nullptr);
    
    if (m_hToolbar) {
        SendMessage(m_hToolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
        
        // 添加工具栏按钮（不使用图标，只使用文本）
        TBBUTTON buttons[] = {
            {I_IMAGENONE, ID_FILE_SCAN, TBSTATE_ENABLED, BTNS_AUTOSIZE | BTNS_SHOWTEXT, {0}, 0, 0},
            {I_IMAGENONE, ID_FILE_REFRESH, TBSTATE_ENABLED, BTNS_AUTOSIZE | BTNS_SHOWTEXT, {0}, 0, 1},
            {0, 0, 0, BTNS_SEP, {0}, 0, 0},
            {I_IMAGENONE, ID_VIEW_LIST, TBSTATE_ENABLED, BTNS_AUTOSIZE | BTNS_SHOWTEXT, {0}, 0, 2},
            {I_IMAGENONE, ID_VIEW_DETAILS, TBSTATE_ENABLED, BTNS_AUTOSIZE | BTNS_SHOWTEXT, {0}, 0, 3},
            {0, 0, 0, BTNS_SEP, {0}, 0, 0},
            {I_IMAGENONE, ID_TOOLS_SEARCH, TBSTATE_ENABLED, BTNS_AUTOSIZE | BTNS_SHOWTEXT, {0}, 0, 4},
            {I_IMAGENONE, ID_TOOLS_SETTINGS, TBSTATE_ENABLED, BTNS_AUTOSIZE | BTNS_SHOWTEXT, {0}, 0, 5}
        };
        
        // 添加字符串
        SendMessage(m_hToolbar, TB_ADDSTRINGW, 0, reinterpret_cast<LPARAM>(L"扫描\0刷新\0列表\0详细信息\0搜索\0设置\0"));
        
        SendMessage(m_hToolbar, TB_ADDBUTTONS, sizeof(buttons) / sizeof(TBBUTTON), reinterpret_cast<LPARAM>(buttons));
        SendMessage(m_hToolbar, TB_AUTOSIZE, 0, 0);
    }
}

void MainWindow::createListView() {
    m_hListView = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, nullptr,
                                WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL,
                                0, 0, 0, 0,
                                m_hwnd, nullptr, m_hInstance, nullptr);
    
    if (m_hListView) {
        // 设置扩展样式
        ListView_SetExtendedListViewStyle(m_hListView, 
            LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
        
        // 添加列
        LVCOLUMN column = {};
        column.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        
        column.pszText = const_cast<LPWSTR>(L"文件名");
        column.cx = 300;
        column.iSubItem = 0;
        ListView_InsertColumn(m_hListView, 0, &column);
        
        column.pszText = const_cast<LPWSTR>(L"大小");
        column.cx = 100;
        column.iSubItem = 1;
        ListView_InsertColumn(m_hListView, 1, &column);
        
        column.pszText = const_cast<LPWSTR>(L"修改时间");
        column.cx = 150;
        column.iSubItem = 2;
        ListView_InsertColumn(m_hListView, 2, &column);
        
        column.pszText = const_cast<LPWSTR>(L"类型");
        column.cx = 80;
        column.iSubItem = 3;
        ListView_InsertColumn(m_hListView, 3, &column);
        
        column.pszText = const_cast<LPWSTR>(L"路径");
        column.cx = 400;
        column.iSubItem = 4;
        ListView_InsertColumn(m_hListView, 4, &column);
    }
}

void MainWindow::createStatusBar() {
    m_hStatusBar = CreateWindowEx(0, STATUSCLASSNAME, nullptr,
                                 WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
                                 0, 0, 0, 0,
                                 m_hwnd, nullptr, m_hInstance, nullptr);
    
    if (m_hStatusBar) {
        // 设置状态栏分区
        int parts[] = {200, 400, -1};
        SendMessage(m_hStatusBar, SB_SETPARTS, 3, reinterpret_cast<LPARAM>(parts));
        
        updateStatusBar("就绪", "", "");
    }
}

void MainWindow::createMenus() {
    // 菜单已在资源文件中定义，这里可以添加动态菜单项
}

void MainWindow::startDirectoryScan() {
    printf("=== startDirectoryScan 函数被调用 ===\n");
    
    if (m_isScanning) {
        printf("当前正在扫描中，忽略此次调用\n");
        return;
    }
    
    printf("检查扫描路径数量: %zu\n", m_settings.scanPaths.size());
    
    // 总是弹出文件夹选择对话框让用户选择要扫描的路径
    printf("准备打开文件夹选择对话框\n");
    printf("窗口句柄: %p\n", m_hwnd);
    
    std::filesystem::path selectedPath = Utils::SelectFolderDialog(m_hwnd, "选择要扫描的视频文件夹");
    
    printf("文件夹选择对话框返回，路径: '%s'\n", selectedPath.string().c_str());
    
    if (selectedPath.empty()) {
        printf("用户未选择路径或对话框失败\n");
        updateStatusBar("未选择扫描路径", "", "");
        return;
    }
    
    // 清空现有路径并添加新选择的路径
    m_settings.scanPaths.clear();
    m_settings.scanPaths.push_back(selectedPath);
    printf("已将路径设置为扫描路径: %s\n", selectedPath.string().c_str());
    updateStatusBar("已选择扫描路径: " + selectedPath.string(), "", "");
    
    // 检查扫描路径是否存在
    std::vector<std::filesystem::path> validPaths;
    for (const auto& path : m_settings.scanPaths) {
        if (std::filesystem::exists(path)) {
            validPaths.push_back(path);
        }
    }
    
    if (validPaths.empty()) {
        updateStatusBar("扫描路径不存在", "", "");
        return;
    }
    
    m_isScanning = true;
    updateStatusBar("正在扫描...", "", "");
    
    // 清空当前列表
    ListView_DeleteAllItems(m_hListView);
    m_videoFiles.clear();
    
    // 开始异步扫描
    auto progressCallback = [this](const std::filesystem::path& currentPath, size_t foundCount) {
        onScanProgress(static_cast<int>(foundCount), static_cast<int>(foundCount + 1));
    };
    
    auto completionCallback = [this](const std::vector<VideoFile>& files) {
        onScanCompleted(files);
    };
    
    m_fileScanner->scanDirectoriesAsync(validPaths, progressCallback, completionCallback);
}

void MainWindow::refreshFileList() {
    printf("=== refreshFileList 函数被调用 ===\n");
    
    if (m_settings.scanPaths.empty()) {
        printf("没有扫描路径，调用 startDirectoryScan\n");
        startDirectoryScan();
        return;
    }
    
    printf("刷新当前扫描路径: %zu 个路径\n", m_settings.scanPaths.size());
    
    if (m_isScanning) {
        printf("当前正在扫描中，忽略刷新请求\n");
        return;
    }
    
    m_isScanning = true;
    m_videoFiles.clear();
    
    // 清空列表视图
    ListView_DeleteAllItems(m_hListView);
    
    updateStatusBar("正在刷新文件列表...", "", "");
    
    // 重新扫描所有已配置的路径
    for (const auto& path : m_settings.scanPaths) {
        printf("重新扫描路径: %s\n", path.string().c_str());
        
        if (!std::filesystem::exists(path)) {
            printf("路径不存在: %s\n", path.string().c_str());
            continue;
        }
        
        try {
            auto files = m_fileScanner->scanDirectory(path);
            printf("在路径 %s 中找到 %zu 个视频文件\n", path.string().c_str(), files.size());
            
            m_videoFiles.insert(m_videoFiles.end(), files.begin(), files.end());
        } catch (const std::exception& e) {
            printf("扫描路径时出错 %s: %s\n", path.string().c_str(), e.what());
        }
    }
    
    printf("刷新完成，总共找到 %zu 个视频文件\n", m_videoFiles.size());
    
    // 更新列表视图
    updateFileList();
    
    m_isScanning = false;
    
    std::string statusText = "刷新完成，找到 " + std::to_string(m_videoFiles.size()) + " 个视频文件";
    updateStatusBar(statusText, "", "");
}

void MainWindow::setViewMode(ViewMode mode) {
    m_viewMode = mode;
    
    DWORD style = GetWindowLong(m_hListView, GWL_STYLE);
    style &= ~LVS_TYPEMASK;
    
    switch (mode) {
        case ViewMode::List:
            style |= LVS_LIST;
            break;
        case ViewMode::Details:
            style |= LVS_REPORT;
            break;
        case ViewMode::Thumbnail:
            style |= LVS_ICON;
            break;
    }
    
    SetWindowLong(m_hListView, GWL_STYLE, style);
    InvalidateRect(m_hListView, nullptr, TRUE);
}

void MainWindow::showSearchDialog() {
    printf("=== showSearchDialog 函数被调用 ===\n");
    
    if (m_videoFiles.empty()) {
        Utils::ShowInfoMessage(m_hwnd, "没有可搜索的视频文件，请先扫描目录");
        return;
    }
    
    DialogBoxParam(m_hInstance, MAKEINTRESOURCE(IDD_SEARCH), m_hwnd, SearchDialogProc, reinterpret_cast<LPARAM>(this));
}

void MainWindow::showSettingsDialog() {
    printf("=== showSettingsDialog 函数被调用 ===\n");
    DialogBoxParam(m_hInstance, MAKEINTRESOURCE(IDD_SETTINGS), m_hwnd, SettingsDialogProc, reinterpret_cast<LPARAM>(this));
}

void MainWindow::showAboutDialog() {
    std::wstring aboutText = L"Windows 视频快速阅览器 v1.0\n\n";
    aboutText += L"一个轻量级的视频文件管理工具\n";
    aboutText += L"支持常见视频格式的快速浏览和管理\n\n";
    aboutText += L"开发：VideoViewer Team\n";
    aboutText += L"版权所有 © 2024";
    
    MessageBoxW(m_hwnd, aboutText.c_str(), L"关于", MB_OK | MB_ICONINFORMATION);
}

LRESULT MainWindow::onListViewDoubleClick(LPNMITEMACTIVATE pnmia) {
    if (pnmia->iItem >= 0 && pnmia->iItem < static_cast<int>(m_videoFiles.size())) {
        playSelectedFile(pnmia->iItem);
    }
    return 0;
}

LRESULT MainWindow::onListViewRightClick(LPNMITEMACTIVATE pnmia) {
    if (pnmia->iItem >= 0) {
        showContextMenu(pnmia->ptAction);
    }
    return 0;
}

LRESULT MainWindow::onListViewKeyDown(LPNMLVKEYDOWN pnkd) {
    if (pnkd->wVKey == VK_DELETE) {
        int selectedItem = ListView_GetNextItem(m_hListView, -1, LVNI_SELECTED);
        if (selectedItem >= 0) {
            deleteSelectedFile(selectedItem);
        }
    }
    return 0;
}

void MainWindow::playSelectedFile(int index) {
    if (index < 0 || index >= static_cast<int>(m_videoFiles.size())) {
        return;
    }
    
    const VideoFile& videoFile = m_videoFiles[index];
    
    if (!m_videoManager->openWithDefaultPlayer(videoFile.filePath)) {
        Utils::ShowErrorMessage(m_hwnd, "播放失败: " + m_videoManager->getLastError());
    }
}

void MainWindow::deleteSelectedFile(int index) {
    if (index < 0 || index >= static_cast<int>(m_videoFiles.size())) {
        return;
    }
    
    const VideoFile& videoFile = m_videoFiles[index];
    
    std::wstring message = L"确定要删除文件 \"" + 
                          Utils::StringToWString(videoFile.fileName) + L"\" 吗？";
    
    if (Utils::ShowConfirmDialog(m_hwnd, Utils::WStringToString(message))) {
        if (m_videoManager->moveToRecycleBin(videoFile.filePath)) {
            // 从列表中移除
            ListView_DeleteItem(m_hListView, index);
            m_videoFiles.erase(m_videoFiles.begin() + index);
            
            updateStatusBar("文件已删除", "", "");
        } else {
            Utils::ShowErrorMessage(m_hwnd, "删除失败: " + m_videoManager->getLastError());
        }
    }
}

void MainWindow::showFileProperties(int index) {
    if (index < 0 || index >= static_cast<int>(m_videoFiles.size())) {
        return;
    }
    
    const VideoFile& videoFile = m_videoFiles[index];
    std::string properties = m_videoManager->getFileProperties(videoFile.filePath);
    
    Utils::ShowInfoMessage(m_hwnd, properties);
}

void MainWindow::copyPathToClipboard(int index) {
    if (index < 0 || index >= static_cast<int>(m_videoFiles.size())) {
        return;
    }
    
    const VideoFile& videoFile = m_videoFiles[index];
    
    if (m_videoManager->copyPathToClipboard(videoFile.filePath)) {
        updateStatusBar("路径已复制到剪贴板", "", "");
    } else {
        Utils::ShowErrorMessage(m_hwnd, "复制失败: " + m_videoManager->getLastError());
    }
}

void MainWindow::showInExplorer(int index) {
    if (index < 0 || index >= static_cast<int>(m_videoFiles.size())) {
        return;
    }
    
    const VideoFile& videoFile = m_videoFiles[index];
    
    if (!m_videoManager->showInExplorer(videoFile.filePath)) {
        Utils::ShowErrorMessage(m_hwnd, "打开资源管理器失败: " + m_videoManager->getLastError());
    }
}

void MainWindow::showContextMenu(POINT pt) {
    HMENU hMenu = CreatePopupMenu();
    if (hMenu) {
        AppendMenuW(hMenu, MF_STRING, ID_CONTEXT_PLAY, L"播放");
        AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(hMenu, MF_STRING, ID_CONTEXT_DELETE, L"删除");
        AppendMenuW(hMenu, MF_STRING, ID_CONTEXT_PROPERTIES, L"属性");
        AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(hMenu, MF_STRING, ID_CONTEXT_COPY_PATH, L"复制路径");
        AppendMenuW(hMenu, MF_STRING, ID_CONTEXT_SHOW_IN_EXPLORER, L"在资源管理器中显示");
        
        ClientToScreen(m_hListView, &pt);
        TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, m_hwnd, nullptr);
        DestroyMenu(hMenu);
    }
}

void MainWindow::updateFileList() {
    ListView_DeleteAllItems(m_hListView);
    
    for (size_t i = 0; i < m_videoFiles.size(); ++i) {
        const VideoFile& videoFile = m_videoFiles[i];
        
        LVITEM item = {};
        item.mask = LVIF_TEXT | LVIF_PARAM;
        item.iItem = static_cast<int>(i);
        item.iSubItem = 0;
        item.pszText = const_cast<LPWSTR>(Utils::StringToWString(videoFile.fileName).c_str());
        item.lParam = static_cast<LPARAM>(i);
        
        int itemIndex = ListView_InsertItem(m_hListView, &item);
        
        if (itemIndex >= 0) {
            // 设置子项
            ListView_SetItemText(m_hListView, itemIndex, 1, 
                const_cast<LPWSTR>(Utils::StringToWString(videoFile.getFormattedSize()).c_str()));
            ListView_SetItemText(m_hListView, itemIndex, 2, 
                const_cast<LPWSTR>(Utils::StringToWString(videoFile.getFormattedTime()).c_str()));
            ListView_SetItemText(m_hListView, itemIndex, 3, 
                const_cast<LPWSTR>(Utils::StringToWString(videoFile.extension).c_str()));
            ListView_SetItemText(m_hListView, itemIndex, 4, 
                const_cast<LPWSTR>(videoFile.filePath.parent_path().wstring().c_str()));
        }
    }
}

void MainWindow::updateStatusBar(const std::string& text1, const std::string& text2, const std::string& text3) {
    if (m_hStatusBar) {
        SendMessageA(m_hStatusBar, SB_SETTEXTA, 0, reinterpret_cast<LPARAM>(text1.c_str()));
        SendMessageA(m_hStatusBar, SB_SETTEXTA, 1, reinterpret_cast<LPARAM>(text2.c_str()));
        SendMessageA(m_hStatusBar, SB_SETTEXTA, 2, reinterpret_cast<LPARAM>(text3.c_str()));
    }
}

void MainWindow::onScanProgress(int current, int total) {
    std::ostringstream oss;
    oss << "扫描中... (" << current << "/" << total << ")";
    updateStatusBar(oss.str(), "", "");
}

void MainWindow::onScanCompleted(const std::vector<VideoFile>& files) {
    m_videoFiles = files;
    m_isScanning = false;
    
    updateFileList();
    
    std::ostringstream oss;
    oss << "扫描完成，找到 " << files.size() << " 个视频文件";
    updateStatusBar(oss.str(), "", "");
}

int MainWindow::run() {
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return static_cast<int>(msg.wParam);
}

// 搜索对话框过程函数
INT_PTR CALLBACK MainWindow::SearchDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    static MainWindow* pMainWindow = nullptr;
    
    switch (message) {
        case WM_INITDIALOG:
            pMainWindow = reinterpret_cast<MainWindow*>(lParam);
            SetFocus(GetDlgItem(hDlg, IDC_SEARCH_TEXT));
            return TRUE;
            
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_SEARCH_BUTTON: {
                    if (pMainWindow) {
                        char searchText[256];
                        GetDlgItemTextA(hDlg, IDC_SEARCH_TEXT, searchText, sizeof(searchText));
                        
                        if (strlen(searchText) > 0) {
                            HWND hResultsList = GetDlgItem(hDlg, IDC_SEARCH_RESULTS);
                            pMainWindow->performSearch(std::string(searchText), hResultsList);
                        } else {
                            MessageBoxA(hDlg, "请输入搜索关键词", "提示", MB_OK | MB_ICONINFORMATION);
                        }
                    }
                    return TRUE;
                }
                
                case IDC_CLEAR_SEARCH: {
                    if (pMainWindow) {
                        SetDlgItemTextA(hDlg, IDC_SEARCH_TEXT, "");
                        HWND hResultsList = GetDlgItem(hDlg, IDC_SEARCH_RESULTS);
                        pMainWindow->clearSearchResults(hResultsList);
                        SetFocus(GetDlgItem(hDlg, IDC_SEARCH_TEXT));
                    }
                    return TRUE;
                }
                
                case IDC_SEARCH_RESULTS:
                    if (HIWORD(wParam) == LBN_DBLCLK) {
                        // 双击搜索结果项，播放视频
                        HWND hResultsList = GetDlgItem(hDlg, IDC_SEARCH_RESULTS);
                        int selectedIndex = SendMessage(hResultsList, LB_GETCURSEL, 0, 0);
                        if (selectedIndex != LB_ERR && pMainWindow) {
                            // 获取选中项的数据（视频文件索引）
                            int videoIndex = SendMessage(hResultsList, LB_GETITEMDATA, selectedIndex, 0);
                            if (videoIndex >= 0 && videoIndex < static_cast<int>(pMainWindow->m_videoFiles.size())) {
                                const VideoFile& videoFile = pMainWindow->m_videoFiles[videoIndex];
                                pMainWindow->m_videoManager->openWithDefaultPlayer(videoFile.filePath);
                                EndDialog(hDlg, IDOK);
                            }
                        }
                    }
                    return TRUE;
                
                case IDOK:
                case IDCANCEL:
                    EndDialog(hDlg, LOWORD(wParam));
                    return TRUE;
            }
            break;
            
        case WM_CLOSE:
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
    }
    
    return FALSE;
}

// 执行搜索功能
void MainWindow::performSearch(const std::string& searchText, HWND hResultsList) {
    printf("=== performSearch 函数被调用，搜索关键词: %s ===\n", searchText.c_str());
    
    // 清空之前的搜索结果
    SendMessage(hResultsList, LB_RESETCONTENT, 0, 0);
    
    std::string lowerSearchText = searchText;
    std::transform(lowerSearchText.begin(), lowerSearchText.end(), lowerSearchText.begin(), ::tolower);
    
    int foundCount = 0;
    
    for (size_t i = 0; i < m_videoFiles.size(); ++i) {
        const VideoFile& videoFile = m_videoFiles[i];
        std::string fileName = videoFile.filePath.filename().string();
        std::string lowerFileName = fileName;
        std::transform(lowerFileName.begin(), lowerFileName.end(), lowerFileName.begin(), ::tolower);
        
        // 检查文件名是否包含搜索关键词
        if (lowerFileName.find(lowerSearchText) != std::string::npos) {
            // 添加到搜索结果列表
            std::string displayText = fileName + " (" + videoFile.filePath.parent_path().string() + ")";
            std::wstring wDisplayText = Utils::StringToWString(displayText);
            int itemIndex = SendMessageW(hResultsList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(wDisplayText.c_str()));
            
            // 将视频文件的索引存储为项数据
            SendMessage(hResultsList, LB_SETITEMDATA, itemIndex, i);
            
            foundCount++;
            printf("找到匹配文件: %s\n", fileName.c_str());
        }
    }
    
    printf("搜索完成，找到 %d 个匹配的文件\n", foundCount);
    
    if (foundCount == 0) {
        SendMessageW(hResultsList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"未找到匹配的文件"));
    }
}

// 清空搜索结果
void MainWindow::clearSearchResults(HWND hResultsList) {
    SendMessage(hResultsList, LB_RESETCONTENT, 0, 0);
}

// 设置对话框过程函数
INT_PTR CALLBACK MainWindow::SettingsDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    static MainWindow* pMainWindow = nullptr;
    
    switch (message) {
        case WM_INITDIALOG:
            pMainWindow = reinterpret_cast<MainWindow*>(lParam);
            if (pMainWindow) {
                pMainWindow->loadSettings(hDlg);
            }
            return TRUE;
            
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_ADD_PATH:
                    if (pMainWindow) {
                        pMainWindow->addScanPath(hDlg);
                    }
                    return TRUE;
                    
                case IDC_REMOVE_PATH:
                    if (pMainWindow) {
                        pMainWindow->removeScanPath(hDlg);
                    }
                    return TRUE;
                    
                case IDC_BROWSE_PLAYER:
                    if (pMainWindow) {
                        pMainWindow->browseFolderForSettings(hDlg, IDC_PLAYER_PATH);
                    }
                    return TRUE;
                    
                case IDC_BROWSE_CACHE:
                    if (pMainWindow) {
                        pMainWindow->browseFolderForSettings(hDlg, IDC_CACHE_PATH);
                    }
                    return TRUE;
                    
                case IDOK:
                    if (pMainWindow) {
                        pMainWindow->saveSettings(hDlg);
                    }
                    EndDialog(hDlg, IDOK);
                    return TRUE;
                    
                case IDAPPLY:
                    if (pMainWindow) {
                        pMainWindow->saveSettings(hDlg);
                    }
                    return TRUE;
                    
                case IDCANCEL:
                    EndDialog(hDlg, IDCANCEL);
                    return TRUE;
            }
            break;
            
        case WM_CLOSE:
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
    }
    
    return FALSE;
}

// 加载设置到对话框
void MainWindow::loadSettings(HWND hDlg) {
    printf("=== loadSettings 函数被调用 ===\n");
    
    // 加载扫描路径
    HWND hPathsList = GetDlgItem(hDlg, IDC_SCAN_PATHS);
    SendMessage(hPathsList, LB_RESETCONTENT, 0, 0);
    
    for (const auto& path : m_scanPaths) {
        std::wstring wPath = Utils::StringToWString(path.string());
        SendMessageW(hPathsList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(wPath.c_str()));
    }
    
    // 设置默认值
    CheckDlgButton(hDlg, IDC_USE_DEFAULT_PLAYER, BST_CHECKED);
    CheckDlgButton(hDlg, IDC_SHOW_THUMBNAILS, BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_AUTO_REFRESH, BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_SORT_ASCENDING, BST_CHECKED);
    
    // 初始化排序下拉框
    HWND hSortCombo = GetDlgItem(hDlg, IDC_SORT_BY);
    SendMessageW(hSortCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"文件名"));
    SendMessageW(hSortCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"文件大小"));
    SendMessageW(hSortCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"修改时间"));
    SendMessage(hSortCombo, CB_SETCURSEL, 0, 0); // 默认选择文件名
    
    // 设置默认缓存路径
    std::string defaultCachePath = "C:\\Temp\\VideoViewer";
    SetDlgItemTextA(hDlg, IDC_CACHE_PATH, defaultCachePath.c_str());
}

// 保存设置
void MainWindow::saveSettings(HWND hDlg) {
    printf("=== saveSettings 函数被调用 ===\n");
    
    // 保存扫描路径
    HWND hPathsList = GetDlgItem(hDlg, IDC_SCAN_PATHS);
    int pathCount = SendMessage(hPathsList, LB_GETCOUNT, 0, 0);
    
    m_scanPaths.clear();
    for (int i = 0; i < pathCount; ++i) {
        char pathBuffer[MAX_PATH];
        SendMessageA(hPathsList, LB_GETTEXT, i, reinterpret_cast<LPARAM>(pathBuffer));
        m_scanPaths.push_back(std::filesystem::path(pathBuffer));
    }
    
    // 获取其他设置
    bool useDefaultPlayer = IsDlgButtonChecked(hDlg, IDC_USE_DEFAULT_PLAYER) == BST_CHECKED;
    bool showThumbnails = IsDlgButtonChecked(hDlg, IDC_SHOW_THUMBNAILS) == BST_CHECKED;
    bool autoRefresh = IsDlgButtonChecked(hDlg, IDC_AUTO_REFRESH) == BST_CHECKED;
    bool sortAscending = IsDlgButtonChecked(hDlg, IDC_SORT_ASCENDING) == BST_CHECKED;
    
    char playerPath[MAX_PATH];
    GetDlgItemTextA(hDlg, IDC_PLAYER_PATH, playerPath, sizeof(playerPath));
    
    char cachePath[MAX_PATH];
    GetDlgItemTextA(hDlg, IDC_CACHE_PATH, cachePath, sizeof(cachePath));
    
    HWND hSortCombo = GetDlgItem(hDlg, IDC_SORT_BY);
    int sortIndex = SendMessage(hSortCombo, CB_GETCURSEL, 0, 0);
    
    printf("设置已保存:\n");
    printf("- 扫描路径数量: %zu\n", m_scanPaths.size());
    printf("- 使用默认播放器: %s\n", useDefaultPlayer ? "是" : "否");
    printf("- 显示缩略图: %s\n", showThumbnails ? "是" : "否");
    printf("- 自动刷新: %s\n", autoRefresh ? "是" : "否");
    printf("- 排序方式: %d (%s)\n", sortIndex, sortAscending ? "升序" : "降序");
    printf("- 播放器路径: %s\n", playerPath);
    printf("- 缓存路径: %s\n", cachePath);
    
    Utils::ShowInfoMessage(m_hwnd, "设置已保存");
}

// 添加扫描路径
void MainWindow::addScanPath(HWND hDlg) {
    printf("=== addScanPath 函数被调用 ===\n");
    
    BROWSEINFOA bi = {};
    bi.hwndOwner = hDlg;
    bi.lpszTitle = "选择要扫描的文件夹";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    
    LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
    if (pidl) {
        char folderPath[MAX_PATH];
        if (SHGetPathFromIDListA(pidl, folderPath)) {
            HWND hPathsList = GetDlgItem(hDlg, IDC_SCAN_PATHS);
            
            // 检查路径是否已存在
            int itemCount = SendMessage(hPathsList, LB_GETCOUNT, 0, 0);
            bool pathExists = false;
            
            for (int i = 0; i < itemCount; ++i) {
                char existingPath[MAX_PATH];
                SendMessageA(hPathsList, LB_GETTEXT, i, reinterpret_cast<LPARAM>(existingPath));
                if (strcmp(existingPath, folderPath) == 0) {
                    pathExists = true;
                    break;
                }
            }
            
            if (!pathExists) {
                std::wstring wFolderPath = Utils::StringToWString(folderPath);
                SendMessageW(hPathsList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(wFolderPath.c_str()));
                printf("添加扫描路径: %s\n", folderPath);
            } else {
                Utils::ShowInfoMessage(hDlg, "该路径已存在于扫描列表中");
            }
        }
        
        CoTaskMemFree(pidl);
    }
}

// 删除扫描路径
void MainWindow::removeScanPath(HWND hDlg) {
    printf("=== removeScanPath 函数被调用 ===\n");
    
    HWND hPathsList = GetDlgItem(hDlg, IDC_SCAN_PATHS);
    int selectedIndex = SendMessage(hPathsList, LB_GETCURSEL, 0, 0);
    
    if (selectedIndex != LB_ERR) {
        char pathBuffer[MAX_PATH];
        SendMessageA(hPathsList, LB_GETTEXT, selectedIndex, reinterpret_cast<LPARAM>(pathBuffer));
        SendMessage(hPathsList, LB_DELETESTRING, selectedIndex, 0);
        printf("删除扫描路径: %s\n", pathBuffer);
    } else {
        Utils::ShowInfoMessage(hDlg, "请先选择要删除的路径");
    }
}

// 浏览文件夹（用于设置对话框）
void MainWindow::browseFolderForSettings(HWND hDlg, int editControlId) {
    printf("=== browseFolderForSettings 函数被调用，控件ID: %d ===\n", editControlId);
    
    if (editControlId == IDC_PLAYER_PATH) {
        // 浏览播放器可执行文件
        OPENFILENAMEA ofn = {};
        char fileName[MAX_PATH] = "";
        
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hDlg;
        ofn.lpstrFile = fileName;
        ofn.nMaxFile = sizeof(fileName);
        ofn.lpstrFilter = "可执行文件\0*.exe\0所有文件\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrTitle = "选择视频播放器";
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
        
        if (GetOpenFileNameA(&ofn)) {
            SetDlgItemTextA(hDlg, editControlId, fileName);
            printf("选择播放器: %s\n", fileName);
        }
    } else {
        // 浏览文件夹
        BROWSEINFOA bi = {};
        bi.hwndOwner = hDlg;
        bi.lpszTitle = (editControlId == IDC_CACHE_PATH) ? "选择缓存文件夹" : "选择文件夹";
        bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
        
        LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
        if (pidl) {
            char folderPath[MAX_PATH];
            if (SHGetPathFromIDListA(pidl, folderPath)) {
                SetDlgItemTextA(hDlg, editControlId, folderPath);
                printf("选择文件夹: %s\n", folderPath);
            }
            
            CoTaskMemFree(pidl);
        }
    }
}

} // namespace VideoViewer