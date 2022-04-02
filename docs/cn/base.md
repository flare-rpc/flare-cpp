base
===
base 模块包含功能比较多，也比较杂，主要是low level的封装。从功能上可以再细分若干的
子模块

## digest
该模块分装md5，sha1， sha256， sha512编码，api接口很简洁
例如：
对于比较小的字符串，可以直接使用简洁的方式获得16进制表示的字符串。

```c++
auto str = md5_hex("a");
// str = "0cc175b9c0f1b6a831c399e269772661"
```
上述过程实际上封了md5的计算过程

```c++
    std::string md5_hex(const void *data, uint32_t size) {
        return MD5(data, size).digest_hex();
    }
```

对于比较大的文件，可以这样使用：

```c++
     MD5 md5;
    while(condition) {
        md5.process(str);
    }
    auto coded = md5.digest_hex();
```

sha1， sha256， sha512等组件方法雷同，均可参照md5的模式使用。
