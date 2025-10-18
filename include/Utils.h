#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <windows.h>

namespace VideoViewer {
namespace Utils {

/**
 * @brief 支持的视频文件扩展名
 */
extern const std::vector<std::string> VIDEO_EXTENSIONS;

/**
 * @brief 字符串转换函数
 */
std::wstring StringToWString(const std::string& str);
std::string WStringToString(const std::wstring& wstr);

/**
 * @brief 文件大小格式化
 * @param bytes 字节数
 * @return 格式化的大小字符串（如 "1.5 MB"）
 */
std::string FormatFileSize(std::uintmax_t bytes);

/**
 * @brief 时间格式化
 * @param fileTime 文件时间
 * @return 格式化的时间字符串
 */
std::string FormatFileTime(const std::filesystem::file_time_type& fileTime);

/**
 * @brief 检查文件是否为视频文件
 * @param filePath 文件路径
 * @return 如果是视频文件返回 true
 */
bool IsVideoFile(const std::filesystem::path& filePath);

/**
 * @brief 获取文件扩展名（小写）
 * @param filePath 文件路径
 * @return 小写的扩展名
 */
std::string GetLowerExtension(const std::filesystem::path& filePath);

/**
 * @brief 创建目录（如果不存在）
 * @param dirPath 目录路径
 * @return 成功返回 true
 */
bool CreateDirectoryIfNotExists(const std::filesystem::path& dirPath);

/**
 * @brief 获取应用程序目录
 * @return 应用程序所在目录
 */
std::filesystem::path GetAppDirectory();

/**
 * @brief 获取用户文档目录
 * @return 用户文档目录
 */
std::filesystem::path GetDocumentsDirectory();

/**
 * @brief 获取本地应用数据目录
 * @return 本地应用数据目录
 */
std::filesystem::path GetLocalAppDataDirectory();

/**
 * @brief Windows 错误码转字符串
 * @param errorCode Windows 错误码
 * @return 错误描述字符串
 */
std::string GetWindowsErrorString(DWORD errorCode);

/**
 * @brief 显示错误消息框
 * @param hwnd 父窗口句柄
 * @param message 错误消息
 * @param title 标题（可选）
 */
void ShowErrorMessage(HWND hwnd, const std::string& message, const std::string& title = "错误");

/**
 * @brief 显示信息消息框
 * @param hwnd 父窗口句柄
 * @param message 信息消息
 * @param title 标题（可选）
 */
void ShowInfoMessage(HWND hwnd, const std::string& message, const std::string& title = "信息");

/**
 * @brief 显示确认对话框
 * @param hwnd 父窗口句柄
 * @param message 确认消息
 * @param title 标题（可选）
 * @return 用户点击确定返回 true
 */
bool ShowConfirmDialog(HWND hwnd, const std::string& message, const std::string& title = "确认");

/**
 * @brief 显示确认对话框（Unicode版本）
 * @param hwnd 父窗口句柄
 * @param message 消息内容（宽字符）
 * @param title 对话框标题（宽字符）
 * @return 用户点击"是"返回 true，否则返回 false
 */
bool ShowConfirmDialog(HWND hwnd, const std::wstring& message, const std::wstring& title = L"确认");

/**
 * @brief 选择文件夹对话框
 * @param hwnd 父窗口句柄
 * @param title 对话框标题
 * @return 选择的文件夹路径，取消则返回空路径
 */
std::filesystem::path SelectFolderDialog(HWND hwnd, const std::string& title = "选择文件夹");

/**
 * @brief 计算字符串哈希值（用于缓存文件名）
 * @param str 输入字符串
 * @return 哈希值
 */
std::size_t HashString(const std::string& str);

/**
 * @brief 安全删除文件到回收站
 * @param filePath 文件路径
 * @return 成功返回 true
 */
bool MoveToRecycleBin(const std::filesystem::path& filePath);

/**
 * @brief 使用默认程序打开文件
 * @param filePath 文件路径
 * @return 成功返回 true
 */
bool OpenWithDefaultProgram(const std::filesystem::path& filePath);

/**
 * @brief 使用指定程序打开文件
 * @param filePath 文件路径
 * @param programPath 程序路径
 * @return 成功返回 true
 */
bool OpenWithProgram(const std::filesystem::path& filePath, const std::filesystem::path& programPath);

/**
 * @brief 选择单个视频文件对话框
 * @param hwnd 父窗口句柄
 * @param title 对话框标题
 * @return 选择的文件路径，取消则返回空路径
 */
std::filesystem::path SelectSingleVideoFileDialog(HWND hwnd, const std::string& title = "选择视频文件");

/**
 * @brief 选择多个视频文件对话框
 * @param hwnd 父窗口句柄
 * @param title 对话框标题
 * @return 选择的文件路径列表，取消则返回空列表
 */
std::vector<std::filesystem::path> SelectMultipleVideoFilesDialog(HWND hwnd, const std::string& title = "选择视频文件");

} // namespace Utils
} // namespace VideoViewer