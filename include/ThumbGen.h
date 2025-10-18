#pragma once

#include "VideoFile.h"
#include <filesystem>
#include <functional>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <set>

// Windows Media Foundation 前向声明
struct IMFSourceResolver;
struct IWICImagingFactory;
struct IWICBitmap;

namespace VideoViewer {

/**
 * @brief 缩略图回调函数类型
 * @param success 是否成功
 * @param thumbnailPath 缩略图路径
 * @param errorMessage 错误信息（如果有）
 */
using ThumbnailCallback = std::function<void(bool success, 
                                           const std::filesystem::path& thumbnailPath,
                                           const std::string& errorMessage)>;

/**
 * @brief 缩略图配置
 */
struct ThumbnailConfig {
    int width = 200;                    // 缩略图宽度
    int height = 150;                   // 缩略图高度
    int quality = 85;                   // JPEG质量 (1-100)
    double timePosition = 0.3;          // 提取时间位置 (0.0-1.0)
    bool smartFrameSelection = true;    // 是否启用智能帧选择
    int maxSampleFrames = 5;           // 智能选择时的最大采样帧数
    double blackFrameThreshold = 0.8;  // 黑帧检测阈值
    
    ThumbnailConfig() = default;
    
    ThumbnailConfig(int w, int h, int q = 85, double pos = 0.3) 
        : width(w), height(h), quality(q), timePosition(pos) {}
};

/**
 * @brief 缩略图生成器
 * 
 * 负责从视频文件生成缩略图，支持同步和异步操作。
 * 使用 Windows Media Foundation 和 WIC 进行视频解码和图像处理。
 */
class ThumbGen {
public:
    /**
     * @brief 统计信息结构
     */
    struct Statistics {
        size_t totalGenerated = 0;      // 总生成数量
        size_t successCount = 0;        // 成功数量
        size_t failureCount = 0;        // 失败数量
        double averageGenerationTime = 0.0; // 平均生成时间（毫秒）
        size_t cacheHits = 0;          // 缓存命中次数
    };

public:
    /**
     * @brief 构造函数
     */
    ThumbGen();

    /**
     * @brief 析构函数
     */
    ~ThumbGen();

    /**
     * @brief 初始化缩略图生成器
     * @return 成功返回true
     */
    bool initialize();

    /**
     * @brief 清理资源
     */
    void cleanup();

    /**
     * @brief 同步生成缩略图
     * @param videoFile 视频文件信息
     * @param outputPath 输出图片路径
     * @param config 缩略图配置
     * @return 成功返回true
     */
    bool generateThumbnail(const VideoFile& videoFile,
                          const std::filesystem::path& outputPath,
                          const ThumbnailConfig& config = ThumbnailConfig());

    /**
     * @brief 异步生成缩略图
     * @param videoFile 视频文件信息
     * @param outputPath 输出图片路径
     * @param callback 完成回调函数
     * @param config 缩略图配置
     */
    void generateThumbnailAsync(const VideoFile& videoFile,
                               const std::filesystem::path& outputPath,
                               ThumbnailCallback callback,
                               const ThumbnailConfig& config = ThumbnailConfig());

    /**
     * @brief 批量生成缩略图
     * @param videoFiles 视频文件列表
     * @param outputDirectory 输出目录
     * @param callback 每个文件完成时的回调
     * @param config 缩略图配置
     */
    void generateBatchThumbnails(const std::vector<VideoFile>& videoFiles,
                                const std::filesystem::path& outputDirectory,
                                ThumbnailCallback callback,
                                const ThumbnailConfig& config = ThumbnailConfig());

    /**
     * @brief 检查缩略图是否有效
     * @param thumbnailPath 缩略图路径
     * @return 有效返回true
     */
    bool isThumbnailValid(const std::filesystem::path& thumbnailPath);

    /**
     * @brief 获取视频文件对应的缩略图路径
     * @param videoFile 视频文件信息
     * @return 缩略图路径
     */
    std::filesystem::path getThumbnailPath(const VideoFile& videoFile) const;

    /**
     * @brief 设置缓存目录
     * @param directory 缓存目录路径
     */
    void setCacheDirectory(const std::filesystem::path& directory);

    /**
     * @brief 获取缓存目录
     * @return 缓存目录路径
     */
    std::filesystem::path getCacheDirectory() const;

    /**
     * @brief 检查是否支持该视频格式
     * @param videoPath 视频文件路径
     * @return 支持返回true
     */
    bool isSupportedFormat(const std::filesystem::path& videoPath);

    /**
     * @brief 停止所有待处理的任务
     */
    void stopAllTasks();

    /**
     * @brief 获取当前队列中的任务数量
     * @return 任务数量
     */
    size_t getQueueSize() const;

    /**
     * @brief 获取统计信息
     * @return 统计信息
     */
    Statistics getStatistics() const;

private:
    /**
     * @brief 在指定时间位置提取视频帧
     * @param videoPath 视频文件路径
     * @param timePosition 时间位置（0.0-1.0）
     * @param outputPath 输出图片路径
     * @param config 缩略图配置
     * @return 成功返回true
     */
    bool extractFrameAtTime(const std::filesystem::path& videoPath,
                           double timePosition,
                           const std::filesystem::path& outputPath,
                           const ThumbnailConfig& config);

    /**
     * @brief 保存位图为JPEG文件
     * @param bitmap 位图数据
     * @param outputPath 输出路径
     * @param quality JPEG质量
     * @return 成功返回true
     */
    bool saveBitmapAsJPEG(IWICBitmap* bitmap,
                         const std::filesystem::path& outputPath,
                         int quality);

    /**
     * @brief 工作线程函数
     */
    void workerThread();

    /**
     * @brief 智能帧选择 - 从多个时间点中选择最佳帧
     * @param videoPath 视频文件路径
     * @param outputPath 输出图片路径
     * @param config 缩略图配置
     * @return 成功返回true
     */
    bool extractBestFrame(const std::filesystem::path& videoPath,
                         const std::filesystem::path& outputPath,
                         const ThumbnailConfig& config);

    /**
     * @brief 检测帧是否为黑帧
     * @param bitmap 位图数据
     * @param threshold 黑帧检测阈值
     * @return 是黑帧返回true
     */
    bool isBlackFrame(IWICBitmap* bitmap, double threshold);

    /**
     * @brief 计算帧的亮度值
     * @param bitmap 位图数据
     * @return 亮度值（0.0-1.0）
     */
    double calculateFrameBrightness(IWICBitmap* bitmap);

    /**
     * @brief 异步任务结构
     */
    struct ThumbnailTask {
        VideoFile videoFile;
        std::filesystem::path thumbnailPath;
        ThumbnailCallback callback;
        ThumbnailConfig config;
    };

private:
    bool m_initialized;                           // 是否已初始化
    std::filesystem::path m_cacheDirectory;      // 缓存目录
    
    // Media Foundation相关
    IMFSourceResolver* m_sourceResolver;         // 源解析器
    IWICImagingFactory* m_imagingFactory;        // WIC图像工厂
    
    // 异步处理相关
    std::thread m_workerThread;                  // 工作线程
    std::queue<ThumbnailTask> m_taskQueue;       // 任务队列
    mutable std::mutex m_queueMutex;             // 队列互斥锁
    std::condition_variable m_queueCondition;    // 队列条件变量
    bool m_stopRequested;                        // 停止请求标志
    
    // 统计信息
    mutable std::mutex m_statsMutex;             // 统计互斥锁
    size_t m_totalGenerated;                     // 总生成数量
    size_t m_totalFailed;                        // 总失败数量
};

} // namespace VideoViewer