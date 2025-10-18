#include "VideoFileDAO.h"
#include "DatabaseManager.h"
#include <sqlite3.h>
#include <iostream>
#include <codecvt>
#include <locale>
#include <chrono>
#include <filesystem>
#include <windows.h>

VideoFileDAO::VideoFileDAO(std::shared_ptr<DatabaseManager> dbManager)
    : m_dbManager(dbManager)
{
}

int64_t VideoFileDAO::Insert(const VideoFile& videoFile)
{
    if (!m_dbManager || !m_dbManager->IsConnected()) {
        return -1;
    }

    const std::string sql = R"(
        INSERT INTO video_files (
            file_path, file_name, file_size, duration, width, height,
            frame_rate, bit_rate, codec, created_time, modified_time,
            last_accessed, is_favorite, play_count, last_played,
            thumbnail_path, metadata_extracted
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )";

    sqlite3_stmt* stmt = m_dbManager->PrepareStatement(sql);
    if (!stmt) {
        return -1;
    }

    BindVideoFileParameters(stmt, videoFile, false);

    int result = sqlite3_step(stmt);
    m_dbManager->FinalizeStatement(stmt);

    if (result == SQLITE_DONE) {
        return m_dbManager->GetLastInsertRowId();
    }

    return -1;
}

std::optional<VideoFile> VideoFileDAO::FindById(int64_t id)
{
    if (!m_dbManager || !m_dbManager->IsConnected()) {
        return std::nullopt;
    }

    const std::string sql = R"(
        SELECT id, file_path, file_name, file_size, duration, width, height,
               frame_rate, bit_rate, codec, created_time, modified_time,
               last_accessed, is_favorite, play_count, last_played,
               thumbnail_path, metadata_extracted
        FROM video_files WHERE id = ?
    )";

    sqlite3_stmt* stmt = m_dbManager->PrepareStatement(sql);
    if (!stmt) {
        return std::nullopt;
    }

    sqlite3_bind_int64(stmt, 1, id);

    std::optional<VideoFile> result = std::nullopt;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result = BuildVideoFileFromResult(stmt);
    }

    m_dbManager->FinalizeStatement(stmt);
    return result;
}

std::optional<VideoFile> VideoFileDAO::FindByPath(const std::wstring& filePath)
{
    if (!m_dbManager || !m_dbManager->IsConnected()) {
        return std::nullopt;
    }

    const std::string sql = R"(
        SELECT id, file_path, file_name, file_size, duration, width, height,
               frame_rate, bit_rate, codec, created_time, modified_time,
               last_accessed, is_favorite, play_count, last_played,
               thumbnail_path, metadata_extracted
        FROM video_files WHERE file_path = ?
    )";

    sqlite3_stmt* stmt = m_dbManager->PrepareStatement(sql);
    if (!stmt) {
        return std::nullopt;
    }

    std::string utf8Path = WStringToUTF8(filePath);
    sqlite3_bind_text(stmt, 1, utf8Path.c_str(), -1, SQLITE_STATIC);

    std::optional<VideoFile> result = std::nullopt;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result = BuildVideoFileFromResult(stmt);
    }

    m_dbManager->FinalizeStatement(stmt);
    return result;
}

bool VideoFileDAO::Update(const VideoFile& videoFile)
{
    if (!m_dbManager || !m_dbManager->IsConnected() || videoFile.id <= 0) {
        return false;
    }

    const std::string sql = R"(
        UPDATE video_files SET
            file_path = ?, file_name = ?, file_size = ?, duration = ?,
            width = ?, height = ?, frame_rate = ?, bit_rate = ?, codec = ?,
            modified_time = ?, last_accessed = ?, is_favorite = ?,
            play_count = ?, last_played = ?, thumbnail_path = ?,
            metadata_extracted = ?
        WHERE id = ?
    )";

    sqlite3_stmt* stmt = m_dbManager->PrepareStatement(sql);
    if (!stmt) {
        return false;
    }

    BindVideoFileParameters(stmt, videoFile, true);

    int result = sqlite3_step(stmt);
    m_dbManager->FinalizeStatement(stmt);

    return result == SQLITE_DONE;
}

bool VideoFileDAO::Delete(int64_t id)
{
    if (!m_dbManager || !m_dbManager->IsConnected()) {
        return false;
    }

    const std::string sql = "DELETE FROM video_files WHERE id = ?";
    
    sqlite3_stmt* stmt = m_dbManager->PrepareStatement(sql);
    if (!stmt) {
        return false;
    }

    sqlite3_bind_int64(stmt, 1, id);

    int result = sqlite3_step(stmt);
    m_dbManager->FinalizeStatement(stmt);

    return result == SQLITE_DONE;
}

bool VideoFileDAO::DeleteByPath(const std::wstring& filePath)
{
    if (!m_dbManager || !m_dbManager->IsConnected()) {
        return false;
    }

    const std::string sql = "DELETE FROM video_files WHERE file_path = ?";
    
    sqlite3_stmt* stmt = m_dbManager->PrepareStatement(sql);
    if (!stmt) {
        return false;
    }

    std::string utf8Path = WStringToUTF8(filePath);
    sqlite3_bind_text(stmt, 1, utf8Path.c_str(), -1, SQLITE_STATIC);

    int result = sqlite3_step(stmt);
    m_dbManager->FinalizeStatement(stmt);

    return result == SQLITE_DONE;
}

std::vector<VideoFile> VideoFileDAO::FindAll()
{
    std::vector<VideoFile> results;
    
    if (!m_dbManager || !m_dbManager->IsConnected()) {
        return results;
    }

    const std::string sql = R"(
        SELECT id, file_path, file_name, file_size, duration, width, height,
               frame_rate, bit_rate, codec, created_time, modified_time,
               last_accessed, is_favorite, play_count, last_played,
               thumbnail_path, metadata_extracted
        FROM video_files ORDER BY file_name
    )";

    sqlite3_stmt* stmt = m_dbManager->PrepareStatement(sql);
    if (!stmt) {
        return results;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        results.push_back(BuildVideoFileFromResult(stmt));
    }

    m_dbManager->FinalizeStatement(stmt);
    return results;
}

std::vector<VideoFile> VideoFileDAO::FindByNamePattern(const std::wstring& pattern)
{
    std::vector<VideoFile> results;
    
    if (!m_dbManager || !m_dbManager->IsConnected()) {
        return results;
    }

    const std::string sql = R"(
        SELECT id, file_path, file_name, file_size, duration, width, height,
               frame_rate, bit_rate, codec, created_time, modified_time,
               last_accessed, is_favorite, play_count, last_played,
               thumbnail_path, metadata_extracted
        FROM video_files WHERE file_name LIKE ? ORDER BY file_name
    )";

    sqlite3_stmt* stmt = m_dbManager->PrepareStatement(sql);
    if (!stmt) {
        return results;
    }

    std::string utf8Pattern = WStringToUTF8(pattern);
    sqlite3_bind_text(stmt, 1, utf8Pattern.c_str(), -1, SQLITE_STATIC);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        results.push_back(BuildVideoFileFromResult(stmt));
    }

    m_dbManager->FinalizeStatement(stmt);
    return results;
}

std::vector<VideoFile> VideoFileDAO::FindBySizeRange(int64_t minSize, int64_t maxSize)
{
    std::vector<VideoFile> results;
    
    if (!m_dbManager || !m_dbManager->IsConnected()) {
        return results;
    }

    const std::string sql = R"(
        SELECT id, file_path, file_name, file_size, duration, width, height,
               frame_rate, bit_rate, codec, created_time, modified_time,
               last_accessed, is_favorite, play_count, last_played,
               thumbnail_path, metadata_extracted
        FROM video_files WHERE file_size BETWEEN ? AND ? ORDER BY file_size
    )";

    sqlite3_stmt* stmt = m_dbManager->PrepareStatement(sql);
    if (!stmt) {
        return results;
    }

    sqlite3_bind_int64(stmt, 1, minSize);
    sqlite3_bind_int64(stmt, 2, maxSize);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        results.push_back(BuildVideoFileFromResult(stmt));
    }

    m_dbManager->FinalizeStatement(stmt);
    return results;
}

std::vector<VideoFile> VideoFileDAO::FindByDurationRange(int minDuration, int maxDuration)
{
    std::vector<VideoFile> results;
    
    if (!m_dbManager || !m_dbManager->IsConnected()) {
        return results;
    }

    const std::string sql = R"(
        SELECT id, file_path, file_name, file_size, duration, width, height,
               frame_rate, bit_rate, codec, created_time, modified_time,
               last_accessed, is_favorite, play_count, last_played,
               thumbnail_path, metadata_extracted
        FROM video_files WHERE duration BETWEEN ? AND ? ORDER BY duration
    )";

    sqlite3_stmt* stmt = m_dbManager->PrepareStatement(sql);
    if (!stmt) {
        return results;
    }

    sqlite3_bind_int(stmt, 1, minDuration);
    sqlite3_bind_int(stmt, 2, maxDuration);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        results.push_back(BuildVideoFileFromResult(stmt));
    }

    m_dbManager->FinalizeStatement(stmt);
    return results;
}

std::vector<VideoFile> VideoFileDAO::FindFavorites()
{
    std::vector<VideoFile> results;
    
    if (!m_dbManager || !m_dbManager->IsConnected()) {
        return results;
    }

    const std::string sql = R"(
        SELECT id, file_path, file_name, file_size, duration, width, height,
               frame_rate, bit_rate, codec, created_time, modified_time,
               last_accessed, is_favorite, play_count, last_played,
               thumbnail_path, metadata_extracted
        FROM video_files WHERE is_favorite = 1 ORDER BY file_name
    )";

    sqlite3_stmt* stmt = m_dbManager->PrepareStatement(sql);
    if (!stmt) {
        return results;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        results.push_back(BuildVideoFileFromResult(stmt));
    }

    m_dbManager->FinalizeStatement(stmt);
    return results;
}

std::vector<VideoFile> VideoFileDAO::FindRecentlyPlayed(int limit)
{
    std::vector<VideoFile> results;
    
    if (!m_dbManager || !m_dbManager->IsConnected()) {
        return results;
    }

    const std::string sql = R"(
        SELECT id, file_path, file_name, file_size, duration, width, height,
               frame_rate, bit_rate, codec, created_time, modified_time,
               last_accessed, is_favorite, play_count, last_played,
               thumbnail_path, metadata_extracted
        FROM video_files WHERE last_played IS NOT NULL 
        ORDER BY last_played DESC LIMIT ?
    )";

    sqlite3_stmt* stmt = m_dbManager->PrepareStatement(sql);
    if (!stmt) {
        return results;
    }

    sqlite3_bind_int(stmt, 1, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        results.push_back(BuildVideoFileFromResult(stmt));
    }

    m_dbManager->FinalizeStatement(stmt);
    return results;
}

std::vector<VideoFile> VideoFileDAO::FindMostPlayed(int limit)
{
    std::vector<VideoFile> results;
    
    if (!m_dbManager || !m_dbManager->IsConnected()) {
        return results;
    }

    const std::string sql = R"(
        SELECT id, file_path, file_name, file_size, duration, width, height,
               frame_rate, bit_rate, codec, created_time, modified_time,
               last_accessed, is_favorite, play_count, last_played,
               thumbnail_path, metadata_extracted
        FROM video_files WHERE play_count > 0 
        ORDER BY play_count DESC, last_played DESC LIMIT ?
    )";

    sqlite3_stmt* stmt = m_dbManager->PrepareStatement(sql);
    if (!stmt) {
        return results;
    }

    sqlite3_bind_int(stmt, 1, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        results.push_back(BuildVideoFileFromResult(stmt));
    }

    m_dbManager->FinalizeStatement(stmt);
    return results;
}

bool VideoFileDAO::SetFavorite(int64_t id, bool isFavorite)
{
    if (!m_dbManager || !m_dbManager->IsConnected()) {
        return false;
    }

    const std::string sql = "UPDATE video_files SET is_favorite = ? WHERE id = ?";
    
    sqlite3_stmt* stmt = m_dbManager->PrepareStatement(sql);
    if (!stmt) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, isFavorite ? 1 : 0);
    sqlite3_bind_int64(stmt, 2, id);

    int result = sqlite3_step(stmt);
    m_dbManager->FinalizeStatement(stmt);

    return result == SQLITE_DONE;
}

bool VideoFileDAO::IncrementPlayCount(int64_t id)
{
    if (!m_dbManager || !m_dbManager->IsConnected()) {
        return false;
    }

    const std::string sql = "UPDATE video_files SET play_count = play_count + 1 WHERE id = ?";
    
    sqlite3_stmt* stmt = m_dbManager->PrepareStatement(sql);
    if (!stmt) {
        return false;
    }

    sqlite3_bind_int64(stmt, 1, id);

    int result = sqlite3_step(stmt);
    m_dbManager->FinalizeStatement(stmt);

    return result == SQLITE_DONE;
}

bool VideoFileDAO::UpdateLastPlayed(int64_t id)
{
    if (!m_dbManager || !m_dbManager->IsConnected()) {
        return false;
    }

    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

    const std::string sql = "UPDATE video_files SET last_played = ? WHERE id = ?";
    
    sqlite3_stmt* stmt = m_dbManager->PrepareStatement(sql);
    if (!stmt) {
        return false;
    }

    sqlite3_bind_int64(stmt, 1, timestamp);
    sqlite3_bind_int64(stmt, 2, id);

    int result = sqlite3_step(stmt);
    m_dbManager->FinalizeStatement(stmt);

    return result == SQLITE_DONE;
}

int VideoFileDAO::BatchInsert(const std::vector<VideoFile>& videoFiles)
{
    if (!m_dbManager || !m_dbManager->IsConnected() || videoFiles.empty()) {
        return 0;
    }

    if (!m_dbManager->BeginTransaction()) {
        return 0;
    }

    int insertedCount = 0;
    const std::string sql = R"(
        INSERT INTO video_files (
            file_path, file_name, file_size, duration, width, height,
            frame_rate, bit_rate, codec, created_time, modified_time,
            last_accessed, is_favorite, play_count, last_played,
            thumbnail_path, metadata_extracted
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )";

    sqlite3_stmt* stmt = m_dbManager->PrepareStatement(sql);
    if (!stmt) {
        m_dbManager->RollbackTransaction();
        return 0;
    }

    for (const auto& videoFile : videoFiles) {
        sqlite3_reset(stmt);
        BindVideoFileParameters(stmt, videoFile, false);
        
        if (sqlite3_step(stmt) == SQLITE_DONE) {
            insertedCount++;
        }
    }

    m_dbManager->FinalizeStatement(stmt);

    if (insertedCount > 0) {
        m_dbManager->CommitTransaction();
    } else {
        m_dbManager->RollbackTransaction();
    }

    return insertedCount;
}

bool VideoFileDAO::ExistsByPath(const std::wstring& filePath)
{
    if (!m_dbManager || !m_dbManager->IsConnected()) {
        return false;
    }

    const std::string sql = "SELECT 1 FROM video_files WHERE file_path = ? LIMIT 1";
    
    sqlite3_stmt* stmt = m_dbManager->PrepareStatement(sql);
    if (!stmt) {
        return false;
    }

    std::string utf8Path = WStringToUTF8(filePath);
    sqlite3_bind_text(stmt, 1, utf8Path.c_str(), -1, SQLITE_STATIC);

    bool exists = (sqlite3_step(stmt) == SQLITE_ROW);
    m_dbManager->FinalizeStatement(stmt);

    return exists;
}

int64_t VideoFileDAO::GetTotalCount()
{
    if (!m_dbManager || !m_dbManager->IsConnected()) {
        return 0;
    }

    const std::string sql = "SELECT COUNT(*) FROM video_files";
    
    sqlite3_stmt* stmt = m_dbManager->PrepareStatement(sql);
    if (!stmt) {
        return 0;
    }

    int64_t count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int64(stmt, 0);
    }

    m_dbManager->FinalizeStatement(stmt);
    return count;
}

int64_t VideoFileDAO::GetTotalSize()
{
    if (!m_dbManager || !m_dbManager->IsConnected()) {
        return 0;
    }

    const std::string sql = "SELECT SUM(file_size) FROM video_files";
    
    sqlite3_stmt* stmt = m_dbManager->PrepareStatement(sql);
    if (!stmt) {
        return 0;
    }

    int64_t totalSize = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        totalSize = sqlite3_column_int64(stmt, 0);
    }

    m_dbManager->FinalizeStatement(stmt);
    return totalSize;
}

int VideoFileDAO::CleanupMissingFiles()
{
    if (!m_dbManager || !m_dbManager->IsConnected()) {
        return 0;
    }

    // 首先查询所有文件路径
    const std::string selectSql = "SELECT id, file_path FROM video_files";
    sqlite3_stmt* selectStmt = m_dbManager->PrepareStatement(selectSql);
    if (!selectStmt) {
        return 0;
    }

    std::vector<int64_t> missingFileIds;
    
    while (sqlite3_step(selectStmt) == SQLITE_ROW) {
        int64_t id = sqlite3_column_int64(selectStmt, 0);
        const char* pathStr = reinterpret_cast<const char*>(sqlite3_column_text(selectStmt, 1));
        
        if (pathStr) {
            std::wstring filePath = UTF8ToWString(pathStr);
            if (!std::filesystem::exists(filePath)) {
                missingFileIds.push_back(id);
            }
        }
    }

    m_dbManager->FinalizeStatement(selectStmt);

    // 删除不存在的文件记录
    if (!missingFileIds.empty()) {
        if (!m_dbManager->BeginTransaction()) {
            return 0;
        }

        const std::string deleteSql = "DELETE FROM video_files WHERE id = ?";
        sqlite3_stmt* deleteStmt = m_dbManager->PrepareStatement(deleteSql);
        if (!deleteStmt) {
            m_dbManager->RollbackTransaction();
            return 0;
        }

        int deletedCount = 0;
        for (int64_t id : missingFileIds) {
            sqlite3_reset(deleteStmt);
            sqlite3_bind_int64(deleteStmt, 1, id);
            
            if (sqlite3_step(deleteStmt) == SQLITE_DONE) {
                deletedCount++;
            }
        }

        m_dbManager->FinalizeStatement(deleteStmt);
        m_dbManager->CommitTransaction();
        
        return deletedCount;
    }

    return 0;
}

VideoFile VideoFileDAO::BuildVideoFileFromResult(sqlite3_stmt* stmt)
{
    VideoFile videoFile;
    
    videoFile.id = sqlite3_column_int64(stmt, 0);
    
    const char* pathStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    if (pathStr) {
        videoFile.filePath = UTF8ToWString(pathStr);
    }
    
    const char* nameStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    if (nameStr) {
        videoFile.fileName = nameStr;
    }
    
    videoFile.fileSize = sqlite3_column_int64(stmt, 3);
    videoFile.duration = sqlite3_column_int(stmt, 4);
    videoFile.width = sqlite3_column_int(stmt, 5);
    videoFile.height = sqlite3_column_int(stmt, 6);
    videoFile.frameRate = sqlite3_column_double(stmt, 7);
    videoFile.bitRate = sqlite3_column_int64(stmt, 8);
    
    const char* codecStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
    if (codecStr) {
        videoFile.codec = codecStr;
    }
    
    // 时间戳转换
    int64_t createdTimestamp = sqlite3_column_int64(stmt, 10);
    int64_t modifiedTimestamp = sqlite3_column_int64(stmt, 11);
    int64_t accessedTimestamp = sqlite3_column_int64(stmt, 12);
    
    videoFile.createdTime = std::chrono::system_clock::from_time_t(createdTimestamp);
    // 使用简单的时间戳转换，避免复杂的文件时间类型转换
    // 直接使用文件系统时间类型的默认构造
    videoFile.lastModified = std::filesystem::file_time_type{} + std::chrono::seconds(modifiedTimestamp);
    videoFile.lastAccessed = std::chrono::system_clock::from_time_t(accessedTimestamp);
    
    videoFile.isFavorite = sqlite3_column_int(stmt, 13) != 0;
    videoFile.playCount = sqlite3_column_int(stmt, 14);
    
    int64_t lastPlayedTimestamp = sqlite3_column_int64(stmt, 15);
    if (lastPlayedTimestamp > 0) {
        videoFile.lastPlayed = std::chrono::system_clock::from_time_t(lastPlayedTimestamp);
    }
    
    const char* thumbnailStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 16));
    if (thumbnailStr) {
        videoFile.thumbnailPath = UTF8ToWString(thumbnailStr);
        videoFile.hasThumbnail = !videoFile.thumbnailPath.empty();
    }
    
    videoFile.metadataExtracted = sqlite3_column_int(stmt, 17) != 0;
    
    // 设置文件扩展名
    if (!videoFile.filePath.empty()) {
        videoFile.extension = videoFile.filePath.extension().string();
    }
    
    return videoFile;
}

void VideoFileDAO::BindVideoFileParameters(sqlite3_stmt* stmt, const VideoFile& videoFile, bool includeId)
{
    int paramIndex = 1;
    
    // 绑定基本文件信息
    std::string utf8Path = WStringToUTF8(videoFile.filePath.wstring());
    sqlite3_bind_text(stmt, paramIndex++, utf8Path.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, paramIndex++, videoFile.fileName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, paramIndex++, static_cast<int64_t>(videoFile.fileSize));
    
    // 绑定视频信息
    sqlite3_bind_int(stmt, paramIndex++, videoFile.duration);
    sqlite3_bind_int(stmt, paramIndex++, videoFile.width);
    sqlite3_bind_int(stmt, paramIndex++, videoFile.height);
    sqlite3_bind_double(stmt, paramIndex++, videoFile.frameRate);
    sqlite3_bind_int64(stmt, paramIndex++, videoFile.bitRate);
    sqlite3_bind_text(stmt, paramIndex++, videoFile.codec.c_str(), -1, SQLITE_TRANSIENT);
    
    // 绑定时间戳
    auto createdTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
        videoFile.createdTime.time_since_epoch()).count();
    sqlite3_bind_int64(stmt, paramIndex++, createdTimestamp);
    
    auto modifiedTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
        videoFile.lastModified.time_since_epoch()).count();
    sqlite3_bind_int64(stmt, paramIndex++, modifiedTimestamp);
    
    auto accessedTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
        videoFile.lastAccessed.time_since_epoch()).count();
    sqlite3_bind_int64(stmt, paramIndex++, accessedTimestamp);
    
    // 绑定用户数据
    sqlite3_bind_int(stmt, paramIndex++, videoFile.isFavorite ? 1 : 0);
    sqlite3_bind_int(stmt, paramIndex++, videoFile.playCount);
    
    // 最后播放时间（可能为空）
    if (videoFile.playCount > 0) {
        auto lastPlayedTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
            videoFile.lastPlayed.time_since_epoch()).count();
        sqlite3_bind_int64(stmt, paramIndex++, lastPlayedTimestamp);
    } else {
        sqlite3_bind_null(stmt, paramIndex++);
    }
    
    // 缩略图路径
    if (!videoFile.thumbnailPath.empty()) {
        std::string utf8ThumbnailPath = WStringToUTF8(videoFile.thumbnailPath.wstring());
        sqlite3_bind_text(stmt, paramIndex++, utf8ThumbnailPath.c_str(), -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt, paramIndex++);
    }
    
    sqlite3_bind_int(stmt, paramIndex++, videoFile.metadataExtracted ? 1 : 0);
    
    // 如果是更新操作，绑定ID
    if (includeId) {
        sqlite3_bind_int64(stmt, paramIndex++, videoFile.id);
    }
}

std::string VideoFileDAO::WStringToUTF8(const std::wstring& wstr) const
{
    if (wstr.empty()) {
        return std::string();
    }

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

std::wstring VideoFileDAO::UTF8ToWString(const std::string& str) const
{
    if (str.empty()) {
        return std::wstring();
    }

    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}