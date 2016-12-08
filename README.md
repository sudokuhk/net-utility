### net-utililty

一个网络代码库，和一些依赖库实现的服务器应用程序.</br></br>
目录介绍：</br>
</br>
├── LICENSE</br>
├── Makefile
├── README.md
├── uconfig      -- 配置文件读取</br>
├── uhttp        -- 实现的http模块</br>
├── uimg         -- 依赖库实现的图片服务器，支持上传和下载</br>
├── ulog         -- 日志模块</br>
├── unetwork     -- 网络模块</br>
├── ustream      -- 流模块</br>
├── utest        -- 测试代码</br>
├── uthread      -- 使用context实现的协程库</br>
└── utools       -- 工具</br>
</br>

其中，unetwork、ustream、uthread是参照腾讯开源的phxrpc，使用C++代码实现的相互独立的模块。</br>
phxrpc仓库：https://github.com/tencent-wechat/phxrpc</br>

####[uimg]
#####编译
1. 进入uimg目录
2. 执行make
3. ./uimg -f default.conf，默认监听端口8783.

#####使用
######浏览器
1. 打开浏览器，输入ip:8783访问index,上传图片
2. 打开ip:8783/download?md5=xxxx 可以下载md5对应的图片

######二进制上传
1. ip:8783/upload，post方式
