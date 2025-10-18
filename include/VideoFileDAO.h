#pragma once

#include "VideoFile.h"
#include <memory>
#include <vector>
#include <string>
#include <optional>
#include <sqlite3.h>

// 使用VideoViewer命名空间中的类型
using VideoViewer::VideoFile;

// 前向声明
class DatabaseManager;

/**
 * @brief 视频文件数据访问对象
 * 
 * 提供视频文件的数据库CRUD操作
 */
class VideoFileDAO {
public:
    /**
     * @brief 构造函数
     * @param dbManager 数据库管理器
     */
    explicit VideoFileDAO(std::shared_ptr<DatabaseManager> dbManager);

    /**
     * @brief 析构函数
     */
    ~VideoFileDAO() = default;

    /**
     * @brief 插入新的视频文件记录
     * @param videoFile 视频文件对象
     * @return 成功返回插入的记录ID，失败返回-1
     */
    int64_t Insert(const VideoFile& videoFile);

    /**
     * @brief 根据ID查询视频文件
     * @param id 视频文件ID
     * @return 找到返回VideoFile对象，否则返回nullopt
     */
    std::optional<VideoFile> FindById(int64_t id);

    /**
     * @brief 根据文件路径查询视频文件
     * @param filePath 文件路径
     * @return 找到返回VideoFile对象，否则返回nullopt
     */
    std::optional<VideoFile> FindByPath(const std::wstring& filePath);

    /**
     * @brief 更新视频文件记录
     * @param videoFile 视频文件对象（必须包含有效的ID）
     * @return 成功返回true，失败返回false
     */
    bool Update(const VideoFile& videoFile);

    /**
     * @brief 删除视频文件记录
     * @param id 视频文件ID
     * @return 成功返回true，失败返回false
     */
    bool Delete(int64_t id);

    /**
     * @brief 根据文件路径删除视频文件记录
     * @param filePath 文件路径
     * @return 成功返回true，失败返回false
     */
    bool DeleteByPath(const std::wstring& filePath);

    /**
     * @brief 查询所有视频文件
     * @return 视频文件列表
     */
    std::vector<VideoFile> FindAll();

    /**
     * @brief 根据文件名模糊查询视频文件
     * @param pattern 文件名模式（支持通配符）
     * @return 匹配的视频文件列表
     */
    std::vector<VideoFile> FindByNamePattern(const std::wstring& pattern);

    /**
     * @brief 根据文件大小范围查询视频文件
     * @param minSize 最小文件大小（字节）
     * @param maxSize 最大文件大小（字节）
     * @return 匹配的视频文件列表
     */
    std::vector<VideoFile> FindBySizeRange(int64_t minSize, int64_t maxSize);

    /**
     * @brief 根据时长范围查询视频文件
     * @param minDuration 最小时长（秒）
     * @param maxDuration 最大时长（秒）
     * @return 匹配的视频文件列表
     */
    std::vector<VideoFile> FindByDurationRange(int minDuration, int maxDuration);

    /**
     * @brief 查询收藏的视频文件
     * @return 收藏的视频文件列表
     */
    std::vector<VideoFile> FindFavorites();

    /**
     * @brief 根据最后播放时间排序查询视频文件
     * @param limit 返回记录数限制
     * @return 最近播放的视频文件列表
     */
    std::vector<VideoFile> FindRecentlyPlayed(int limit = 10);

    /**
     * @brief 根据播放次数排序查询视频文件
     * @param limit 返回记录数限制
     * @return 最常播放的视频文件列表
     */
    std::vector<VideoFile> FindMostPlayed(int limit = 10);

    /**
     * @brief 设置视频文件为收藏/取消收藏
     * @param id 视频文件ID
     * @param isFavorite 是否收藏
     * @return 成功返回true，失败返回false
     */
    bool SetFavorite(int64_t id, bool isFavorite);

    /**
     * @brief 增加播放次数
     * @param id 视频文件ID
     * @return 成功返回true，失败返回false
     */
    bool IncrementPlayCount(int64_t id);

    /**
     * @brief 更新最后播放时间
     * @param id 视频文件ID
     * @return 成功返回true，失败返回false
     */
    bool UpdateLastPlayed(int64_t id);

    /**
     * @brief 批量插入视频文件
     * @param videoFiles 视频文件列表
     * @return 成功插入的记录数
     */
    int BatchInsert(const std::vector<VideoFile>& videoFiles);

    /**
     * @brief 检查文件路径是否已存在
     * @param filePath 文件路径
     * @return 存在返回true，否则返回false
     */
    bool ExistsByPath(const std::wstring& filePath);

    /**
     * @brief 获取数据库中视频文件总数
     * @return 视频文件总数
     */
    int64_t GetTotalCount();

    /**
     * @brief 获取数据库中视频文件总大小
     * @return 总大小（字节）
     */
    int64_t GetTotalSize();

    /**
     * @brief 清理不存在的文件记录
     * @return 清理的记录数
     */
    int CleanupMissingFiles();

private:
    std::shared_ptr<DatabaseManager> m_dbManager;

    /**
     * @brief 从查询结果构建VideoFile对象
     * @param stmt SQLite语句句柄
     * @return VideoFile对象
     */
    VideoFile BuildVideoFileFromResult(sqlite3_stmt* stmt);

    /**
     * @brief 绑定VideoFile参数到SQL语句
     * @param stmt SQLite语句句柄
     * @param videoFile 视频文件对象
     * @param includeId 是否包含ID参数
     */
    void BindVideoFileParameters(sqlite3_stmt* stmt, const VideoFile& videoFile, bool includeId = false);

    /**
     * @brief 转换宽字符串为UTF-8字符串
     * @param wstr 宽字符串
     * @return UTF-8字符串
     */
    std::string WStringToUTF8(const std::wstring& wstr) const;

    /**
     * @brief 转换UTF-8字符串为宽字符串
     * @param str UTF-8字符串
     * @return 宽字符串
     */
    std::wstring UTF8ToWString(const std::string& str) const;
};