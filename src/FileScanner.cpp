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

std::vector<VideoFile> FileScanner::scanDirectory(const std::filesystem::path& path, bool recursive) {
    std::vector<VideoFile> results;
    
    if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path)) {
        return results;
    }
    
    m_status = ScanStatus::Scanning;
    m_foundFileCount = 0;
    m_cancelRequested = false;
    
    try {
        scanDirectoryRecursive(path, results, nullptr, recursive);
        m_status = ScanStatus::Completed;
    }
    catch (const std::exception&) {
        m_status = ScanStatus::Error;
    }
    
    return results;
}

void FileScanner::scanDirectoryAsync(const std::filesystem::path& path,
                                    ProgressCallback progressCallback,
                                    CompletionCallback completionCallback,
                                    bool recursive) {
    std::vector<std::filesystem::path> paths = { path };
    scanDirectoriesAsync(paths, progressCallback, completionCallback, recursive);
}

std::vector<VideoFile> FileScanner::scanDirectories(const std::vector<std::filesystem::path>& paths, bool recursive) {
    std::vector<VideoFile> results;
    
    m_status = ScanStatus::Scanning;
    m_foundFileCount = 0;
    m_cancelRequested = false;
    
    try {
        for (const auto& path : paths) {
            if (m_cancelRequested) break;
            
            if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
                scanDirectoryRecursive(path, results, nullptr, recursive);
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
                                      CompletionCallback completionCallback,
                                      bool recursive) {
    // 取消之前的扫描
    cancelScan();
    
    // 启动新的扫描线程
    m_scanThread = std::make_unique<std::thread>(
        &FileScanner::scanWorker, this, paths, progressCallback, completionCallback, recursive);
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
                                        ProgressCallback progressCallback,
                                        bool recursive) {
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
                else if (entry.is_directory() && recursive) {
                    // 只有在递归模式下才扫描子目录
                    scanDirectoryRecursive(entry.path(), results, progressCallback, recursive);
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
                            CompletionCallback completionCallback,
                            bool recursive) {
    m_isScanning = true;
    m_status = ScanStatus::Scanning;
    m_foundFileCount = 0;
    m_cancelRequested = false;
    
    std::vector<VideoFile> results;
    
    try {
        for (const auto& path : paths) {
            if (m_cancelRequested) break;
            
            if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
                scanDirectoryRecursive(path, results, progressCallback, recursive);
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

ScanResult FileScanner::scanDirectoryWithFolders(const std::filesystem::path& path, bool recursive) {
    ScanResult result;
    
    if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path)) {
        return result;
    }
    
    m_status = ScanStatus::Scanning;
    m_foundFileCount = 0;
    m_cancelRequested = false;
    
    try {
        scanDirectoryWithFoldersRecursive(path, result, nullptr, recursive);
        m_status = ScanStatus::Completed;
    }
    catch (const std::exception&) {
        m_status = ScanStatus::Error;
    }
    
    return result;
}

void FileScanner::scanDirectoryWithFoldersAsync(const std::filesystem::path& path,
                                               ProgressCallback progressCallback,
                                               ScanResultCallback resultCallback,
                                               bool recursive) {
    // 取消之前的扫描
    cancelScan();
    
    // 启动新的扫描线程
    m_scanThread = std::make_unique<std::thread>(&FileScanner::scanWithFoldersWorker, this,
                                                path, progressCallback, resultCallback, recursive);
}

void FileScanner::scanDirectoryWithFoldersRecursive(const std::filesystem::path& path,
                                                   ScanResult& result,
                                                   ProgressCallback progressCallback,
                                                   bool recursive) {
    if (m_cancelRequested) return;
    
    try {
        // 更新当前扫描路径
        {
            std::lock_guard<std::mutex> lock(m_pathMutex);
            m_currentScanPath = path;
        }
        
        std::error_code ec;
        for (const auto& entry : std::filesystem::directory_iterator(path, ec)) {
            if (m_cancelRequested) break;
            
            if (entry.is_directory(ec)) {
                // 收集文件夹信息
                FolderItem folder;
                folder.folderPath = entry.path();
                folder.folderName = entry.path().filename().string();
                
                try {
                    folder.lastModified = std::filesystem::last_write_time(entry.path());
                } catch (...) {
                    folder.lastModified = std::filesystem::file_time_type{};
                }
                
                // 统计文件夹中的视频文件数量和总大小
                folder.videoCount = 0;
                folder.totalSize = 0;
                
                std::error_code subEc;
                for (const auto& subEntry : std::filesystem::recursive_directory_iterator(entry.path(), subEc)) {
                    if (subEntry.is_regular_file(subEc) && isVideoFile(subEntry.path())) {
                        folder.videoCount++;
                        try {
                            folder.totalSize += std::filesystem::file_size(subEntry.path());
                        } catch (...) {
                            // 忽略无法获取大小的文件
                        }
                    }
                }
                
                result.folders.push_back(folder);
                
                // 如果是递归模式，继续扫描子目录
                if (recursive) {
                    scanDirectoryWithFoldersRecursive(entry.path(), result, progressCallback, recursive);
                }
            }
            else if (entry.is_regular_file(ec) && isVideoFile(entry.path())) {
                // 处理视频文件
                VideoFile videoFile;
                videoFile.filePath = entry.path();
                videoFile.fileName = entry.path().filename().string();
                videoFile.extension = Utils::GetLowerExtension(entry.path());
                
                try {
                    videoFile.fileSize = std::filesystem::file_size(entry.path());
                    videoFile.lastModified = std::filesystem::last_write_time(entry.path());
                } catch (...) {
                    videoFile.fileSize = 0;
                    videoFile.lastModified = std::filesystem::file_time_type{};
                }
                
                result.videoFiles.push_back(videoFile);
                m_foundFileCount++;
                
                // 调用进度回调
                if (progressCallback) {
                    progressCallback(entry.path(), m_foundFileCount);
                }
            }
        }
    }
    catch (const std::exception&) {
        // 忽略访问错误，继续扫描其他目录
    }
}

void FileScanner::scanWithFoldersWorker(const std::filesystem::path& path,
                                       ProgressCallback progressCallback,
                                       ScanResultCallback resultCallback,
                                       bool recursive) {
    m_isScanning = true;
    m_status = ScanStatus::Scanning;
    m_foundFileCount = 0;
    m_cancelRequested = false;
    
    try {
        ScanResult result;
        
        if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
            scanDirectoryWithFoldersRecursive(path, result, progressCallback, recursive);
        }
        
        if (!m_cancelRequested && resultCallback) {
            resultCallback(result);
        }
        
        m_status = m_cancelRequested ? ScanStatus::Idle : ScanStatus::Completed;
    }
    catch (const std::exception&) {
        m_status = ScanStatus::Error;
    }
    
    m_isScanning = false;
}

} // namespace VideoViewer