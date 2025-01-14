cmake_minimum_required(VERSION 3.10)
project(char)

# 配置编译选项
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g)

# 配置可执行文件输出位置
# 当前 CMakeList.txt 文件所在的目录
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)

# 配置头文件搜索路径
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)#public
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/server)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/server/db)#mysql
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/server/model)#model
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/server/redis)#redis
include_directories(${PROJECT_SOURCE_DIR}/thirdparty)

# 加载子目录
add_subdirectory(src)
