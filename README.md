# Windows Video Viewer

一个基于 C++ 和 Win32 API 开发的 Windows 视频文件管理器，支持中文文件名和路径。

## 功能特性

- **视频文件扫描**: 支持扫描指定目录下的视频文件
- **数据库管理**: 使用 SQLite 数据库存储视频文件信息
- **中文支持**: 完全支持中文文件名和路径
- **文件操作**: 支持播放、删除、重命名等操作
- **搜索功能**: 支持按文件名、大小、时长等条件搜索
- **收藏功能**: 支持标记和管理收藏的视频文件
- **播放统计**: 记录播放次数和最后播放时间

## 技术栈

- **语言**: C++20
- **GUI**: Win32 API
- **数据库**: SQLite3
- **构建系统**: CMake
- **编译器**: MSVC / MinGW

## 项目结构

```
windows-video-viewer/
├── include/                 # 头文件
│   ├── DatabaseManager.h
│   ├── FileScanner.h
│   ├── MainWindow.h
│   ├── Utils.h
│   ├── VideoFile.h
│   ├── VideoFileDAO.h
│   └── VideoManager.h
├── src/                     # 源文件
│   ├── DatabaseManager.cpp
│   ├── FileScanner.cpp
│   ├── MainWindow.cpp
│   ├── Utils.cpp
│   ├── VideoFile.cpp
│   ├── VideoFileDAO.cpp
│   ├── VideoManager.cpp
│   └── main.cpp
├── resources/               # 资源文件
│   ├── resource.h
│   └── VideoViewer.rc
└── CMakeLists.txt          # 构建配置
```

## 编译要求

- Windows 10/11
- Visual Studio 2019+ 或 MinGW-w64
- CMake 3.20+
- SQLite3 开发库

## 编译步骤

1. 克隆仓库：
```bash
git clone https://github.com/Quantriple/windows-video-viewer.git
cd windows-video-viewer
```

2. 创建构建目录：
```bash
mkdir build
cd build
```

3. 配置 CMake：
```bash
cmake ..
```

4. 编译项目：
```bash
cmake --build . --config Release
```

## SQLite 配置

项目支持两种 SQLite 配置方式：

### 方式一：使用本地源文件（推荐）
1. 从 [SQLite 官网](https://www.sqlite.org/download.html) 下载 sqlite-amalgamation 包
2. 创建目录结构：
   ```
   third_party/sqlite/
   ├── include/sqlite3.h
   └── src/sqlite3.c
   ```
3. 将 `sqlite3.h` 放入 `third_party/sqlite/include/`
4. 将 `sqlite3.c` 放入 `third_party/sqlite/src/`

### 方式二：使用系统 SQLite
安装系统 SQLite3 开发库，CMake 会自动查找。

## 使用说明

1. 启动程序后，通过菜单 "文件 -> 扫描目录" 选择要扫描的视频文件目录
2. 程序会自动扫描并将视频文件信息存储到数据库
3. 在主界面可以查看、搜索、播放视频文件
4. 支持右键菜单进行文件操作

## 支持的视频格式

- MP4
- AVI
- MKV
- MOV
- WMV
- FLV
- 3GP
- M4V

## 许可证

本项目采用 MIT 许可证。详见 [LICENSE](LICENSE) 文件。

## 贡献

欢迎提交 Issue 和 Pull Request 来改进项目。

## 作者

Quantriple