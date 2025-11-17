cmake_minimum_required(VERSION 3.20)
project(BronchoscopyLib CXX)

# 加载本地配置文件（如果存在）
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/config.local.cmake")
    message(STATUS "Loading local configuration from config.local.cmake")
    include("${CMAKE_CURRENT_SOURCE_DIR}/config.local.cmake")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/config.cmake")
    message(STATUS "Loading configuration from config.cmake")
    include("${CMAKE_CURRENT_SOURCE_DIR}/config.cmake")
endif()

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 解决MSVC编译器字符编码警告
if(MSVC)
    add_compile_options("/source-charset:utf-8")
endif()

# VTK配置 - 自动查找或使用环境变量
if(NOT DEFINED VTK_DIR)
    # 首选用户指定的 VTK 9.4.2 路径
    set(VTK_PATHS
        "D:/Compile/VTK9.4.2/lib/cmake/vtk-9.4"
        "C:/VTK/lib/cmake/vtk-9.4"
        "C:/Program Files/VTK/lib/cmake/vtk-9.4"
        "$ENV{VTK_DIR}"
        "$ENV{VTK_ROOT}/lib/cmake/vtk-9.4"
    )
    
    foreach(path ${VTK_PATHS})
        if(EXISTS "${path}/VTKConfig.cmake")
            set(VTK_DIR "${path}")
            message(STATUS "Found VTK at: ${VTK_DIR}")
            break()
        endif()
    endforeach()
endif()

# 如果还是找不到VTK，提示用户设置
if(NOT DEFINED VTK_DIR)
    message(FATAL_ERROR "VTK not found. Please set VTK_DIR or install VTK\n"
                        "You can set it by: cmake -DVTK_DIR=<path_to_vtk>/lib/cmake/vtk-9.4")
endif()

find_package(VTK 9.4 REQUIRED)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 收集源文件
set(SOURCES
    src/BronchoscopyViewer.cpp
    src/CameraPath.cpp
    src/CameraController.cpp
    src/ModelManager.cpp
    src/PathVisualization.cpp
    src/RenderingEngine.cpp
    src/NavigationController.cpp
    src/SceneManager.cpp
    src/ShaderSystem.cpp
    src/BronchoscopyAPI.cpp
)

set(HEADERS
    header/BronchoscopyViewer.h
    header/CameraPath.h
    header/CameraController.h
    header/ModelManager.h
    header/PathVisualization.h
    header/RenderingEngine.h
    header/NavigationController.h
    header/SceneManager.h
    header/ShaderSystem.h
    header/BronchoscopyAPI.h
)

# 创建静态库
add_library(BronchoscopyLib STATIC ${SOURCES} ${HEADERS})

# 设置包含目录
target_include_directories(BronchoscopyLib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/header
    ${VTK_INCLUDE_DIRS}
)

# 链接VTK库
target_link_libraries(BronchoscopyLib PUBLIC ${VTK_LIBRARIES})

# Windows特定设置
if(WIN32)
    # 定义预处理器宏
    target_compile_definitions(BronchoscopyLib PRIVATE
        _WINDOWS
        _UNICODE
        UNICODE
    )
    
    # 设置编译选项
    target_compile_options(BronchoscopyLib PRIVATE
        /W3     # 警告级别3
        /EHsc   # 启用C++异常处理
        /MP     # 多处理器编译
    )
    
    # Release配置优化
    target_compile_options(BronchoscopyLib PRIVATE
        $<$<CONFIG:Release>:/O2>    # 最大优化
        $<$<CONFIG:Release>:/GL>    # 全程序优化
    )
    
    # Debug配置
    target_compile_options(BronchoscopyLib PRIVATE
        $<$<CONFIG:Debug>:/Od>      # 禁用优化
        $<$<CONFIG:Debug>:/Zi>      # 生成调试信息
    )
endif()

# 显示配置信息
message(STATUS "Building BronchoscopyLib Static Library with VTK support")
message(STATUS "VTK_VERSION: ${VTK_VERSION}")
message(STATUS "VTK_USE_FILE: ${VTK_USE_FILE}")
message(STATUS "Output directory: ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")
