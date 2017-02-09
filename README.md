# ZCache
zc cache for study redis

1. 底层基于自己写的网络库
2. 通信协议采用二进制协议
3. 支持各类数据结构嵌套
4. 多线程服务器构建
5. 同步、异步两种客户端模式


# 测试
1. 基本功能都已经跑通，包括各种正常请求以及非法请求
2. 异步性能较高，本机吞吐量可达60W以上，两边都存在缓冲队列，且只有发送遇到阻塞才会尝试Sleep(1)，所以延迟较高
3. 同步延迟低，99.99%响应时间不超过2ms（time精度不行），吞吐量12W



## function test

```C++
Running main() from gtest_main.cc
[==========] Running 53 tests from 22 test cases.
[----------] Global test environment set-up.
Init client .
Init test context .
Init done .
[----------] 1 test from ZCmdPing
[ RUN      ] ZCmdPing.PingPong
[       OK ] ZCmdPing.PingPong (0 ms)
[----------] 1 test from ZCmdPing (2 ms total)

[----------] 2 tests from ZCmdExist
[ RUN      ] ZCmdExist.ExistFalse
[       OK ] ZCmdExist.ExistFalse (1 ms)
[ RUN      ] ZCmdExist.ExistTrue
[       OK ] ZCmdExist.ExistTrue (4 ms)
[----------] 2 tests from ZCmdExist (6325 ms total)

[----------] 4 tests from ZCmdInc
[ RUN      ] ZCmdInc.Normal
[       OK ] ZCmdInc.Normal (1 ms)
[ RUN      ] ZCmdInc.NoKey
[       OK ] ZCmdInc.NoKey (0 ms)
[ RUN      ] ZCmdInc.ErrType
[       OK ] ZCmdInc.ErrType (1 ms)
[ RUN      ] ZCmdInc.Overflow
[       OK ] ZCmdInc.Overflow (0 ms)
[----------] 4 tests from ZCmdInc (3 ms total)

[----------] 2 tests from ZCmdSelect
[ RUN      ] ZCmdSelect.Normal
[       OK ] ZCmdSelect.Normal (0 ms)
[ RUN      ] ZCmdSelect.InvalidIndex
[       OK ] ZCmdSelect.InvalidIndex (0 ms)
[----------] 2 tests from ZCmdSelect (2 ms total)

[----------] 1 test from ZCmdDBSize
[ RUN      ] ZCmdDBSize.Normal
[       OK ] ZCmdDBSize.Normal (1 ms)
[----------] 1 test from ZCmdDBSize (1 ms total)

[----------] 3 tests from ZCmdRename
[ RUN      ] ZCmdRename.Normal
[       OK ] ZCmdRename.Normal (0 ms)
[ RUN      ] ZCmdRename.NoOldKey
[       OK ] ZCmdRename.NoOldKey (0 ms)
[ RUN      ] ZCmdRename.ExistNewKey
[       OK ] ZCmdRename.ExistNewKey (0 ms)
[----------] 3 tests from ZCmdRename (2 ms total)

[----------] 2 tests from ZCmdType
[ RUN      ] ZCmdType.Normal
[       OK ] ZCmdType.Normal (0 ms)
[ RUN      ] ZCmdType.NoKey
[       OK ] ZCmdType.NoKey (0 ms)
[----------] 2 tests from ZCmdType (3 ms total)

[----------] 3 tests from ZCmdPush
[ RUN      ] ZCmdPush.Normal
[       OK ] ZCmdPush.Normal (0 ms)
[ RUN      ] ZCmdPush.NoKey
[       OK ] ZCmdPush.NoKey (0 ms)
[ RUN      ] ZCmdPush.WrongType
[       OK ] ZCmdPush.WrongType (0 ms)
[----------] 3 tests from ZCmdPush (4 ms total)

[----------] 4 tests from ZCmdPop
[ RUN      ] ZCmdPop.Normal
[       OK ] ZCmdPop.Normal (1 ms)
[ RUN      ] ZCmdPop.NoKey
[       OK ] ZCmdPop.NoKey (0 ms)
[ RUN      ] ZCmdPop.WrongType
[       OK ] ZCmdPop.WrongType (0 ms)
[ RUN      ] ZCmdPop.EmptyErr
[       OK ] ZCmdPop.EmptyErr (0 ms)
[----------] 4 tests from ZCmdPop (3 ms total)

[----------] 3 tests from ZCmdLLen
[ RUN      ] ZCmdLLen.Normal
[       OK ] ZCmdLLen.Normal (0 ms)
[ RUN      ] ZCmdLLen.NoKey
[       OK ] ZCmdLLen.NoKey (0 ms)
[ RUN      ] ZCmdLLen.WrongType
[       OK ] ZCmdLLen.WrongType (0 ms)
[----------] 3 tests from ZCmdLLen (3 ms total)

[----------] 4 tests from ZCmdLRange
[ RUN      ] ZCmdLRange.Normal
[       OK ] ZCmdLRange.Normal (2 ms)
[ RUN      ] ZCmdLRange.NoKey
[       OK ] ZCmdLRange.NoKey (0 ms)
[ RUN      ] ZCmdLRange.WrongType
[       OK ] ZCmdLRange.WrongType (0 ms)
[ RUN      ] ZCmdLRange.InvalidParam
[       OK ] ZCmdLRange.InvalidParam (1 ms)
[----------] 4 tests from ZCmdLRange (5 ms total)

[----------] 1 test from ZAPerformanceTest
[ RUN      ] ZAPerformanceTest.Normal

```


## asyn test

cpu: I7 6700
mem: 16 GB

synFlag false
client 50
buffersize 256
requestTime 100000


```c++

[Info]: 监控日志已经建立！
[Info]: 0.145000 <= 0 ms
[Info]: 0.152000 <= 1 ms
[Info]: 0.154000 <= 2 ms
[Info]: 0.156000 <= 3 ms
[Info]: 0.159000 <= 4 ms
[Info]: 0.232000 <= 5 ms
[Info]: 0.404000 <= 6 ms
[Info]: 0.587000 <= 7 ms
[Info]: 0.828000 <= 8 ms
[Info]: 1.215000 <= 9 ms
[Info]: 1.592000 <= 10 ms
[Info]: 2.262000 <= 20 ms
[Info]: 8.436001 <= 30 ms
[Info]: 40.259003 <= 40 ms
[Info]: 65.138000 <= 50 ms
[Info]: 84.300003 <= 60 ms
[Info]: 94.969002 <= 70 ms
[Info]: 98.940002 <= 80 ms
[Info]: 99.784012 <= 90 ms
[Info]:

Request 100000 times, last 100000 take 6572 ms, 15216.068359 per sec.
[Info]: 0.000000 <= 0 ms
[Info]: 0.105000 <= 1 ms
[Info]: 0.319000 <= 2 ms
[Info]: 0.447000 <= 3 ms
[Info]: 0.773000 <= 4 ms
[Info]: 1.446000 <= 5 ms
[Info]: 2.491000 <= 6 ms
[Info]: 3.977000 <= 7 ms
[Info]: 5.345000 <= 8 ms
[Info]: 6.536000 <= 9 ms
[Info]: 7.743000 <= 10 ms
[Info]: 17.424999 <= 20 ms
[Info]: 36.314999 <= 30 ms
[Info]: 62.311993 <= 40 ms
[Info]: 72.486000 <= 50 ms
[Info]: 78.753998 <= 60 ms
[Info]: 85.546997 <= 70 ms
[Info]: 91.250999 <= 80 ms
[Info]: 95.085999 <= 90 ms
[Info]: 96.721992 <= 100 ms
[Info]: 98.745987 <= 110 ms
[Info]: 99.816978 <= 120 ms
[Info]:

Request 200000 times, last 100000 take 159 ms, 628930.812500 per sec.
[Info]: 0.003000 <= 0 ms
[Info]: 0.135000 <= 1 ms
[Info]: 0.481000 <= 2 ms
[Info]: 1.098000 <= 3 ms
[Info]: 1.613000 <= 4 ms
[Info]: 2.277000 <= 5 ms
[Info]: 3.419000 <= 6 ms
[Info]: 4.618000 <= 7 ms
[Info]: 6.002000 <= 8 ms
[Info]: 8.280000 <= 9 ms
[Info]: 10.965000 <= 10 ms
[Info]: 29.024998 <= 20 ms
[Info]: 48.116997 <= 30 ms
[Info]: 57.341999 <= 40 ms
[Info]: 66.464989 <= 50 ms
[Info]: 76.138977 <= 60 ms
[Info]: 82.928978 <= 70 ms
[Info]: 87.621956 <= 80 ms
[Info]: 91.958954 <= 90 ms
[Info]: 95.646942 <= 100 ms
[Info]: 97.859947 <= 110 ms
[Info]: 98.626945 <= 120 ms
[Info]: 99.256950 <= 130 ms
[Info]: 99.853951 <= 140 ms
[Info]:

Request 300000 times, last 100000 take 149 ms, 671140.937500 per sec.
[Info]: 0.000000 <= 0 ms
[Info]: 0.071000 <= 1 ms
[Info]: 0.619000 <= 2 ms
[Info]: 1.900000 <= 3 ms
[Info]: 3.905000 <= 4 ms
[Info]: 6.103000 <= 5 ms
[Info]: 8.856000 <= 6 ms
[Info]: 12.922000 <= 7 ms
[Info]: 16.405001 <= 8 ms
[Info]: 19.180000 <= 9 ms
[Info]: 21.757000 <= 10 ms
[Info]: 37.960999 <= 20 ms
[Info]: 48.500000 <= 30 ms
[Info]: 54.123993 <= 40 ms
[Info]: 59.887997 <= 50 ms
[Info]: 66.034996 <= 60 ms
[Info]: 76.164001 <= 70 ms
[Info]: 83.675987 <= 80 ms
[Info]: 87.703987 <= 90 ms
[Info]: 91.677994 <= 100 ms
[Info]: 95.520996 <= 110 ms
[Info]: 98.493996 <= 120 ms
[Info]:

Request 400000 times, last 100000 take 154 ms, 649350.625000 per sec.
```


## syn test

cpu: I7 6700
mem: 16 GB

synFlag true
client 50
buffersize 256
requestTime 100000


```c++
[Info]: 监控日志已经建立！
[Info]: 93.620003 <= 0 ms
[Info]:

Request 100000 times, last 100000 take 928 ms, 107758.617188 per sec.
[Info]: 93.835999 <= 0 ms
[Info]:

Request 200000 times, last 100000 take 779 ms, 128369.703125 per sec.
[Info]: 93.893997 <= 0 ms
[Info]:

Request 300000 times, last 100000 take 773 ms, 129366.109375 per sec.
[Info]: 93.886002 <= 0 ms
[Info]:

Request 400000 times, last 100000 take 774 ms, 129198.968750 per sec.
[Info]: 93.880997 <= 0 ms
[Info]:

Request 500000 times, last 100000 take 774 ms, 129198.968750 per sec.
[Info]: 93.902000 <= 0 ms
[Info]:

Request 600000 times, last 100000 take 771 ms, 129701.687500 per sec.
[Info]: 93.865997 <= 0 ms
[Info]:

Request 700000 times, last 100000 take 777 ms, 128700.125000 per sec.
```