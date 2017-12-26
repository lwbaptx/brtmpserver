# brtmpserver
rtmpserver base brpc

## 编译
1.编译[brpc](https://github.com/brpc/brpc),把brpc的output输出文件拷贝到/usr/local下面
```
sudo cp -r output/include/ /usr/local/include/brpc/
sudo cp -r output/lib/ /usr/local/lib/
sudo ldconfig
```

2.编译源码
```
mkdir build
cd build
cmake ..
make
```

3.执行 ./bin/brtmpserver 启动进程

## 推流
`ffmpeg -re -i test.flv -acodec copy -vcodec copy -f flv rtmp://localhost/live/test`

## 播放
`ffplay rtmp://localhost/live/test`
