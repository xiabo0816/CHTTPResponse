# CHTTPResponse

based on [libevent-2.1.12-stable.tar.gz](https://libevent.org/)

# 使用方法

## 启动和注册路由
```c
web_init();
web_add_service("/dump", dump_request_cb);
web_start(9999);
```

## 读写数据
```c
if(!web_read(req, psMsg, 1024 * 1024 * 2)){
    printf("Error web_read\n");
    web_error(req, HTTP_BADREQUEST, 0);
    return;
}

if(!web_write(req, "asdfadf")){
    web_error(req, HTTP_BADREQUEST, 0);
    return;
}

web_put_status(req, 200, "OK");
```

# 编译和测试

```bash
cd /usr/local/include
mv event2 event2bak
```

```bash
mkdir build
cd build
cmake ..
make
./CHTTPResponse -p 9999 .
```

# 注意事项

baks下是还未处理好的文件