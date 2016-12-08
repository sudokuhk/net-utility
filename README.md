# net-utililty

一个网络代码库，和一些依赖库实现的服务器应用程序.

目录介绍：

├── LICENSE

├── Makefile

├── README.md

├── uconfig      -- 配置文件读取

├── uhttp        -- 实现的http模块

├── uimg         -- 依赖库实现的图片服务器，支持上传和下载

├── ulog         -- 日志模块

├── unetwork     -- 网络模块

├── ustream      -- 流模块

├── utest        -- 测试代码

├── uthread      -- 使用context实现的协程库

└── utools       -- 工具


其中，unetwork、ustream、uthread是参照腾讯开源的phxrpc，使用C++代码实现的相互独立的模块。
phxrpc仓库：https://github.com/tencent-wechat/phxrpc

[uimg]
--编译
1. 进入uimg目录
2. 执行make
3. ./uimg -f default.conf，默认监听端口8783.

--使用
[浏览器]
1. 打开浏览器，输入ip:8783访问index,上传图片
2. 打开ip:8783/download?md5=xxxx 可以下载md5对应的图片

[二进制上传]
1. ip:8783/upload，post方式
