cmake_minimum_required(VERSION 3.20)
project(VideoViewer VERSION 1.0.0 LANGUAGES C CXX)

# 设置 C++20 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 编译器特定设置
if(MSVC)
    # MSVC 编译器设置
    add_compile_options(/W4 /WX- /utf-8)
    add_compile_definitions(_UNICODE UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
    # 启用多处理器编译
    add_compile_options(/MP)
elseif(MINGW)
    # MinGW 编译器设置
    add_compile_options(-Wall -Wextra -Wpedantic)
    add_compile_definitions(_UNICODE UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
endif()

# ============================================================================
# SQLite3 配置 - 优先使用本地源文件
# ============================================================================

# 设置 SQLite 路径
set(SQLITE_SOURCE_DIR ${CMAKE_SOURCE_DIR}/third_party/sqlite)
set(SQLITE_INCLUDE_DIR ${SQLITE_SOURCE_DIR}/include)
set(SQLITE_SRC_FILE ${SQLITE_SOURCE_DIR}/src/sqlite3.c)

# 检查是否存在本地 SQLite 源文件
if(EXISTS ${SQLITE_SRC_FILE} AND EXISTS ${SQLITE_INCLUDE_DIR}/sqlite3.h)
    message(STATUS "使用本地 SQLite 源文件: ${SQLITE_SRC_FILE}")
    
    # 创建 SQLite 静态库
    add_library(sqlite3_local STATIC ${SQLITE_SRC_FILE})
    
    # 设置 SQLite 包含目录
    target_include_directories(sqlite3_local PUBLIC ${SQLITE_INCLUDE_DIR})
    
    # 明确设置语言为C
    set_target_properties(sqlite3_local PROPERTIES
        LINKER_LANGUAGE C
        C_STANDARD 99
    )
    
    # SQLite 编译选项 - 启用常用功能并确保中文支持
    target_compile_definitions(sqlite3_local PRIVATE
        SQLITE_ENABLE_FTS5=1          # 全文搜索
        SQLITE_ENABLE_JSON1=1         # JSON 支持
        SQLITE_ENABLE_RTREE=1         # R-Tree 索引
        SQLITE_THREADSAFE=1           # 线程安全
        SQLITE_ENABLE_COLUMN_METADATA=1  # 列元数据
        SQLITE_ENABLE_DBSTAT_VTAB=1   # 数据库统计
        _UNICODE                      # Unicode 支持
        UNICODE                       # Unicode 支持
    )
    
    # 设置 SQLite 变量供主程序使用
    set(SQLITE3_FOUND TRUE)
    set(SQLITE3_INCLUDE_DIRS ${SQLITE_INCLUDE_DIR})
    set(SQLITE3_LIBRARIES sqlite3_local)
    
else()
    # 回退到系统 SQLite3
    message(STATUS "本地 SQLite 源文件未找到，尝试查找系统 SQLite3...")
    
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(SQLITE3 QUIET sqlite3)
    endif()

    if(NOT SQLITE3_FOUND)
        # 如果没有找到 pkg-config 或 SQLite3，尝试手动查找
        find_path(SQLITE3_INCLUDE_DIR sqlite3.h)
        find_library(SQLITE3_LIBRARY sqlite3)
        
        if(SQLITE3_INCLUDE_DIR AND SQLITE3_LIBRARY)
            set(SQLITE3_FOUND TRUE)
            set(SQLITE3_INCLUDE_DIRS ${SQLITE3_INCLUDE_DIR})
            set(SQLITE3_LIBRARIES ${SQLITE3_LIBRARY})
        else()
            message(FATAL_ERROR "SQLite3 未找到！请按照以下步骤操作：
1. 从 https://www.sqlite.org/download.html 下载 sqlite-amalgamation 包
2. 将 sqlite3.h 放入 ${SQLITE_INCLUDE_DIR}/
3. 将 sqlite3.c 放入 ${SQLITE_SOURCE_DIR}/src/
4. 或者安装系统 SQLite3 开发库")
        endif()
    endif()
endif()

# ============================================================================
# 项目配置
# ============================================================================

# 包含目录
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/resources)
if(SQLITE3_FOUND)
    include_directories(${SQLITE3_INCLUDE_DIRS})
endif()

# 源文件
set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/FileScanner.cpp
    src/VideoManager.cpp
    src/VideoFile.cpp
    src/Utils.cpp
    src/DatabaseManager.cpp
    src/VideoFileDAO.cpp
)

# 头文件
set(HEADERS
    include/MainWindow.h
    include/FileScanner.h
    include/VideoManager.h
    include/VideoFile.h
    include/Utils.h
    resources/resource.h
    include/DatabaseManager.h
    include/VideoFileDAO.h
)

# 资源文件
set(RESOURCES
    resources/VideoViewer.rc
)

# 设置可执行文件（临时移除 WIN32 以便看到控制台输出）
add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
    ${RESOURCES}
)

# 链接 Windows 系统库
target_link_libraries(${PROJECT_NAME}
    user32
    kernel32
    gdi32
    shell32
    comctl32
    ole32
    oleaut32
    uuid
    mf
    mfplat
    mfreadwrite
    mfuuid
    shlwapi
)

# 链接 SQLite3 库
if(SQLITE3_FOUND)
    target_link_libraries(${PROJECT_NAME} ${SQLITE3_LIBRARIES})
    message(STATUS "SQLite3 已链接: ${SQLITE3_LIBRARIES}")
else()
    message(FATAL_ERROR "SQLite3 未找到，无法编译项目")
endif()

# 设置子系统为 Windows
if(MSVC)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:WINDOWS"
    )
endif()

# 复制资源文件到输出目录
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/resources
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/resources
)

# 安装规则
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

install(DIRECTORY resources/
    DESTINATION bin/resources
)

# ============================================================================
# 调试信息
# ============================================================================
message(STATUS "=== SQLite3 配置信息 ===")
message(STATUS "SQLite3 找到: ${SQLITE3_FOUND}")
message(STATUS "SQLite3 包含目录: ${SQLITE3_INCLUDE_DIRS}")
message(STATUS "SQLite3 库: ${SQLITE3_LIBRARIES}")
if(EXISTS ${SQLITE_SRC_FILE})
    message(STATUS "使用本地 SQLite 源文件: 是")
else()
    message(STATUS "使用本地 SQLite 源文件: 否")
    message(STATUS "请下载 SQLite amalgamation 包以获得最佳性能")
endif()
message(STATUS "========================")