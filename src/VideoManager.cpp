#include "VideoManager.h"
#include "Utils.h"
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <sstream>
#include <iostream>
#include <algorithm>

namespace VideoViewer {

VideoManager::VideoManager() {
    clearError();
}

VideoManager::~VideoManager() {
}

bool VideoManager::openWithDefaultPlayer(const std::filesystem::path& videoPath) {
    clearError();
    
    if (!fileExists(videoPath)) {
        setError("文件不存在: " + videoPath.string());
        return false;
    }
    
    if (Utils::OpenWithDefaultProgram(videoPath)) {
        return true;
    } else {
        DWORD error = GetLastError();
        setError("无法打开文件: " + Utils::GetWindowsErrorString(error));
        return false;
    }
}

bool VideoManager::openWithPlayer(const std::filesystem::path& videoPath, 
                                 const std::filesystem::path& playerPath) {
    clearError();
    
    if (!fileExists(videoPath)) {
        setError("视频文件不存在: " + videoPath.string());
        return false;
    }
    
    if (!isPlayerAvailable(playerPath)) {
        setError("播放器不可用: " + playerPath.string());
        return false;
    }
    
    if (Utils::OpenWithProgram(videoPath, playerPath)) {
        return true;
    } else {
        DWORD error = GetLastError();
        setError("无法使用指定播放器打开文件: " + Utils::GetWindowsErrorString(error));
        return false;
    }
}

bool VideoManager::moveToRecycleBin(const std::filesystem::path& videoPath) {
    clearError();
    
    if (!fileExists(videoPath)) {
        setError("文件不存在: " + videoPath.string());
        return false;
    }
    
    if (Utils::MoveToRecycleBin(videoPath)) {
        return true;
    } else {
        DWORD error = GetLastError();
        setError("无法删除文件: " + Utils::GetWindowsErrorString(error));
        return false;
    }
}

VideoInfo VideoManager::getVideoInfo(const std::filesystem::path& videoPath) {
    VideoInfo info;
    clearError();
    
    if (!fileExists(videoPath)) {
        setError("文件不存在: " + videoPath.string());
        return info;
    }
    
    // 目前返回基础信息，后续可以使用 Media Foundation 获取详细信息
    try {
        auto fileSize = std::filesystem::file_size(videoPath);
        // 这里可以添加更多的视频信息提取逻辑
        // 暂时只设置一些默认值
        info.width = 0;
        info.height = 0;
        info.duration = 0.0;
        info.codec = "未知";
        info.frameRate = 0.0;
        info.bitRate = 0;
    }
    catch (const std::filesystem::filesystem_error& e) {
        setError("获取文件信息失败: " + std::string(e.what()));
    }
    
    return info;
}

bool VideoManager::fileExists(const std::filesystem::path& videoPath) {
    return std::filesystem::exists(videoPath) && std::filesystem::is_regular_file(videoPath);
}

bool VideoManager::renameFile(const std::filesystem::path& oldPath, 
                             const std::filesystem::path& newPath) {
    clearError();
    
    if (!fileExists(oldPath)) {
        setError("源文件不存在: " + oldPath.string());
        return false;
    }
    
    if (std::filesystem::exists(newPath)) {
        setError("目标文件已存在: " + newPath.string());
        return false;
    }
    
    try {
        std::filesystem::rename(oldPath, newPath);
        return true;
    }
    catch (const std::filesystem::filesystem_error& e) {
        setError("重命名文件失败: " + std::string(e.what()));
        return false;
    }
}

bool VideoManager::copyPathToClipboard(const std::filesystem::path& videoPath) {
    clearError();
    
    std::string pathStr = videoPath.string();
    
    if (OpenClipboard(NULL)) {
        EmptyClipboard();
        
        HGLOBAL hClipboardData = GlobalAlloc(GMEM_DDESHARE, pathStr.size() + 1);
        if (hClipboardData) {
            char* pchData = (char*)GlobalLock(hClipboardData);
            if (pchData) {
                strcpy_s(pchData, pathStr.size() + 1, pathStr.c_str());
                GlobalUnlock(hClipboardData);
                
                SetClipboardData(CF_TEXT, hClipboardData);
                CloseClipboard();
                return true;
            }
            GlobalFree(hClipboardData);
        }
        CloseClipboard();
    }
    
    DWORD error = GetLastError();
    setError("复制路径到剪贴板失败: " + Utils::GetWindowsErrorString(error));
    return false;
}

bool VideoManager::showInExplorer(const std::filesystem::path& videoPath) {
    clearError();
    
    if (!fileExists(videoPath)) {
        setError("文件不存在: " + videoPath.string());
        return false;
    }
    
    std::wstring wPath = videoPath.wstring();
    std::wstring command = L"/select,\"" + wPath + L"\"";
    
    HINSTANCE result = ShellExecuteW(NULL, L"open", L"explorer.exe", 
                                    command.c_str(), NULL, SW_SHOWNORMAL);
    
    if (reinterpret_cast<intptr_t>(result) > 32) {
        return true;
    } else {
        DWORD error = GetLastError();
        setError("无法在资源管理器中显示文件: " + Utils::GetWindowsErrorString(error));
        return false;
    }
}

std::string VideoManager::getFileProperties(const std::filesystem::path& videoPath) {
    clearError();
    
    if (!fileExists(videoPath)) {
        setError("文件不存在: " + videoPath.string());
        return "";
    }
    
    try {
        std::ostringstream oss;
        
        // 基本文件信息
        oss << "文件路径: " << videoPath.string() << "\n";
        oss << "文件名: " << videoPath.filename().string() << "\n";
        oss << "文件大小: " << Utils::FormatFileSize(std::filesystem::file_size(videoPath)) << "\n";
        oss << "修改时间: " << Utils::FormatFileTime(std::filesystem::last_write_time(videoPath)) << "\n";
        oss << "文件类型: " << videoPath.extension().string() << "\n";
        
        // 可以添加更多属性信息
        return oss.str();
    }
    catch (const std::filesystem::filesystem_error& e) {
        setError("获取文件属性失败: " + std::string(e.what()));
        return "";
    }
}

void VideoManager::setDefaultPlayerPath(const std::filesystem::path& playerPath) {
    m_defaultPlayerPath = playerPath;
}

std::filesystem::path VideoManager::getDefaultPlayerPath() const {
    return m_defaultPlayerPath;
}

bool VideoManager::isPlayerAvailable(const std::filesystem::path& playerPath) {
    return std::filesystem::exists(playerPath) && std::filesystem::is_regular_file(playerPath);
}

std::string VideoManager::getLastError() const {
    return m_lastError;
}

void VideoManager::setError(const std::string& error) {
    m_lastError = error;
}

void VideoManager::clearError() {
    m_lastError.clear();
}

// 数据库相关方法实现
bool VideoManager::initializeDatabase(const std::filesystem::path& dbPath) {
    clearError();
    
    try {
        m_dbManager = std::make_shared<DatabaseManager>(dbPath.wstring());
        if (!m_dbManager->Initialize()) {
            setError("无法初始化数据库: " + dbPath.string());
            return false;
        }
        
        m_videoDAO = std::make_shared<VideoFileDAO>(m_dbManager);
        return true;
    } catch (const std::exception& e) {
        setError("数据库初始化异常: " + std::string(e.what()));
        return false;
    }
}

int VideoManager::scanDirectoryToDatabase(const std::filesystem::path& directory) {
    clearError();
    
    if (!m_dbManager || !m_videoDAO) {
        setError("数据库未初始化");
        return -1;
    }
    
    if (!std::filesystem::exists(directory)) {
        setError("目录不存在: " + directory.string());
        return -1;
    }
    
    int addedCount = 0;
    std::vector<VideoFile> videoFiles;
    
    try {
        // 支持的视频文件扩展名
        std::vector<std::string> videoExtensions = {
            ".mp4", ".avi", ".mkv", ".mov", ".wmv", ".flv", 
            ".webm", ".m4v", ".3gp", ".mpg", ".mpeg"
        };
        
        // 递归扫描目录
        for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                auto extension = entry.path().extension().string();
                std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
                
                // 检查是否为视频文件
                if (std::find(videoExtensions.begin(), videoExtensions.end(), extension) != videoExtensions.end()) {
                    // 检查文件是否已存在于数据库中
                    if (!m_videoDAO->ExistsByPath(entry.path().wstring())) {
                        VideoFile videoFile(entry.path());
                        
                        // 获取文件信息
                        auto fileSize = std::filesystem::file_size(entry.path());
                        auto lastWrite = std::filesystem::last_write_time(entry.path());
                        
                        videoFile.fileSize = static_cast<std::uintmax_t>(fileSize);
                        videoFile.lastModified = lastWrite;
                        videoFile.createdTime = std::chrono::system_clock::now();
                        videoFile.lastAccessed = std::chrono::system_clock::now();
                        
                        videoFiles.push_back(videoFile);
                    }
                }
            }
        }
        
        // 批量插入到数据库
        if (!videoFiles.empty()) {
            addedCount = m_videoDAO->BatchInsert(videoFiles);
            if (addedCount < 0) {
                setError("批量插入视频文件失败");
                return -1;
            }
        }
        
    } catch (const std::exception& e) {
        setError("扫描目录时发生异常: " + std::string(e.what()));
        return -1;
    }
    
    return addedCount;
}

std::vector<VideoFile> VideoManager::loadVideosFromDatabase() {
    clearError();
    
    if (!m_dbManager || !m_videoDAO) {
        setError("数据库未初始化");
        return {};
    }
    
    try {
        return m_videoDAO->FindAll();
    } catch (const std::exception& e) {
        setError("从数据库加载视频文件时发生异常: " + std::string(e.what()));
        return {};
    }
}

bool VideoManager::addVideoToDatabase(const VideoFile& videoFile) {
    clearError();
    
    if (!m_dbManager || !m_videoDAO) {
        setError("数据库未初始化");
        return false;
    }
    
    try {
        int64_t id = m_videoDAO->Insert(videoFile);
        return id > 0;
    } catch (const std::exception& e) {
        setError("添加视频文件到数据库时发生异常: " + std::string(e.what()));
        return false;
    }
}

bool VideoManager::removeVideoFromDatabase(const std::filesystem::path& filePath) {
    clearError();
    
    if (!m_dbManager || !m_videoDAO) {
        setError("数据库未初始化");
        return false;
    }
    
    try {
        return m_videoDAO->DeleteByPath(filePath.wstring());
    } catch (const std::exception& e) {
        setError("从数据库删除视频文件时发生异常: " + std::string(e.what()));
        return false;
    }
}

bool VideoManager::updatePlayCount(const std::filesystem::path& filePath) {
    clearError();
    
    if (!m_dbManager || !m_videoDAO) {
        setError("数据库未初始化");
        return false;
    }
    
    try {
        // 先查找文件获取ID
        auto videoFile = m_videoDAO->FindByPath(filePath.wstring());
        if (!videoFile.has_value()) {
            setError("在数据库中未找到文件: " + filePath.string());
            return false;
        }
        
        // 更新播放次数和最后播放时间
        bool success = m_videoDAO->IncrementPlayCount(videoFile->id);
        if (success) {
            success = m_videoDAO->UpdateLastPlayed(videoFile->id);
        }
        
        return success;
    } catch (const std::exception& e) {
        setError("更新播放次数时发生异常: " + std::string(e.what()));
        return false;
    }
}

bool VideoManager::setVideoFavorite(const std::filesystem::path& filePath, bool isFavorite) {
    clearError();
    
    if (!m_dbManager || !m_videoDAO) {
        setError("数据库未初始化");
        return false;
    }
    
    try {
        // 先查找文件获取ID
        auto videoFile = m_videoDAO->FindByPath(filePath.wstring());
        if (!videoFile.has_value()) {
            setError("在数据库中未找到文件: " + filePath.string());
            return false;
        }
        
        return m_videoDAO->SetFavorite(videoFile->id, isFavorite);
    } catch (const std::exception& e) {
        setError("设置收藏状态时发生异常: " + std::string(e.what()));
        return false;
    }
}

int VideoManager::cleanupMissingFiles() {
    clearError();
    
    if (!m_dbManager || !m_videoDAO) {
        setError("数据库未初始化");
        return -1;
    }
    
    try {
        return m_videoDAO->CleanupMissingFiles();
    } catch (const std::exception& e) {
        setError("清理缺失文件时发生异常: " + std::string(e.what()));
        return -1;
    }
}

} // namespace VideoViewer