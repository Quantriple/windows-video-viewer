#pragma once

#include "VideoFile.h"
#include "DatabaseManager.h"
#include "VideoFileDAO.h"
#include <filesystem>
#include <string>
#include <memory>
#include <vector>

namespace VideoViewer {

/**
 * @brief 视频管理器类
 * 负责视频文件的播放、删除等操作
 */
class VideoManager {
public:
    VideoManager();
    ~VideoManager();

    /**
     * @brief 使用默认播放器打开视频文件
     * @param videoPath 视频文件路径
     * @return 成功返回 true
     */
    bool openWithDefaultPlayer(const std::filesystem::path& videoPath);

    /**
     * @brief 使用指定播放器打开视频文件
     * @param videoPath 视频文件路径
     * @param playerPath 播放器程序路径
     * @return 成功返回 true
     */
    bool openWithPlayer(const std::filesystem::path& videoPath, 
                       const std::filesystem::path& playerPath);

    /**
     * @brief 将文件移动到回收站
     * @param videoPath 视频文件路径
     * @return 成功返回 true
     */
    bool moveToRecycleBin(const std::filesystem::path& videoPath);

    /**
     * @brief 获取视频文件信息（扩展功能，暂时返回基础信息）
     * @param videoPath 视频文件路径
     * @return 视频信息结构体
     */
    VideoInfo getVideoInfo(const std::filesystem::path& videoPath);

    /**
     * @brief 检查文件是否存在
     * @param videoPath 视频文件路径
     * @return 文件存在返回 true
     */
    bool fileExists(const std::filesystem::path& videoPath);

    /**
     * @brief 重命名视频文件
     * @param oldPath 原文件路径
     * @param newPath 新文件路径
     * @return 成功返回 true
     */
    bool renameFile(const std::filesystem::path& oldPath, 
                   const std::filesystem::path& newPath);

    /**
     * @brief 复制文件路径到剪贴板
     * @param videoPath 视频文件路径
     * @return 成功返回 true
     */
    bool copyPathToClipboard(const std::filesystem::path& videoPath);

    /**
     * @brief 在资源管理器中显示文件
     * @param videoPath 视频文件路径
     * @return 成功返回 true
     */
    bool showInExplorer(const std::filesystem::path& videoPath);

    /**
     * @brief 获取文件属性信息
     * @param videoPath 视频文件路径
     * @return 文件属性字符串
     */
    std::string getFileProperties(const std::filesystem::path& videoPath);

    /**
     * @brief 设置默认播放器路径
     * @param playerPath 播放器程序路径
     */
    void setDefaultPlayerPath(const std::filesystem::path& playerPath);

    /**
     * @brief 获取默认播放器路径
     * @return 播放器程序路径
     */
    std::filesystem::path getDefaultPlayerPath() const;

    /**
     * @brief 检查播放器是否可用
     * @param playerPath 播放器程序路径
     * @return 播放器可用返回 true
     */
    bool isPlayerAvailable(const std::filesystem::path& playerPath);

    /**
     * @brief 获取最后一次错误信息
     * @return 错误信息字符串
     */
    std::string getLastError() const;

    // 数据库相关方法
    /**
     * @brief 初始化数据库连接
     * @param dbPath 数据库文件路径
     * @return 成功返回 true
     */
    bool initializeDatabase(const std::filesystem::path& dbPath);

    /**
     * @brief 扫描目录并将视频文件添加到数据库
     * @param directory 要扫描的目录
     * @return 添加的文件数量
     */
    int scanDirectoryToDatabase(const std::filesystem::path& directory);

    /**
     * @brief 从数据库加载所有视频文件
     * @return 视频文件列表
     */
    std::vector<VideoFile> loadVideosFromDatabase();

    /**
     * @brief 将视频文件添加到数据库
     * @param videoFile 视频文件对象
     * @return 成功返回 true
     */
    bool addVideoToDatabase(const VideoFile& videoFile);

    /**
     * @brief 从数据库中删除视频文件记录
     * @param filePath 文件路径
     * @return 成功返回 true
     */
    bool removeVideoFromDatabase(const std::filesystem::path& filePath);

    /**
     * @brief 更新视频文件的播放次数
     * @param filePath 文件路径
     * @return 成功返回 true
     */
    bool updatePlayCount(const std::filesystem::path& filePath);

    /**
     * @brief 设置视频文件为收藏/取消收藏
     * @param filePath 文件路径
     * @param isFavorite 是否收藏
     * @return 成功返回 true
     */
    bool setVideoFavorite(const std::filesystem::path& filePath, bool isFavorite);

    /**
     * @brief 清理数据库中不存在的文件记录
     * @return 清理的记录数量
     */
    int cleanupMissingFiles();

private:
    /**
     * @brief 设置错误信息
     * @param error 错误信息
     */
    void setError(const std::string& error);

    /**
     * @brief 清除错误信息
     */
    void clearError();

    std::filesystem::path m_defaultPlayerPath;  // 默认播放器路径
    std::string m_lastError;                    // 最后一次错误信息
    
    // 数据库相关成员
    std::shared_ptr<DatabaseManager> m_dbManager;  // 数据库管理器
    std::shared_ptr<VideoFileDAO> m_videoDAO;      // 视频文件数据访问对象
};

} // namespace VideoViewer