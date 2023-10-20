# C++高性能服务器框架 | 项目作者：bilibili @sylar-yin   
基于C++的Linux高性能服务器框架，包含日志模块、配置模块、协程模块、调度器模块以及网络模块等。

## 环境：
    - CentOS 7.9.2009
    - Cmake 3.4.1
    - gcc 8.3.1
## 第三方库：
    - boost
    - yaml-cpp 0.6.2
## 使用方法：
    - 你需要首先安装以上两个库，运行环境及及三方库如上述所示
    - 库头文件需要修改为实际你安装后所在路径
    - 按照需求，编写自己的函数，并在CMakeLists文件中添加编译
## 运行：
    - 到CMakeLists.txt所在路径，执行cmake . 生成make文件
    - 执行make clean清理下文件
    - 编写自己的程序，在CMakeLists.txt中添加编译，之后执行make
    - 执行可运行程序，tests文件夹下有一些测试各模块的用例，这些文件可执行文件在bin/下生成    

