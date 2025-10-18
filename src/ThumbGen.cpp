#include "../include/ThumbGen.h"
#include "../include/Utils.h"
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <wincodec.h>
#include <propvarutil.h>
#include <wrl/client.h>
#include <iostream>
#include <algorithm>
#include <thread>
#include <chrono>

#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "propsys.lib")

namespace VideoViewer {

ThumbGen::ThumbGen() 
    : m_initialized(false)
    , m_sourceResolver(nullptr)
    , m_imagingFactory(nullptr)
    , m_stopRequested(false)
    , m_totalGenerated(0)
    , m_totalFailed(0) {
    printf("ThumbGen 构造函数\n");
}

ThumbGen::~ThumbGen() {
    printf("ThumbGen 析构函数\n");
    cleanup();
}

bool ThumbGen::initialize() {
    printf("初始化 ThumbGen...\n");
    
    if (m_initialized) {
        printf("ThumbGen 已经初始化\n");
        return true;
    }

    HRESULT hr = S_OK;

    // 初始化 Media Foundation
    hr = MFStartup(MF_VERSION, MFSTARTUP_LITE);
    if (FAILED(hr)) {
        printf("初始化 Media Foundation 失败: 0x%08lX\n", hr);
        return false;
    }
    printf("Media Foundation 初始化成功\n");

    // 初始化 COM
    hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        printf("初始化 COM 失败: 0x%08lX\n", hr);
        MFShutdown();
        return false;
    }
    printf("COM 初始化成功\n");

    // 创建 WIC 图像工厂
    hr = CoCreateInstance(CLSID_WICImagingFactory,
                         nullptr,
                         CLSCTX_INPROC_SERVER,
                         IID_IWICImagingFactory,
                         reinterpret_cast<void**>(&m_imagingFactory));
    if (FAILED(hr)) {
        printf("创建 WIC 图像工厂失败: 0x%08lX\n", hr);
        CoUninitialize();
        MFShutdown();
        return false;
    }
    printf("WIC 图像工厂创建成功\n");

    // 创建源解析器
    hr = MFCreateSourceResolver(&m_sourceResolver);
    if (FAILED(hr)) {
        printf("创建源解析器失败: 0x%08lX\n", hr);
        m_imagingFactory->Release();
        m_imagingFactory = nullptr;
        CoUninitialize();
        MFShutdown();
        return false;
    }
    printf("源解析器创建成功\n");

    // 设置默认缓存目录
    std::filesystem::path tempDir = std::filesystem::temp_directory_path();
    m_cacheDirectory = tempDir / "VideoViewer" / "Thumbnails";
    
    try {
        std::filesystem::create_directories(m_cacheDirectory);
        printf("缓存目录创建成功: %s\n", m_cacheDirectory.string().c_str());
    } catch (const std::exception& e) {
        printf("创建缓存目录失败: %s\n", e.what());
        m_sourceResolver->Release();
        m_sourceResolver = nullptr;
        m_imagingFactory->Release();
        m_imagingFactory = nullptr;
        CoUninitialize();
        MFShutdown();
        return false;
    }

    // 启动工作线程
    m_stopRequested = false;
    m_workerThread = std::thread(&ThumbGen::workerThread, this);
    printf("工作线程启动成功\n");

    m_initialized = true;
    printf("ThumbGen 初始化完成\n");
    return true;
}

void ThumbGen::cleanup() {
    printf("清理 ThumbGen...\n");
    
    if (!m_initialized) {
        return;
    }

    // 停止工作线程
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_stopRequested = true;
    }
    m_queueCondition.notify_all();
    
    if (m_workerThread.joinable()) {
        m_workerThread.join();
        printf("工作线程已停止\n");
    }

    // 清理 COM 对象
    if (m_sourceResolver) {
        m_sourceResolver->Release();
        m_sourceResolver = nullptr;
        printf("源解析器已释放\n");
    }

    if (m_imagingFactory) {
        m_imagingFactory->Release();
        m_imagingFactory = nullptr;
        printf("WIC 图像工厂已释放\n");
    }

    // 清理 Media Foundation
    MFShutdown();
    printf("Media Foundation 已关闭\n");

    CoUninitialize();
    printf("COM 已清理\n");

    m_initialized = false;
    printf("ThumbGen 清理完成\n");
}

bool ThumbGen::generateThumbnail(const VideoFile& videoFile,
                                const std::filesystem::path& outputPath,
                                const ThumbnailConfig& config) {
    if (!m_initialized) {
        printf("ThumbGen 未初始化\n");
        return false;
    }

    printf("生成缩略图: %s -> %s\n", 
           videoFile.filePath.string().c_str(), 
           outputPath.string().c_str());

    // 检查视频文件是否存在
    if (!std::filesystem::exists(videoFile.filePath)) {
        printf("视频文件不存在: %s\n", videoFile.filePath.string().c_str());
        std::lock_guard<std::mutex> lock(m_statsMutex);
        m_totalFailed++;
        return false;
    }

    // 创建输出目录
    try {
        std::filesystem::create_directories(outputPath.parent_path());
    } catch (const std::exception& e) {
        printf("创建输出目录失败: %s\n", e.what());
        std::lock_guard<std::mutex> lock(m_statsMutex);
        m_totalFailed++;
        return false;
    }

    bool success = false;
    
    // 根据配置选择提取方法
    if (config.smartFrameSelection) {
        success = extractBestFrame(videoFile.filePath, outputPath, config);
    } else {
        success = extractFrameAtTime(videoFile.filePath, config.timePosition, outputPath, config);
    }

    // 更新统计信息
    {
        std::lock_guard<std::mutex> lock(m_statsMutex);
        if (success) {
            m_totalGenerated++;
            printf("缩略图生成成功\n");
        } else {
            m_totalFailed++;
            printf("缩略图生成失败\n");
        }
    }

    return success;
}

void ThumbGen::generateThumbnailAsync(const VideoFile& videoFile,
                                     const std::filesystem::path& outputPath,
                                     ThumbnailCallback callback,
                                     const ThumbnailConfig& config) {
    if (!m_initialized) {
        if (callback) {
            callback(false, outputPath, "ThumbGen 未初始化");
        }
        return;
    }

    ThumbnailTask task;
    task.videoFile = videoFile;
    task.thumbnailPath = outputPath;
    task.callback = callback;
    task.config = config;

    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_taskQueue.push(task);
    }
    m_queueCondition.notify_one();
}

void ThumbGen::generateBatchThumbnails(const std::vector<VideoFile>& videoFiles,
                                      const std::filesystem::path& outputDirectory,
                                      ThumbnailCallback callback,
                                      const ThumbnailConfig& config) {
    if (!m_initialized) {
        printf("ThumbGen 未初始化\n");
        return;
    }

    printf("批量生成缩略图，共 %zu 个文件\n", videoFiles.size());

    for (const auto& videoFile : videoFiles) {
        // 生成输出文件名
        std::string filename = videoFile.filePath.stem().string() + ".jpg";
        std::filesystem::path outputPath = outputDirectory / filename;
        
        generateThumbnailAsync(videoFile, outputPath, callback, config);
    }
}

bool ThumbGen::isThumbnailValid(const std::filesystem::path& thumbnailPath) {
    if (!std::filesystem::exists(thumbnailPath)) {
        return false;
    }

    try {
        auto fileSize = std::filesystem::file_size(thumbnailPath);
        // 检查文件大小是否合理（至少1KB）
        if (fileSize < 1024) {
            return false;
        }

        // 可以添加更多验证逻辑，比如检查文件头等
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

std::filesystem::path ThumbGen::getThumbnailPath(const VideoFile& videoFile) const {
    // 使用文件路径的哈希值作为缩略图文件名
    std::string pathStr = videoFile.filePath.string();
    std::hash<std::string> hasher;
    size_t hashValue = hasher(pathStr);
    
    std::string filename = std::to_string(hashValue) + ".jpg";
    return m_cacheDirectory / filename;
}

void ThumbGen::setCacheDirectory(const std::filesystem::path& directory) {
    m_cacheDirectory = directory;
    
    try {
        std::filesystem::create_directories(m_cacheDirectory);
        printf("缓存目录设置为: %s\n", m_cacheDirectory.string().c_str());
    } catch (const std::exception& e) {
        printf("设置缓存目录失败: %s\n", e.what());
    }
}

std::filesystem::path ThumbGen::getCacheDirectory() const {
    return m_cacheDirectory;
}

bool ThumbGen::isSupportedFormat(const std::filesystem::path& videoPath) {
    std::string extension = videoPath.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    // 支持的视频格式
    static const std::set<std::string> supportedFormats = {
        ".mp4", ".avi", ".mkv", ".mov", ".wmv", ".flv", ".webm", ".m4v",
        ".3gp", ".asf", ".rm", ".rmvb", ".vob", ".ts", ".mts", ".m2ts"
    };
    
    return supportedFormats.find(extension) != supportedFormats.end();
}

void ThumbGen::stopAllTasks() {
    printf("停止所有缩略图生成任务\n");
    
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        // 清空任务队列
        while (!m_taskQueue.empty()) {
            m_taskQueue.pop();
        }
    }
    m_queueCondition.notify_all();
}

size_t ThumbGen::getQueueSize() const {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    return m_taskQueue.size();
}

ThumbGen::Statistics ThumbGen::getStatistics() const {
    std::lock_guard<std::mutex> lock(m_statsMutex);
    Statistics stats;
    stats.totalGenerated = m_totalGenerated;
    stats.successCount = m_totalGenerated;
    stats.failureCount = m_totalFailed;
    stats.averageGenerationTime = 0; // 暂时不计算平均时间
    stats.cacheHits = 0; // 暂时不计算缓存命中
    return stats;
}

bool ThumbGen::extractFrameAtTime(const std::filesystem::path& videoPath,
                                 double timePosition,
                                 const std::filesystem::path& outputPath,
                                 const ThumbnailConfig& config) {
    HRESULT hr = S_OK;
    IMFSourceReader* sourceReader = nullptr;
    IMFMediaType* mediaType = nullptr;
    IMFSample* sample = nullptr;
    IMFMediaBuffer* buffer = nullptr;
    IWICBitmap* bitmap = nullptr;

    try {
        // 创建源读取器属性
        IMFAttributes* attributes = nullptr;
        hr = MFCreateAttributes(&attributes, 1);
        if (FAILED(hr)) {
            printf("创建属性失败: 0x%08lX\n", hr);
            return false;
        }

        // 启用视频处理以支持格式转换
        hr = attributes->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);
        if (FAILED(hr)) {
            printf("设置视频处理属性失败: 0x%08lX\n", hr);
            attributes->Release();
            return false;
        }

        // 创建源读取器
        std::wstring wVideoPath = videoPath.wstring();
        hr = MFCreateSourceReaderFromURL(wVideoPath.c_str(), attributes, &sourceReader);
        attributes->Release(); // 释放属性对象
        if (FAILED(hr)) {
            printf("创建源读取器失败: 0x%08lX\n", hr);
            return false;
        }

        // 配置媒体类型为RGB32
        hr = MFCreateMediaType(&mediaType);
        if (FAILED(hr)) {
            printf("创建媒体类型失败: 0x%08lX\n", hr);
            return false;
        }

        hr = mediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
        hr = mediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
        
        if (FAILED(hr)) {
            printf("设置媒体类型失败: 0x%08lX\n", hr);
            return false;
        }

        hr = sourceReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, mediaType);
        if (FAILED(hr)) {
            printf("设置当前媒体类型失败: 0x%08lX\n", hr);
            return false;
        }

        // 获取视频时长并计算目标时间
        PROPVARIANT var;
        PropVariantInit(&var);
        hr = sourceReader->GetPresentationAttribute(MF_SOURCE_READER_MEDIASOURCE,
                                                   MF_PD_DURATION, &var);
        
        LONGLONG duration = 0;
        if (SUCCEEDED(hr) && var.vt == VT_UI8) {
            duration = var.uhVal.QuadPart;
        }
        PropVariantClear(&var);

        // 计算目标时间位置
        LONGLONG targetTime = static_cast<LONGLONG>(duration * timePosition);
        
        // 定位到目标时间
        PROPVARIANT seekVar;
        PropVariantInit(&seekVar);
        seekVar.vt = VT_I8;
        seekVar.hVal.QuadPart = targetTime;
        
        hr = sourceReader->SetCurrentPosition(GUID_NULL, seekVar);
        PropVariantClear(&seekVar);
        
        if (FAILED(hr)) {
            printf("定位到目标时间失败: 0x%08lX\n", hr);
            // 继续尝试读取第一帧
        }

        // 读取视频帧
        DWORD streamIndex, flags;
        LONGLONG timestamp;
        
        hr = sourceReader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM,
                                     0, &streamIndex, &flags, &timestamp, &sample);
        
        if (FAILED(hr) || !sample) {
            printf("读取视频帧失败: 0x%08lX\n", hr);
            return false;
        }

        // 获取媒体缓冲区
        hr = sample->ConvertToContiguousBuffer(&buffer);
        if (FAILED(hr)) {
            printf("转换为连续缓冲区失败: 0x%08lX\n", hr);
            return false;
        }

        // 获取缓冲区数据
        BYTE* data = nullptr;
        DWORD dataLength = 0;
        hr = buffer->Lock(&data, nullptr, &dataLength);
        if (FAILED(hr)) {
            printf("锁定缓冲区失败: 0x%08lX\n", hr);
            return false;
        }

        // 获取当前媒体类型以获取视频尺寸
        IMFMediaType* currentMediaType = nullptr;
        hr = sourceReader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, &currentMediaType);
        if (FAILED(hr)) {
            printf("获取当前媒体类型失败: 0x%08lX\n", hr);
            buffer->Unlock();
            return false;
        }

        // 获取视频尺寸
        UINT32 width = 0, height = 0;
        hr = MFGetAttributeSize(currentMediaType, MF_MT_FRAME_SIZE, &width, &height);
        currentMediaType->Release();
        if (FAILED(hr)) {
            printf("获取帧尺寸失败: 0x%08lX\n", hr);
            buffer->Unlock();
            return false;
        }

        // 创建WIC位图
        hr = m_imagingFactory->CreateBitmapFromMemory(width, height,
                                                     GUID_WICPixelFormat32bppBGRA,
                                                     width * 4, dataLength, data, &bitmap);
        
        buffer->Unlock();
        
        if (FAILED(hr)) {
            printf("创建WIC位图失败: 0x%08lX\n", hr);
            return false;
        }

        // 如果需要调整尺寸
        IWICBitmap* resizedBitmap = nullptr;
        if (width != config.width || height != config.height) {
            IWICBitmapScaler* scaler = nullptr;
            hr = m_imagingFactory->CreateBitmapScaler(&scaler);
            if (SUCCEEDED(hr)) {
                hr = scaler->Initialize(bitmap, config.width, config.height,
                                      WICBitmapInterpolationModeFant);
                if (SUCCEEDED(hr)) {
                    hr = m_imagingFactory->CreateBitmapFromSource(scaler, WICBitmapCacheOnDemand, &resizedBitmap);
                }
                scaler->Release();
            }
            
            if (SUCCEEDED(hr) && resizedBitmap) {
                bitmap->Release();
                bitmap = resizedBitmap;
            }
        }

        // 保存为JPEG
        bool saveSuccess = saveBitmapAsJPEG(bitmap, outputPath, config.quality);
        
        return saveSuccess;

    } catch (const std::exception& e) {
        printf("提取视频帧异常: %s\n", e.what());
        return false;
    } catch (...) {
        printf("提取视频帧发生未知异常\n");
        return false;
    }

    // 清理资源
    if (bitmap) bitmap->Release();
    if (buffer) buffer->Release();
    if (sample) sample->Release();
    if (mediaType) mediaType->Release();
    if (sourceReader) sourceReader->Release();

    return false;
}

bool ThumbGen::saveBitmapAsJPEG(IWICBitmap* bitmap,
                               const std::filesystem::path& outputPath,
                               int quality) {
    HRESULT hr = S_OK;
    IWICStream* stream = nullptr;
    IWICBitmapEncoder* encoder = nullptr;
    IWICBitmapFrameEncode* frameEncode = nullptr;
    IPropertyBag2* propertyBag = nullptr;

    try {
        // 创建文件流
        hr = m_imagingFactory->CreateStream(&stream);
        if (FAILED(hr)) {
            printf("创建文件流失败: 0x%08lX\n", hr);
            return false;
        }

        std::wstring wOutputPath = outputPath.wstring();
        hr = stream->InitializeFromFilename(wOutputPath.c_str(), GENERIC_WRITE);
        if (FAILED(hr)) {
            printf("初始化文件流失败: 0x%08lX\n", hr);
            return false;
        }

        // 创建JPEG编码器
        hr = m_imagingFactory->CreateEncoder(GUID_ContainerFormatJpeg, nullptr, &encoder);
        if (FAILED(hr)) {
            printf("创建JPEG编码器失败: 0x%08lX\n", hr);
            return false;
        }

        hr = encoder->Initialize(stream, WICBitmapEncoderNoCache);
        if (FAILED(hr)) {
            printf("初始化编码器失败: 0x%08lX\n", hr);
            return false;
        }

        // 创建帧编码器
        hr = encoder->CreateNewFrame(&frameEncode, &propertyBag);
        if (FAILED(hr)) {
            printf("创建帧编码器失败: 0x%08lX\n", hr);
            return false;
        }

        // 设置JPEG质量
        VARIANT qualityValue;
        VariantInit(&qualityValue);
        qualityValue.vt = VT_R4;
        qualityValue.fltVal = quality / 100.0f;
        
        PROPBAG2 propBag = {};
        propBag.pstrName = const_cast<LPOLESTR>(L"ImageQuality");
        hr = propertyBag->Write(1, &propBag, &qualityValue);
        VariantClear(&qualityValue);

        hr = frameEncode->Initialize(propertyBag);
        if (FAILED(hr)) {
            printf("初始化帧编码器失败: 0x%08lX\n", hr);
            return false;
        }

        // 写入位图
        hr = frameEncode->WriteSource(bitmap, nullptr);
        if (FAILED(hr)) {
            printf("写入位图失败: 0x%08lX\n", hr);
            return false;
        }

        hr = frameEncode->Commit();
        if (FAILED(hr)) {
            printf("提交帧失败: 0x%08lX\n", hr);
            return false;
        }

        hr = encoder->Commit();
        if (FAILED(hr)) {
            printf("提交编码器失败: 0x%08lX\n", hr);
            return false;
        }

        return true;

    } catch (const std::exception& e) {
        printf("保存JPEG异常: %s\n", e.what());
        return false;
    } catch (...) {
        printf("保存JPEG发生未知异常\n");
        return false;
    }

    // 清理资源
    if (propertyBag) propertyBag->Release();
    if (frameEncode) frameEncode->Release();
    if (encoder) encoder->Release();
    if (stream) stream->Release();

    return false;
}

void ThumbGen::workerThread() {
    printf("缩略图生成工作线程启动\n");

    while (true) {
        ThumbnailTask task;
        
        // 等待任务
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_queueCondition.wait(lock, [this] {
                return !m_taskQueue.empty() || m_stopRequested;
            });

            if (m_stopRequested && m_taskQueue.empty()) {
                break;
            }

            if (!m_taskQueue.empty()) {
                task = m_taskQueue.front();
                m_taskQueue.pop();
            } else {
                continue;
            }
        }

        // 执行任务
        bool success = generateThumbnail(task.videoFile, task.thumbnailPath, task.config);
        
        // 调用回调
        if (task.callback) {
            std::string errorMessage = success ? "" : "缩略图生成失败";
            task.callback(success, task.thumbnailPath, errorMessage);
        }
    }

    printf("缩略图生成工作线程结束\n");
}

bool ThumbGen::extractBestFrame(const std::filesystem::path& videoPath,
                               const std::filesystem::path& outputPath,
                               const ThumbnailConfig& config) {
    // 定义多个采样点（避开开头和结尾）
    std::vector<double> samplePoints;
    int maxSamples = std::min(config.maxSampleFrames, 8); // 最多8个采样点
    
    // 生成采样点：从15%到85%之间均匀分布
    for (int i = 0; i < maxSamples; ++i) {
        double position = 0.15 + (0.7 * i) / (maxSamples - 1);
        samplePoints.push_back(position);
    }
    
    // 如果只有一个采样点，使用默认位置
    if (maxSamples == 1) {
        samplePoints = {config.timePosition};
    }
    
    double bestBrightness = 0.0;
    std::filesystem::path bestFramePath;
    bool foundValidFrame = false;
    
    // 为每个采样点生成临时缩略图
    for (size_t i = 0; i < samplePoints.size(); ++i) {
        std::filesystem::path tempPath = outputPath;
        tempPath.replace_filename(outputPath.stem().string() + "_temp_" + std::to_string(i) + ".jpg");
        
        // 提取帧
        if (extractFrameAtTime(videoPath, samplePoints[i], tempPath, config)) {
            // 加载位图进行分析
            Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
            HRESULT hr = m_imagingFactory->CreateDecoderFromFilename(
                tempPath.wstring().c_str(),
                nullptr,
                GENERIC_READ,
                WICDecodeMetadataCacheOnDemand,
                &decoder
            );
            
            if (SUCCEEDED(hr)) {
                Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
                hr = decoder->GetFrame(0, &frame);
                
                if (SUCCEEDED(hr)) {
                    Microsoft::WRL::ComPtr<IWICBitmap> bitmap;
                    hr = m_imagingFactory->CreateBitmapFromSource(frame.Get(), WICBitmapCacheOnDemand, &bitmap);
                    
                    if (SUCCEEDED(hr)) {
                        // 检查是否为黑帧
                        if (!isBlackFrame(bitmap.Get(), config.blackFrameThreshold)) {
                            double brightness = calculateFrameBrightness(bitmap.Get());
                            
                            // 选择亮度最高的非黑帧
                            if (brightness > bestBrightness) {
                                bestBrightness = brightness;
                                bestFramePath = tempPath;
                                foundValidFrame = true;
                            }
                        }
                    }
                }
            }
        }
    }
    
    // 如果找到了合适的帧，复制到目标位置
    if (foundValidFrame && !bestFramePath.empty()) {
        try {
            std::filesystem::copy_file(bestFramePath, outputPath, std::filesystem::copy_options::overwrite_existing);
        } catch (const std::exception&) {
            foundValidFrame = false;
        }
    }
    
    // 清理临时文件
    for (size_t i = 0; i < samplePoints.size(); ++i) {
        std::filesystem::path tempPath = outputPath;
        tempPath.replace_filename(outputPath.stem().string() + "_temp_" + std::to_string(i) + ".jpg");
        try {
            std::filesystem::remove(tempPath);
        } catch (...) {
            // 忽略删除错误
        }
    }
    
    // 如果没有找到合适的帧，使用默认方法
    if (!foundValidFrame) {
        return extractFrameAtTime(videoPath, config.timePosition, outputPath, config);
    }
    
    return true;
}

bool ThumbGen::isBlackFrame(IWICBitmap* bitmap, double threshold) {
    if (!bitmap) return true;
    
    UINT width, height;
    HRESULT hr = bitmap->GetSize(&width, &height);
    if (FAILED(hr)) return true;
    
    // 采样检查（每隔10个像素检查一次以提高性能）
    const int step = 10;
    int totalPixels = 0;
    int darkPixels = 0;
    
    Microsoft::WRL::ComPtr<IWICBitmapLock> lock;
    WICRect rect = {0, 0, static_cast<INT>(width), static_cast<INT>(height)};
    hr = bitmap->Lock(&rect, WICBitmapLockRead, &lock);
    if (FAILED(hr)) return true;
    
    UINT bufferSize;
    BYTE* buffer;
    hr = lock->GetDataPointer(&bufferSize, &buffer);
    if (FAILED(hr)) return true;
    
    UINT stride;
    hr = lock->GetStride(&stride);
    if (FAILED(hr)) return true;
    
    for (UINT y = 0; y < height; y += step) {
        for (UINT x = 0; x < width; x += step) {
            UINT offset = y * stride + x * 4; // 假设BGRA格式
            if (offset + 3 < bufferSize) {
                BYTE b = buffer[offset];
                BYTE g = buffer[offset + 1];
                BYTE r = buffer[offset + 2];
                
                // 计算亮度
                double luminance = (0.299 * r + 0.587 * g + 0.114 * b) / 255.0;
                
                if (luminance < 0.1) { // 很暗的像素
                    darkPixels++;
                }
                totalPixels++;
            }
        }
    }
    
    if (totalPixels == 0) return true;
    
    double darkRatio = static_cast<double>(darkPixels) / totalPixels;
    return darkRatio > threshold;
}

double ThumbGen::calculateFrameBrightness(IWICBitmap* bitmap) {
    if (!bitmap) return 0.0;
    
    UINT width, height;
    HRESULT hr = bitmap->GetSize(&width, &height);
    if (FAILED(hr)) return 0.0;
    
    Microsoft::WRL::ComPtr<IWICBitmapLock> lock;
    WICRect rect = {0, 0, static_cast<INT>(width), static_cast<INT>(height)};
    hr = bitmap->Lock(&rect, WICBitmapLockRead, &lock);
    if (FAILED(hr)) return 0.0;
    
    UINT bufferSize;
    BYTE* buffer;
    hr = lock->GetDataPointer(&bufferSize, &buffer);
    if (FAILED(hr)) return 0.0;
    
    UINT stride;
    hr = lock->GetStride(&stride);
    if (FAILED(hr)) return 0.0;
    
    double totalBrightness = 0.0;
    int pixelCount = 0;
    
    // 采样计算平均亮度（每隔5个像素）
    const int step = 5;
    for (UINT y = 0; y < height; y += step) {
        for (UINT x = 0; x < width; x += step) {
            UINT offset = y * stride + x * 4; // 假设BGRA格式
            if (offset + 3 < bufferSize) {
                BYTE b = buffer[offset];
                BYTE g = buffer[offset + 1];
                BYTE r = buffer[offset + 2];
                
                // 计算亮度
                double luminance = (0.299 * r + 0.587 * g + 0.114 * b) / 255.0;
                totalBrightness += luminance;
                pixelCount++;
            }
        }
    }
    
    return pixelCount > 0 ? totalBrightness / pixelCount : 0.0;
}

} // namespace VideoViewer