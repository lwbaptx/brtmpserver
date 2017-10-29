# brtmpserver
rtmpserver base brpc

## 编译
1.编译[brpc](https://github.com/brpc/brpc),把brpc编译出来的output文件拷贝到当前的brpc文件夹下面

2.make 编译源码

3.执行 ./rtmp_server 启动进程

## 推流
`ffmpeg -re -i test.flv -acodec copy -vcodec copy -f flv rtmp://localhost/live/test`

## 播放
`ffplay rtmp://localhost/live/test`
