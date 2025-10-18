# Windows 视频快速阅览器

一个轻量级的 Windows 原生桌面应用，专门用于快速浏览和管理本地视频文件。

## 功能特性

- 🎬 **快速扫描**: 递归扫描目录，支持多种视频格式
- 📋 **文件列表**: 清晰显示视频文件信息（名称、大小、修改时间）
- ▶️ **一键播放**: 双击文件即可调用系统默认播放器
- 🗑️ **安全删除**: 支持将文件移动到回收站
- 🔍 **快速搜索**: 按文件名快速定位视频文件
- 🎯 **原生性能**: 使用 C++20 和 Win32 API，无额外依赖

## 支持的视频格式

- MP4 (.mp4)
- MKV (.mkv) 
- AVI (.avi)
- MOV (.mov)
- WMV (.wmv)
- FLV (.flv)
- WebM (.webm)

## 系统要求

- Windows 10/11 (x64)
- Visual C++ Redistributable (如果使用 MSVC 编译)

## 构建要求

- CMake 3.20+
- C++20 兼容编译器:
  - MSVC 2022+ (推荐)
  - MinGW-w64 15.1.0+
- Windows SDK

## 构建说明

### 使用 MSVC (推荐)

```bash
# 创建构建目录
mkdir build
cd build

# 配置项目
cmake .. -G "Visual Studio 17 2022" -A x64

# 编译
cmake --build . --config Release
```

### 使用 MinGW

```bash
# 创建构建目录
mkdir build
cd build

# 配置项目
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release

# 编译
cmake --build .
```

## 使用说明

1. 启动应用程序
2. 点击"扫描目录"按钮选择要扫描的文件夹
3. 等待扫描完成，视频文件将显示在列表中
4. 双击任意视频文件即可播放
5. 选中文件后按 Delete 键可删除文件（移动到回收站）

## 项目结构

```
VideoViewer/
├── CMakeLists.txt          # CMake 构建配置
├── README.md               # 项目说明
├── include/                # 头文件目录
│   ├── MainWindow.h        # 主窗口类
│   ├── FileScanner.h       # 文件扫描器
│   ├── VideoManager.h      # 视频管理器
│   ├── VideoFile.h         # 数据结构定义
│   ├── Utils.h             # 工具函数
│   └── resource.h          # 资源定义
├── src/                    # 源文件目录
│   ├── main.cpp            # 程序入口
│   ├── MainWindow.cpp      # 主窗口实现
│   ├── FileScanner.cpp     # 文件扫描实现
│   ├── VideoManager.cpp    # 视频管理实现
│   └── Utils.cpp           # 工具函数实现
└── resources/              # 资源文件目录
    └── VideoViewer.rc      # Windows 资源文件
```

## 开发路线图

- [x] 阶段 1: MVP 实现（文件扫描、列表显示、播放、删除）
- [ ] 阶段 2: 缩略图生成与缓存
- [ ] 阶段 3: 现代 UI 升级（WinUI 3）
- [ ] 阶段 4: SQLite 索引与搜索
- [ ] 阶段 5: 高级功能（收藏、标签、多线程）

## 许可证

本项目采用 MIT 许可证。详见 LICENSE 文件。

## 贡献

欢迎提交 Issue 和 Pull Request！

## 联系方式

如有问题或建议，请通过 GitHub Issues 联系。