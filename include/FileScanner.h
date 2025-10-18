#pragma once

#include "VideoFile.h"
#include <filesystem>
#include <vector>
#include <functional>
#include <atomic>
#include <thread>
#include <mutex>

namespace VideoViewer {

/**
 * @brief 扫描结果结构体
 * 包含视频文件和文件夹信息
 */
struct ScanResult {
    std::vector<VideoFile> videoFiles;    // 视频文件列表
    std::vector<FolderItem> folders;      // 文件夹列表

    ScanResult() = default;
    
    ScanResult(const std::vector<VideoFile>& videos, const std::vector<FolderItem>& dirs)
        : videoFiles(videos), folders(dirs) {}
};

/**
 * @brief 文件扫描器类
 * 负责递归扫描目录，查找视频文件
 */
class FileScanner {
public:
    /**
     * @brief 扫描进度回调函数类型
     * 参数：当前扫描的文件路径，已找到的文件数量
     */
    using ProgressCallback = std::function<void(const std::filesystem::path&, size_t)>;

    /**
     * @brief 扫描完成回调函数类型
     * 参数：扫描结果（视频文件列表）
     */
    using CompletionCallback = std::function<void(const std::vector<VideoFile>&)>;

    /**
     * @brief 扫描完成回调函数类型（包含文件夹信息）
     * 参数：扫描结果（包含视频文件和文件夹）
     */
    using ScanResultCallback = std::function<void(const ScanResult&)>;

    FileScanner();
    ~FileScanner();

    /**
     * @brief 设置支持的视频文件扩展名
     * @param extensions 扩展名列表（如 {".mp4", ".mkv"}）
     */
    void setVideoExtensions(const std::vector<std::string>& extensions);

    /**
     * @brief 获取当前支持的视频文件扩展名
     */
    const std::vector<std::string>& getVideoExtensions() const;

    /**
     * @brief 检查文件是否为视频文件
     * @param filePath 文件路径
     * @return 如果是支持的视频文件返回 true
     */
    bool isVideoFile(const std::filesystem::path& filePath) const;

    /**
     * @brief 同步扫描目录
     * @param path 要扫描的目录路径
     * @param recursive 是否递归扫描子目录
     * @return 找到的视频文件列表
     */
    std::vector<VideoFile> scanDirectory(const std::filesystem::path& path, bool recursive = true);

    /**
     * @brief 异步扫描目录
     * @param path 要扫描的目录路径
     * @param progressCallback 进度回调函数（可选）
     * @param completionCallback 完成回调函数
     * @param recursive 是否递归扫描子目录
     */
    void scanDirectoryAsync(const std::filesystem::path& path,
                           ProgressCallback progressCallback = nullptr,
                           CompletionCallback completionCallback = nullptr,
                           bool recursive = true);

    /**
     * @brief 扫描多个目录
     * @param paths 目录路径列表
     * @param recursive 是否递归扫描子目录
     * @return 找到的视频文件列表
     */
    std::vector<VideoFile> scanDirectories(const std::vector<std::filesystem::path>& paths, bool recursive = true);

    /**
     * @brief 异步扫描多个目录
     * @param paths 目录路径列表
     * @param progressCallback 进度回调函数（可选）
     * @param completionCallback 完成回调函数
     * @param recursive 是否递归扫描子目录
     */
    void scanDirectoriesAsync(const std::vector<std::filesystem::path>& paths,
                             ProgressCallback progressCallback = nullptr,
                             CompletionCallback completionCallback = nullptr,
                             bool recursive = true);

    /**
     * @brief 扫描目录并返回视频文件和文件夹信息
     * @param path 目录路径
     * @param recursive 是否递归扫描子目录
     * @return 扫描结果（包含视频文件和文件夹）
     */
    ScanResult scanDirectoryWithFolders(const std::filesystem::path& path, bool recursive = true);

    /**
     * @brief 异步扫描目录并返回视频文件和文件夹信息
     * @param path 目录路径
     * @param progressCallback 进度回调函数（可选）
     * @param resultCallback 完成回调函数
     * @param recursive 是否递归扫描子目录
     */
    void scanDirectoryWithFoldersAsync(const std::filesystem::path& path,
                                      ProgressCallback progressCallback = nullptr,
                                      ScanResultCallback resultCallback = nullptr,
                                      bool recursive = true);

    /**
     * @brief 取消当前扫描操作
     */
    void cancelScan();

    /**
     * @brief 检查是否正在扫描
     */
    bool isScanning() const;

    /**
     * @brief 获取扫描状态
     */
    ScanStatus getStatus() const;

    /**
     * @brief 获取已找到的文件数量
     */
    size_t getFoundFileCount() const;

    /**
     * @brief 获取当前扫描的路径
     */
    std::filesystem::path getCurrentScanPath() const;

private:
    /**
     * @brief 递归扫描目录的内部实现
     */
    void scanDirectoryRecursive(const std::filesystem::path& path,
                               std::vector<VideoFile>& results,
                               ProgressCallback progressCallback,
                               bool recursive = true);

    /**
     * @brief 异步扫描的工作线程函数
     */
    void scanWorker(const std::vector<std::filesystem::path>& paths,
                   ProgressCallback progressCallback,
                   CompletionCallback completionCallback,
                   bool recursive = true);

    /**
     * @brief 扫描目录并收集视频文件和文件夹信息
     * @param path 目录路径
     * @param result 扫描结果
     * @param progressCallback 进度回调函数
     * @param recursive 是否递归扫描子目录
     */
    void scanDirectoryWithFoldersRecursive(const std::filesystem::path& path,
                                          ScanResult& result,
                                          ProgressCallback progressCallback,
                                          bool recursive = true);

    /**
     * @brief 异步扫描工作线程（支持文件夹信息）
     * @param path 目录路径
     * @param progressCallback 进度回调函数
     * @param resultCallback 完成回调函数
     * @param recursive 是否递归扫描子目录
     */
    void scanWithFoldersWorker(const std::filesystem::path& path,
                              ProgressCallback progressCallback,
                              ScanResultCallback resultCallback,
                              bool recursive = true);

    std::vector<std::string> m_videoExtensions;    // 支持的视频扩展名
    std::atomic<bool> m_cancelRequested;           // 取消标志
    std::atomic<bool> m_isScanning;                // 扫描状态
    std::atomic<ScanStatus> m_status;              // 扫描状态
    std::atomic<size_t> m_foundFileCount;          // 已找到文件数量
    std::filesystem::path m_currentScanPath;       // 当前扫描路径
    mutable std::mutex m_pathMutex;                // 路径访问互斥锁
    std::unique_ptr<std::thread> m_scanThread;     // 扫描线程
};

} // namespace VideoViewer