#include "DatabaseManager.h"
#include <sqlite3.h>
#include <iostream>
#include <codecvt>
#include <locale>
#include <windows.h>

DatabaseManager::DatabaseManager(const std::wstring& dbPath)
    : m_db(nullptr)
    , m_dbPath(dbPath)
    , m_isConnected(false)
{
}

DatabaseManager::~DatabaseManager()
{
    Close();
}

bool DatabaseManager::Initialize()
{
    if (m_isConnected) {
        return true;
    }

    // 转换路径为UTF-8
    std::string utf8Path = WStringToUTF8(m_dbPath);
    
    // 打开数据库连接
    int result = sqlite3_open(utf8Path.c_str(), &m_db);
    if (result != SQLITE_OK) {
        std::wcerr << L"无法打开数据库: " << m_dbPath << L", 错误: " << sqlite3_errmsg(m_db) << std::endl;
        sqlite3_close(m_db);
        m_db = nullptr;
        return false;
    }

    m_isConnected = true;

    // 启用外键约束
    if (!ExecuteSQL("PRAGMA foreign_keys = ON;")) {
        std::wcerr << L"无法启用外键约束" << std::endl;
        return false;
    }

    // 设置WAL模式以提高并发性能
    if (!ExecuteSQL("PRAGMA journal_mode = WAL;")) {
        std::wcerr << L"无法设置WAL模式" << std::endl;
        // 这不是致命错误，继续执行
    }

    // 创建表结构
    if (!CreateTables()) {
        std::wcerr << L"无法创建数据库表结构" << std::endl;
        return false;
    }

    std::wcout << L"数据库初始化成功: " << m_dbPath << std::endl;
    return true;
}

void DatabaseManager::Close()
{
    if (m_db) {
        sqlite3_close(m_db);
        m_db = nullptr;
        m_isConnected = false;
    }
}

bool DatabaseManager::IsConnected() const
{
    return m_isConnected && m_db != nullptr;
}

bool DatabaseManager::CreateTables()
{
    if (!IsConnected()) {
        return false;
    }

    // 创建版本表
    if (!CreateVersionTable()) {
        return false;
    }

    // 检查数据库版本
    int currentVersion = GetDatabaseVersion();
    if (currentVersion < CURRENT_DB_VERSION) {
        if (!UpgradeDatabase(currentVersion, CURRENT_DB_VERSION)) {
            return false;
        }
    }

    // 创建所有表
    return CreateVideoFilesTable() &&
           CreateThumbnailsTable() &&
           CreateTagsTable() &&
           CreateFavoritesTable();
}

bool DatabaseManager::ExecuteSQL(const std::string& sql)
{
    if (!IsConnected()) {
        return false;
    }

    char* errorMsg = nullptr;
    int result = sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &errorMsg);
    
    if (result != SQLITE_OK) {
        std::cerr << "SQL执行失败: " << sql << std::endl;
        std::cerr << "错误: " << (errorMsg ? errorMsg : "未知错误") << std::endl;
        if (errorMsg) {
            sqlite3_free(errorMsg);
        }
        return false;
    }

    return true;
}

sqlite3_stmt* DatabaseManager::PrepareStatement(const std::string& sql)
{
    if (!IsConnected()) {
        return nullptr;
    }

    sqlite3_stmt* stmt = nullptr;
    int result = sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr);
    
    if (result != SQLITE_OK) {
        std::cerr << "SQL语句准备失败: " << sql << std::endl;
        std::cerr << "错误: " << sqlite3_errmsg(m_db) << std::endl;
        return nullptr;
    }

    return stmt;
}

void DatabaseManager::FinalizeStatement(sqlite3_stmt* stmt)
{
    if (stmt) {
        sqlite3_finalize(stmt);
    }
}

bool DatabaseManager::BeginTransaction()
{
    return ExecuteSQL("BEGIN TRANSACTION;");
}

bool DatabaseManager::CommitTransaction()
{
    return ExecuteSQL("COMMIT;");
}

bool DatabaseManager::RollbackTransaction()
{
    return ExecuteSQL("ROLLBACK;");
}

int64_t DatabaseManager::GetLastInsertRowId() const
{
    if (!IsConnected()) {
        return -1;
    }
    return sqlite3_last_insert_rowid(m_db);
}

std::string DatabaseManager::GetLastError() const
{
    if (!IsConnected()) {
        return "数据库未连接";
    }
    return sqlite3_errmsg(m_db);
}

int DatabaseManager::GetDatabaseVersion() const
{
    if (!IsConnected()) {
        return 0;
    }

    sqlite3_stmt* stmt = const_cast<DatabaseManager*>(this)->PrepareStatement(
        "SELECT version FROM database_version LIMIT 1;"
    );
    
    if (!stmt) {
        return 0;
    }

    int version = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        version = sqlite3_column_int(stmt, 0);
    }

    const_cast<DatabaseManager*>(this)->FinalizeStatement(stmt);
    return version;
}

bool DatabaseManager::SetDatabaseVersion(int version)
{
    if (!IsConnected()) {
        return false;
    }

    const std::string sql = "INSERT OR REPLACE INTO database_version (id, version) VALUES (1, ?)";
    sqlite3_stmt* stmt = PrepareStatement(sql);
    if (!stmt) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, version);
    int result = sqlite3_step(stmt);
    FinalizeStatement(stmt);

    return result == SQLITE_DONE;
}

bool DatabaseManager::UpgradeDatabase(int fromVersion, int toVersion)
{
    if (!IsConnected()) {
        return false;
    }

    std::wcout << L"升级数据库从版本 " << fromVersion << L" 到版本 " << toVersion << std::endl;

    // 开始事务
    if (!BeginTransaction()) {
        return false;
    }

    try {
        // 根据版本执行相应的升级脚本
        for (int version = fromVersion; version < toVersion; ++version) {
            switch (version) {
            case 0:
                // 从版本0升级到版本1
                // 这里可以添加具体的升级逻辑
                break;
            default:
                std::wcerr << L"未知的数据库版本: " << version << std::endl;
                RollbackTransaction();
                return false;
            }
        }

        // 更新数据库版本
        if (!SetDatabaseVersion(toVersion)) {
            RollbackTransaction();
            return false;
        }

        // 提交事务
        if (!CommitTransaction()) {
            return false;
        }

        std::wcout << L"数据库升级成功" << std::endl;
        return true;
    }
    catch (...) {
        RollbackTransaction();
        std::wcerr << L"数据库升级过程中发生异常" << std::endl;
        return false;
    }
}

std::string DatabaseManager::WStringToUTF8(const std::wstring& wstr) const
{
    if (wstr.empty()) {
        return std::string();
    }

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

bool DatabaseManager::CreateVideoFilesTable()
{
    const std::string sql = R"(
        CREATE TABLE IF NOT EXISTS video_files (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            file_path TEXT NOT NULL UNIQUE,
            file_name TEXT NOT NULL,
            file_size INTEGER NOT NULL,
            duration INTEGER DEFAULT 0,
            width INTEGER DEFAULT 0,
            height INTEGER DEFAULT 0,
            frame_rate REAL DEFAULT 0.0,
            bit_rate INTEGER DEFAULT 0,
            codec TEXT DEFAULT '',
            created_time INTEGER NOT NULL,
            modified_time INTEGER NOT NULL,
            last_accessed INTEGER DEFAULT 0,
            is_favorite INTEGER DEFAULT 0,
            play_count INTEGER DEFAULT 0,
            last_played INTEGER DEFAULT 0,
            thumbnail_path TEXT DEFAULT '',
            metadata_extracted INTEGER DEFAULT 0
        )
    )";

    if (!ExecuteSQL(sql)) {
        return false;
    }

    // 创建索引以提高查询性能
    const std::vector<std::string> indexes = {
        "CREATE INDEX IF NOT EXISTS idx_video_files_path ON video_files(file_path)",
        "CREATE INDEX IF NOT EXISTS idx_video_files_name ON video_files(file_name)",
        "CREATE INDEX IF NOT EXISTS idx_video_files_size ON video_files(file_size)",
        "CREATE INDEX IF NOT EXISTS idx_video_files_duration ON video_files(duration)",
        "CREATE INDEX IF NOT EXISTS idx_video_files_favorite ON video_files(is_favorite)",
        "CREATE INDEX IF NOT EXISTS idx_video_files_play_count ON video_files(play_count)",
        "CREATE INDEX IF NOT EXISTS idx_video_files_last_played ON video_files(last_played)"
    };

    for (const auto& indexSql : indexes) {
        if (!ExecuteSQL(indexSql)) {
            std::wcerr << L"创建索引失败: " << indexSql.c_str() << std::endl;
            // 索引创建失败不是致命错误，继续执行
        }
    }

    return true;
}

bool DatabaseManager::CreateThumbnailsTable()
{
    const std::string sql = R"(
        CREATE TABLE IF NOT EXISTS thumbnails (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            video_file_id INTEGER NOT NULL,
            thumbnail_path TEXT NOT NULL,
            thumbnail_size INTEGER DEFAULT 0,
            created_time INTEGER NOT NULL,
            FOREIGN KEY (video_file_id) REFERENCES video_files(id) ON DELETE CASCADE
        )
    )";

    return ExecuteSQL(sql);
}

bool DatabaseManager::CreateTagsTable()
{
    const std::string sql = R"(
        CREATE TABLE IF NOT EXISTS tags (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            video_file_id INTEGER NOT NULL,
            tag_name TEXT NOT NULL,
            created_time INTEGER NOT NULL,
            FOREIGN KEY (video_file_id) REFERENCES video_files(id) ON DELETE CASCADE
        )
    )";

    return ExecuteSQL(sql);
}

bool DatabaseManager::CreateFavoritesTable()
{
    const std::string sql = R"(
        CREATE TABLE IF NOT EXISTS favorites (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            video_file_id INTEGER NOT NULL UNIQUE,
            created_time INTEGER NOT NULL,
            FOREIGN KEY (video_file_id) REFERENCES video_files(id) ON DELETE CASCADE
        )
    )";

    return ExecuteSQL(sql);
}

bool DatabaseManager::CreateVersionTable()
{
    const std::string sql = R"(
        CREATE TABLE IF NOT EXISTS database_version (
            id INTEGER PRIMARY KEY,
            version INTEGER NOT NULL
        )
    )";

    if (!ExecuteSQL(sql)) {
        return false;
    }

    // 如果版本表为空，插入初始版本
    int currentVersion = GetDatabaseVersion();
    if (currentVersion == 0) {
        return SetDatabaseVersion(CURRENT_DB_VERSION);
    }

    return true;
}