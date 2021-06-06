## 向内核传递调试参数<a name="sectiondebug"></a>

### QEMU命令行参数：

-fw_cfg name=opt/d,string="arg0=071 arg1='abcf arg2=0x81 arg3=81"

双引号部分即为要向内核传递的参数：

- 等号左侧为参数名字，可以自由选择
- 等号右侧为值，支持两种类型
    - 数值，包括0开头的八进制，0x或0X开头的十六进制，其它为十进制，可以有符号，数值最大为long long
    - 字符串，为与数值区分，须以单引号开始（传递到内核前丢弃）
- 参数间以空格分隔
- 参数总长度不超过99个字节

命令行中其它部分为固定值。

**注意**：这个简单驱动没有太多的容错能力，并且QEMU命令行也有一些要求，因此对字符串类型值尽量不要出现逗号、等号、空格等，数值应适合所提供的存储区域；如果需要，可参阅QEMU fw_cfg文档关于使用文件传参的描述。

### 示例

1. 假设内核代码有两个参数需要测试
```
    int queueSize;
    char fileName[MAX_NAME_LENGTH];
```

2. 在初始处增加获取命令行参数的代码
```
#ifdef LOSCFG_PLATFORM_QEMU_ARM_VIRT_CA7
    extern void GetDebugArgs(const char *argName, void *valBuf, unsigned valLen);
    GetDebugArgs("qsz", &queueSize, sizeof(int));
    GetDebugArgs("file", fileName, MAX_NAME_LENGTH);
#endif
```

3. 通过QEMU命令行向内核传递这两个参数值
```
qemu-system-arm ...(正常运行参数) \
                -fw_cfg name=opt/d,string="qsz=0x400 file='test.txt"
```
