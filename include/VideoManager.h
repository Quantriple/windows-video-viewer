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
 * @brief 视频信息结构体
 */
struct VideoInfo {
    int width = 0;          ///< 视频宽度
    int height = 0;         ///< 视频高度
    double duration = 0.0;  ///< 视频时长（秒）
    std::string codec;      ///< 视频编码
    double frameRate = 0.0; ///< 帧率
    int64_t bitRate = 0;    ///< 比特率
};

/**
 * @brief 视频管理器类
 * 
 * 提供视频文件的播放、管理和数据库操作功能
 */
class VideoManager {
public:
    /**
     * @brief 构造函数
     */
    VideoManager();

    /**
     * @brief 析构函数
     */
    ~VideoManager();

    // 视频播放相关方法
    
    /**
     * @brief 使用默认播放器打开视频文件
     * @param videoPath 视频文件路径
     * @return 成功返回true，失败返回false
     */
    bool openWithDefaultPlayer(const std::filesystem::path& videoPath);

    /**
     * @brief 使用指定播放器打开视频文件
     * @param videoPath 视频文件路径
     * @param playerPath 播放器路径
     * @return 成功返回true，失败返回false
     */
    bool openWithPlayer(const std::filesystem::path& videoPath, 
                       const std::filesystem::path& playerPath);

    /**
     * @brief 获取视频文件信息
     * @param videoPath 视频文件路径
     * @return 视频信息结构体
     */
    VideoInfo getVideoInfo(const std::filesystem::path& videoPath);

    // 文件操作相关方法
    
    /**
     * @brief 将文件移动到回收站
     * @param videoPath 视频文件路径
     * @return 成功返回true，失败返回false
     */
    bool moveToRecycleBin(const std::filesystem::path& videoPath);

    /**
     * @brief 重命名文件
     * @param oldPath 原文件路径
     * @param newPath 新文件路径
     * @return 成功返回true，失败返回false
     */
    bool renameFile(const std::filesystem::path& oldPath, 
                   const std::filesystem::path& newPath);

    /**
     * @brief 复制文件路径到剪贴板
     * @param videoPath 视频文件路径
     * @return 成功返回true，失败返回false
     */
    bool copyPathToClipboard(const std::filesystem::path& videoPath);

    /**
     * @brief 在资源管理器中显示文件
     * @param videoPath 视频文件路径
     * @return 成功返回true，失败返回false
     */
    bool showInExplorer(const std::filesystem::path& videoPath);

    /**
     * @brief 获取文件属性信息
     * @param videoPath 视频文件路径
     * @return 文件属性字符串
     */
    std::string getFileProperties(const std::filesystem::path& videoPath);

    // 播放器管理相关方法
    
    /**
     * @brief 设置默认播放器路径
     * @param playerPath 播放器路径
     */
    void setDefaultPlayerPath(const std::filesystem::path& playerPath);

    /**
     * @brief 获取默认播放器路径
     * @return 播放器路径
     */
    std::filesystem::path getDefaultPlayerPath() const;

    /**
     * @brief 检查播放器是否可用
     * @param playerPath 播放器路径
     * @return 可用返回true，否则返回false
     */
    bool isPlayerAvailable(const std::filesystem::path& playerPath);

    // 数据库相关方法
    
    /**
     * @brief 初始化数据库
     * @param dbPath 数据库文件路径
     * @return 成功返回true，失败返回false
     */
    bool initializeDatabase(const std::filesystem::path& dbPath);

    /**
     * @brief 扫描目录并将视频文件添加到数据库
     * @param directory 要扫描的目录
     * @return 添加的文件数量，失败返回-1
     */
    int scanDirectoryToDatabase(const std::filesystem::path& directory);

    /**
     * @brief 从数据库加载所有视频文件
     * @return 视频文件列表
     */
    std::vector<VideoFile> loadVideosFromDatabase();

    /**
     * @brief 添加视频文件到数据库
     * @param videoFile 视频文件对象
     * @return 成功返回true，失败返回false
     */
    bool addVideoToDatabase(const VideoFile& videoFile);

    /**
     * @brief 从数据库删除视频文件
     * @param filePath 文件路径
     * @return 成功返回true，失败返回false
     */
    bool removeVideoFromDatabase(const std::filesystem::path& filePath);

    /**
     * @brief 更新视频文件的播放次数
     * @param filePath 文件路径
     * @return 成功返回true，失败返回false
     */
    bool updatePlayCount(const std::filesystem::path& filePath);

    /**
     * @brief 设置视频文件的收藏状态
     * @param filePath 文件路径
     * @param isFavorite 是否收藏
     * @return 成功返回true，失败返回false
     */
    bool setVideoFavorite(const std::filesystem::path& filePath, bool isFavorite);

    /**
     * @brief 清理数据库中不存在的文件记录
     * @return 清理的记录数，失败返回-1
     */
    int cleanupMissingFiles();

    // 错误处理相关方法
    
    /**
     * @brief 获取最后一次错误信息
     * @return 错误信息字符串
     */
    std::string getLastError() const;

private:
    std::filesystem::path m_defaultPlayerPath;  ///< 默认播放器路径
    std::string m_lastError;                    ///< 最后一次错误信息
    
    // 数据库相关成员
    std::shared_ptr<DatabaseManager> m_dbManager;  ///< 数据库管理器
    std::shared_ptr<VideoFileDAO> m_videoDAO;      ///< 视频文件数据访问对象

    /**
     * @brief 检查文件是否存在
     * @param videoPath 文件路径
     * @return 存在返回true，否则返回false
     */
    bool fileExists(const std::filesystem::path& videoPath);

    /**
     * @brief 设置错误信息
     * @param error 错误信息
     */
    void setError(const std::string& error);

    /**
     * @brief 清除错误信息
     */
    void clearError();
};

} // namespace VideoViewer