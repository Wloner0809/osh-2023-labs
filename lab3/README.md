# Lab3

## 遇到的问题以及相对应的sol
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