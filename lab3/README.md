# Lab3

## 编译和运行方法


## 整体设计以及使用的技术

### 解析与检验http头,实现读取文件资源部分
> 此部分用两个函数处理的
* `parse_request()`函数
  * 解析出文件的路径,一经检测到跳出当前路径就返回-1,代表要写入`500 Internal Server Error`
  * 判断请求头是否完整,主要是看是否有HTTP1.0或HTTP1.1以及HOST

* `handle_clnt()`函数
  * 在读`clnt_sock`时判断请求的method是否为GET,不是则写入`500 Internal Server Error`
  * 如果文件不存在(包括请求如果是路径不存在的情况),则写入`404 Not Found`
  * 如果请求的是路径且存在,写入`500 Internal Server Error`
### 多线程部分



## `siege`测试结果



## 遇到的问题以及相对应的sol(相当于做实验的记录,助教可以pass这个部分)
* `path`多出了一个空格导致读文件总是出错
  * 在`parse_request()`函数的最后应该给`path[s2-s1]`赋值'\0'
而不是给`path[s2-s1+1]`
  ```C
  memcpy(path, req + s1, (s2 - s1 + 1) * sizeof(char));
  path[s2 - s1] = '\0';
  ```
* `struct stat *buf`之前没有分配内存且没有指向任何东西，所以使用`stat(path, buf)`会报错
  * 换成使用`struct stat buf`，`stat(path, &buf)`
* `free()`出问题，STFW之后发现是可能改变了指针的指向
  * 在每次`malloc`后都另设一个指针指向相同的区域，用于之后的`free()`
* 针对`POST`请求出现的问题：`POST`请求时request中含有文件内容，所有单纯通过"\r\n\r\n"来判断是否读完不正确
  * 解决方法是在读的while循环里直接进行判断是否为GET方法，而不是跳出循环后再对request进行判断