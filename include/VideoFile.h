#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <chrono>

namespace VideoViewer {

/**
 * @brief 视频文件信息结构体
 */
struct VideoFile {
    // 数据库字段
    int64_t id;                                       // 数据库ID
    std::filesystem::path filePath;                    // 文件完整路径
    std::string fileName;                              // 文件名（不含路径）
    std::uintmax_t fileSize;                          // 文件大小（字节）
    std::filesystem::file_time_type lastModified;     // 最后修改时间
    std::string extension;                             // 文件扩展名
    bool hasThumbnail;                                // 是否有缩略图
    std::filesystem::path thumbnailPath;              // 缩略图路径
    
    // 视频信息字段
    int duration;                                     // 视频时长（秒）
    int width;                                        // 视频宽度
    int height;                                       // 视频高度
    double frameRate;                                 // 帧率
    int64_t bitRate;                                  // 比特率
    std::string codec;                                // 视频编码
    
    // 用户数据字段
    bool isFavorite;                                  // 是否收藏
    int playCount;                                    // 播放次数
    std::chrono::system_clock::time_point lastPlayed; // 最后播放时间
    std::chrono::system_clock::time_point createdTime; // 创建时间
    std::chrono::system_clock::time_point lastAccessed; // 最后访问时间
    bool metadataExtracted;                           // 是否已提取元数据

    VideoFile() 
        : id(-1)
        , fileSize(0)
        , hasThumbnail(false)
        , duration(0)
        , width(0)
        , height(0)
        , frameRate(0.0)
        , bitRate(0)
        , isFavorite(false)
        , playCount(0)
        , metadataExtracted(false)
    {
        auto now = std::chrono::system_clock::now();
        createdTime = now;
        lastAccessed = now;
    }

    VideoFile(const std::filesystem::path& path)
        : id(-1)
        , filePath(path)
        , fileName(path.filename().string())
        , fileSize(0)
        , extension(path.extension().string())
        , hasThumbnail(false)
        , duration(0)
        , width(0)
        , height(0)
        , frameRate(0.0)
        , bitRate(0)
        , isFavorite(false)
        , playCount(0)
        , metadataExtracted(false)
    {
        auto now = std::chrono::system_clock::now();
        createdTime = now;
        lastAccessed = now;
        
        try {
            if (std::filesystem::exists(path)) {
                fileSize = std::filesystem::file_size(path);
                lastModified = std::filesystem::last_write_time(path);
            }
        }
        catch (const std::filesystem::filesystem_error&) {
            // 忽略文件系统错误，保持默认值
        }
    }

    /**
     * @brief 获取格式化的文件大小字符串
     */
    std::string getFormattedSize() const;

    /**
     * @brief 获取格式化的修改时间字符串
     */
    std::string getFormattedTime() const;

    /**
     * @brief 检查文件是否存在
     */
    bool exists() const {
        return std::filesystem::exists(filePath);
    }
};

/**
 * @brief 项目类型枚举
 */
enum class ItemType {
    VideoFile,      // 视频文件
    Folder          // 文件夹
};

/**
 * @brief 文件夹项目结构体
 */
struct FolderItem {
    std::filesystem::path folderPath;                  // 文件夹完整路径
    std::string folderName;                            // 文件夹名（不含路径）
    std::filesystem::file_time_type lastModified;     // 最后修改时间
    size_t videoCount;                                 // 包含的视频文件数量
    std::uintmax_t totalSize;                         // 文件夹总大小

    FolderItem() 
        : videoCount(0)
        , totalSize(0)
    {
    }

    FolderItem(const std::filesystem::path& path)
        : folderPath(path)
        , folderName(path.filename().string())
        , videoCount(0)
        , totalSize(0)
    {
        try {
            if (std::filesystem::exists(path)) {
                lastModified = std::filesystem::last_write_time(path);
            }
        }
        catch (const std::filesystem::filesystem_error&) {
            // 忽略文件系统错误，保持默认值
        }
    }

    /**
     * @brief 获取格式化的文件夹大小字符串
     */
    std::string getFormattedSize() const;

    /**
     * @brief 获取格式化的修改时间字符串
     */
    std::string getFormattedTime() const;

    /**
     * @brief 检查文件夹是否存在
     */
    bool exists() const {
        return std::filesystem::exists(folderPath);
    }
};

/**
 * @brief 通用列表项结构体（用于在列表中同时显示视频文件和文件夹）
 */
struct ListItem {
    ItemType type;                                     // 项目类型
    VideoFile videoFile;                               // 视频文件（当type为VideoFile时使用）
    FolderItem folderItem;                             // 文件夹项目（当type为Folder时使用）

    ListItem(const VideoFile& file) 
        : type(ItemType::VideoFile)
        , videoFile(file)
    {
    }

    ListItem(const FolderItem& folder) 
        : type(ItemType::Folder)
        , folderItem(folder)
    {
    }

    /**
     * @brief 获取项目名称
     */
    std::string getName() const {
        return type == ItemType::VideoFile ? videoFile.fileName : folderItem.folderName;
    }

    /**
     * @brief 获取项目路径
     */
    std::filesystem::path getPath() const {
        return type == ItemType::VideoFile ? videoFile.filePath : folderItem.folderPath;
    }

    /**
     * @brief 获取格式化的大小字符串
     */
    std::string getFormattedSize() const {
        return type == ItemType::VideoFile ? videoFile.getFormattedSize() : folderItem.getFormattedSize();
    }

    /**
     * @brief 获取格式化的时间字符串
     */
    std::string getFormattedTime() const {
        return type == ItemType::VideoFile ? videoFile.getFormattedTime() : folderItem.getFormattedTime();
    }
};

/**
 * @brief 视频信息结构体（用于后续扩展）
 */
struct VideoInfo {
    int width;              // 视频宽度
    int height;             // 视频高度
    double duration;        // 视频时长（秒）
    std::string codec;      // 视频编码
    double frameRate;       // 帧率
    std::uintmax_t bitRate; // 比特率

    VideoInfo() 
        : width(0), height(0), duration(0.0), frameRate(0.0), bitRate(0) {}
};

/**
 * @brief 应用程序设置结构体
 */
struct AppSettings {
    std::vector<std::filesystem::path> scanPaths;     // 扫描路径列表
    std::filesystem::path playerPath;                 // 播放器路径
    std::filesystem::path cachePath;                  // 缓存路径
    bool useDefaultPlayer;                            // 是否使用默认播放器
    bool recursiveScan;                               // 是否递归扫描子目录
    int thumbnailSize;                                // 缩略图大小
    bool showFileSize;                                // 是否显示文件大小
    bool showModifiedTime;                            // 是否显示修改时间
    bool sortAscending;                               // 是否升序排序
    std::string sortColumn;                           // 排序列

    AppSettings()
        : useDefaultPlayer(true)
        , recursiveScan(true)
        , thumbnailSize(150)
        , showFileSize(true)
        , showModifiedTime(true)
        , sortAscending(true)
        , sortColumn("name")
    {
        // 设置默认缓存路径
        auto localAppData = std::getenv("LOCALAPPDATA");
        if (localAppData) {
            cachePath = std::filesystem::path(localAppData) / "VideoViewer" / "Cache";
        }
    }
};

/**
 * @brief 扫描状态枚举
 */
enum class ScanStatus {
    Idle,           // 空闲
    Scanning,       // 扫描中
    Completed,      // 扫描完成
    Error           // 扫描错误
};

/**
 * @brief 视图模式枚举
 */
enum class ViewMode {
    List,           // 列表视图
    Details,        // 详细信息视图
    Thumbnail       // 缩略图视图
};

} // namespace VideoViewer