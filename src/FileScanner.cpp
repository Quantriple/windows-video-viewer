#include "FileScanner.h"
#include "Utils.h"
#include <algorithm>

namespace VideoViewer {

FileScanner::FileScanner()
    : m_videoExtensions(Utils::VIDEO_EXTENSIONS)
    , m_cancelRequested(false)
    , m_isScanning(false)
    , m_status(ScanStatus::Idle)
    , m_foundFileCount(0)
{
}

FileScanner::~FileScanner() {
    cancelScan();
    if (m_scanThread && m_scanThread->joinable()) {
        m_scanThread->join();
    }
}

void FileScanner::setVideoExtensions(const std::vector<std::string>& extensions) {
    m_videoExtensions = extensions;
    
    // 确保所有扩展名都是小写
    for (auto& ext : m_videoExtensions) {
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    }
}

const std::vector<std::string>& FileScanner::getVideoExtensions() const {
    return m_videoExtensions;
}

bool FileScanner::isVideoFile(const std::filesystem::path& filePath) const {
    std::string ext = Utils::GetLowerExtension(filePath);
    return std::find(m_videoExtensions.begin(), m_videoExtensions.end(), ext) != m_videoExtensions.end();
}

std::vector<VideoFile> FileScanner::scanDirectory(const std::filesystem::path& path) {
    std::vector<VideoFile> results;
    
    if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path)) {
        return results;
    }
    
    m_status = ScanStatus::Scanning;
    m_foundFileCount = 0;
    m_cancelRequested = false;
    
    try {
        scanDirectoryRecursive(path, results, nullptr);
        m_status = ScanStatus::Completed;
    }
    catch (const std::exception&) {
        m_status = ScanStatus::Error;
    }
    
    return results;
}

void FileScanner::scanDirectoryAsync(const std::filesystem::path& path,
                                    ProgressCallback progressCallback,
                                    CompletionCallback completionCallback) {
    std::vector<std::filesystem::path> paths = { path };
    scanDirectoriesAsync(paths, progressCallback, completionCallback);
}

std::vector<VideoFile> FileScanner::scanDirectories(const std::vector<std::filesystem::path>& paths) {
    std::vector<VideoFile> results;
    
    m_status = ScanStatus::Scanning;
    m_foundFileCount = 0;
    m_cancelRequested = false;
    
    try {
        for (const auto& path : paths) {
            if (m_cancelRequested) break;
            
            if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
                scanDirectoryRecursive(path, results, nullptr);
            }
        }
        m_status = ScanStatus::Completed;
    }
    catch (const std::exception&) {
        m_status = ScanStatus::Error;
    }
    
    return results;
}

void FileScanner::scanDirectoriesAsync(const std::vector<std::filesystem::path>& paths,
                                      ProgressCallback progressCallback,
                                      CompletionCallback completionCallback) {
    // 取消之前的扫描
    cancelScan();
    
    // 启动新的扫描线程
    m_scanThread = std::make_unique<std::thread>(
        &FileScanner::scanWorker, this, paths, progressCallback, completionCallback);
}

void FileScanner::cancelScan() {
    m_cancelRequested = true;
    
    if (m_scanThread && m_scanThread->joinable()) {
        m_scanThread->join();
        m_scanThread.reset();
    }
    
    m_isScanning = false;
    m_status = ScanStatus::Idle;
}

bool FileScanner::isScanning() const {
    return m_isScanning;
}

ScanStatus FileScanner::getStatus() const {
    return m_status;
}

size_t FileScanner::getFoundFileCount() const {
    return m_foundFileCount;
}

std::filesystem::path FileScanner::getCurrentScanPath() const {
    std::lock_guard<std::mutex> lock(m_pathMutex);
    return m_currentScanPath;
}

void FileScanner::scanDirectoryRecursive(const std::filesystem::path& path,
                                        std::vector<VideoFile>& results,
                                        ProgressCallback progressCallback) {
    if (m_cancelRequested) return;
    
    try {
        // 更新当前扫描路径
        {
            std::lock_guard<std::mutex> lock(m_pathMutex);
            m_currentScanPath = path;
        }
        
        // 调用进度回调
        if (progressCallback) {
            progressCallback(path, m_foundFileCount);
        }
        
        // 遍历目录中的所有项目
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            if (m_cancelRequested) break;
            
            try {
                if (entry.is_regular_file()) {
                    // 检查是否为视频文件
                    if (isVideoFile(entry.path())) {
                        VideoFile videoFile(entry.path());
                        results.push_back(videoFile);
                        m_foundFileCount++;
                        
                        // 调用进度回调
                        if (progressCallback) {
                            progressCallback(entry.path(), m_foundFileCount);
                        }
                    }
                }
                else if (entry.is_directory()) {
                    // 递归扫描子目录
                    scanDirectoryRecursive(entry.path(), results, progressCallback);
                }
            }
            catch (const std::filesystem::filesystem_error&) {
                // 忽略单个文件/目录的错误，继续扫描其他项目
                continue;
            }
        }
    }
    catch (const std::filesystem::filesystem_error&) {
        // 忽略目录访问错误
    }
}

void FileScanner::scanWorker(const std::vector<std::filesystem::path>& paths,
                            ProgressCallback progressCallback,
                            CompletionCallback completionCallback) {
    m_isScanning = true;
    m_status = ScanStatus::Scanning;
    m_foundFileCount = 0;
    m_cancelRequested = false;
    
    std::vector<VideoFile> results;
    
    try {
        for (const auto& path : paths) {
            if (m_cancelRequested) break;
            
            if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
                scanDirectoryRecursive(path, results, progressCallback);
            }
        }
        
        if (!m_cancelRequested) {
            m_status = ScanStatus::Completed;
            
            // 调用完成回调
            if (completionCallback) {
                completionCallback(results);
            }
        }
    }
    catch (const std::exception&) {
        m_status = ScanStatus::Error;
        
        // 即使出错也调用完成回调，传递已找到的结果
        if (completionCallback) {
            completionCallback(results);
        }
    }
    
    m_isScanning = false;
}

} // namespace VideoViewer