# run_gdb.bat使用说明

##  使用命令的方式校验内存，dump/load数据方法

- 1.修改gdb.script文件内的ip地址为ocd_sever端ip地址
- 2.打开sever端ocd，运行run_gdb.bat
- 3.连接成功后可以运行以下命令

    - 读内存数据
    monitor mdw 0x1ff00000

    - 写内存数据
    monitor mww 0x1ff00000 0x1

    - load文件到内存
    restore g:/qiaoqiming/ci112x/test2 binary 0x1ff00000 0 1000

    - dump内存到文件
    dump memory g:/qiaoqiming/ci112x/test 0x1ff00000 0x1ff20000

### 其他更多命令请参考openocd命令和gdb命令手册

- openocd命令需要加前缀monitor
- gdb命令可以直接输入

注意在gdb上执行ocd命令的主体是sever端，所以在远程调试的情况下无法使用待host的命令，但gdb的带host命令可以使用
