# lab_common_so

lab_common_so是一个C/C++语言编写的，基于woo框架（一个轻型通讯框架，通讯协议及日志系统比较完善）的高效计算服务。

目前我们已经将该框架应用于微博推荐的多个场景中，经过一年多的演进，框架趋于稳定。


<br>
##框架特性

* 透明化的数据获取
* 数据本地化
* 非中断式的资源实时更新
* 统一的接口形态
* 规范化的业务流程
* 插件式的业务更新
* 可配置的算法支持

<br>
##使用场景
* 高效的实时在线计算服务
* 并发式的离线计算服务
* 数据存储代理
* 算法模型训练

<br>
##适用平台
linux 2.6.18及以上

x64（x86需要测试）

gcc 4.1.2(更高版本需要测试)

<br>
##使用方法
1、git clone https://github.com/wbrecom/lab_common_so.git

2、执行setup_dependent.sh安装依赖

3、cd work_tool; make 编译生成业务库

4、cd ../alg_tool; make 编译生成算法库

5、cd ..; make  编译生成程序

6、cd bin; sh run.sh  启动程序

7、sh test.sh可以看到程序输出"welcome!"

<br>
##快速入门
快速使用本框架，请参考work_tool/src目录下的两个简单示例。

* example_work_interface 用于演示程序的输入和输出

* simple_frame_work_interface 包含了多种类型的数据获取

更详细的使用方法请参考《lab_common_so说明及使用文档》


<br>
##联系我们
`weibo_recom@staff.sina.com.cn`

[微博推荐博客](http://www.wbrecom.com/)

[微博推荐微博](http://weibo.com/u/5224446495?topnav=1&wvr=6&topsug=1)







