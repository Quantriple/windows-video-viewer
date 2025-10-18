#pragma once

#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <cstdint>

// 前向声明，避免在头文件中包含sqlite3.h
struct sqlite3;
struct sqlite3_stmt;

/**
 * @brief 数据库管理器类
 * 
 * 负责SQLite数据库的连接管理、初始化和基础操作
 */
class DatabaseManager {
public:
    /**
     * @brief 构造函数
     * @param dbPath 数据库文件路径
     */
    explicit DatabaseManager(const std::wstring& dbPath);
    
    /**
     * @brief 析构函数
     */
    ~DatabaseManager();

    // 禁用拷贝构造和赋值
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    /**
     * @brief 初始化数据库连接
     * @return 成功返回true，失败返回false
     */
    bool Initialize();

    /**
     * @brief 关闭数据库连接
     */
    void Close();

    /**
     * @brief 检查数据库是否已连接
     * @return 已连接返回true，否则返回false
     */
    bool IsConnected() const;

    /**
     * @brief 创建数据库表结构
     * @return 成功返回true，失败返回false
     */
    bool CreateTables();

    /**
     * @brief 执行SQL语句（无返回结果）
     * @param sql SQL语句
     * @return 成功返回true，失败返回false
     */
    bool ExecuteSQL(const std::string& sql);

    /**
     * @brief 准备SQL语句
     * @param sql SQL语句
     * @return 成功返回语句句柄，失败返回nullptr
     */
    sqlite3_stmt* PrepareStatement(const std::string& sql);

    /**
     * @brief 完成语句执行并清理资源
     * @param stmt 语句句柄
     */
    void FinalizeStatement(sqlite3_stmt* stmt);

    /**
     * @brief 开始事务
     * @return 成功返回true，失败返回false
     */
    bool BeginTransaction();

    /**
     * @brief 提交事务
     * @return 成功返回true，失败返回false
     */
    bool CommitTransaction();

    /**
     * @brief 回滚事务
     * @return 成功返回true，失败返回false
     */
    bool RollbackTransaction();

    /**
     * @brief 获取最后插入的行ID
     * @return 行ID
     */
    int64_t GetLastInsertRowId() const;

    /**
     * @brief 获取最后的错误信息
     * @return 错误信息字符串
     */
    std::string GetLastError() const;

    /**
     * @brief 获取数据库版本
     * @return 数据库版本号
     */
    int GetDatabaseVersion() const;

    /**
     * @brief 设置数据库版本
     * @param version 版本号
     * @return 成功返回true，失败返回false
     */
    bool SetDatabaseVersion(int version);

    /**
     * @brief 执行数据库升级
     * @param fromVersion 当前版本
     * @param toVersion 目标版本
     * @return 成功返回true，失败返回false
     */
    bool UpgradeDatabase(int fromVersion, int toVersion);

private:
    sqlite3* m_db;                    ///< SQLite数据库连接
    std::wstring m_dbPath;            ///< 数据库文件路径
    bool m_isConnected;               ///< 连接状态
    
    static const int CURRENT_DB_VERSION = 1;  ///< 当前数据库版本

    /**
     * @brief 转换宽字符串为UTF-8字符串
     * @param wstr 宽字符串
     * @return UTF-8字符串
     */
    std::string WStringToUTF8(const std::wstring& wstr) const;

    /**
     * @brief 创建video_files表
     * @return 成功返回true，失败返回false
     */
    bool CreateVideoFilesTable();

    /**
     * @brief 创建thumbnails表
     * @return 成功返回true，失败返回false
     */
    bool CreateThumbnailsTable();

    /**
     * @brief 创建tags表
     * @return 成功返回true，失败返回false
     */
    bool CreateTagsTable();

    /**
     * @brief 创建favorites表
     * @return 成功返回true，失败返回false
     */
    bool CreateFavoritesTable();

    /**
     * @brief 创建数据库版本表
     * @return 成功返回true，失败返回false
     */
    bool CreateVersionTable();
};