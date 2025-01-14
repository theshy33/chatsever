cmake_minimum_required(VERSION 3.0)
project(main)

#  配置编译选项
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g)#-g debug可以dubug

# 配置头文件搜索路径
# include_directories()
# 配置库文件搜索路径
# link_directories()

#设置需要编译的源文件列表
set(SRC_LIST muduo_server.cpp)

# 设置可执行文件的输出路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# 把.指定目录下所有源文件名字加入变量SRC_LIST
# aux_source_directory(. SRC_LIST)

# 表示生成可执行文件server,由SRC_LIST变量定义的源文件编译而来
add_executable(server ${SRC_LIST})
# 表示server这个目标程序，需要连接muduo_net,muduo_base,pthread三个库文件
target_link_libraries(server muduo_net muduo_base pthread)
