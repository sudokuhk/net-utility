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
