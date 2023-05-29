# JPEGCodec

JPEG编解码器

完成于2023年5月29日

https://zhuanlan.zhihu.com/p/633152821

## 用法

| 命令选项       | 说明                                                         |
| -------------- | ------------------------------------------------------------ |
| -encode        | 指示程序以编码模式工作                                       |
| -decode        | 指示程序以解码模式工作                                       |
| -fact_h [arg]  | 指定水平采样因子 1<=[arg]<=2                                 |
| -fact_v [arg]  | 指定垂直采样因子 1<=[arg]<=2                                 |
| -src [arg]     | 指定源文件.  [arg]为文件路径                                 |
| -dst [arg]     | 指定输出文件的路径                                           |
| -quality [arg] | 指定一个质量因子，该因子会影响bmp编码为jpg后的图像质量和大小 |
| -h             | 打印帮助信息                                                 |



## 例子

`JPEGCodec -encode -fact_h 1 -fact_v 2 -quality 75 -src img.bmp -dst img.jpg`

将img.bmp编码输出为img.jpg文件，设置水平采用因子为1，垂直采样因子为2，质量因子为75

`JPEGCodec -decode -src img.jpg -dst img.bmp`

将img.jpg解码为img.bmp

## 其他

在仓库中的test.zip中，我放了6张测试图片，一个编译好的JPEGCodec.exe程序，以及一个测试用的bat文件，这个文件会运行JPEGCodec.exe，尝试以不同的采样因子和质量因子生成jpg，然后将生成的jpg文件解码回去生成相应的bmp文件，用来简单测试程序的正确性。

这六张测试图片各有如下特征：

| 文件名   | 特征                                           |
| -------- | ---------------------------------------------- |
| img.bmp  | 580x580，二次元妹子图                          |
| img2.bmp | 10536x4445，旅行日记, 大文件(133MB)，高度像素不能被8整除 |
| img3.bmp | 1919x941，CLANNAD中的一条公路, 宽度和高度像素均不能被8整除          |
| img4.bmp | 1919x1079，某次网课截图, 32位深度的bmp,宽度和高度像素均不能被8整除       |
| img5.bmp | 8x8，随便画的,图像较小                                  |
| img6.bmp | 5x5，随便画的,图像相当小                                |

也就是说，这六张图片算是考虑了一些极端情况的，我在运行测试时没发现输出的图像有什么问题，当然，如果你在使用过程中发现程序输出有一些错误，欢迎在Issue中提出或者联系我修改。

需要指出的是，这个JPEG的编解码器只能对一些最基本的图片进行编解码，编码部分，无法识别灰度bmp图和带调色盘的bmp图，解码部分，只能解码最简单的JPG图像文件（Baseline DCT, JFIF头，无restart marker等）

Completed on 2023/5/29 by kiminouso, Hohai University
