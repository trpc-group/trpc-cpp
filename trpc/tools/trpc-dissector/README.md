[English](./README.md)

# 前言

tRPC-Dissector是tRPC协议的Wireshark解析器，提供了一对一应答的帧头与ProtoBuf包头解析.

# 使用说明

1. 解析器脚本载入Wireshark-Lua脚本目录（trpc-cpp/trpc/tools/trpc-dissector/tRPC_dissector.lua)。
   
   - 打开Wireshark，“帮助”选项卡->关于Wireshark->“文件夹”选项卡->个人Lua插件，将Lua脚本载入该目录。
   - 根据项目服务端口配置，修改Lua脚本中server_port参数。
   - 重新载入Lua脚本。

2. 导入proto。
   
   - “编辑”选项卡->首选项->Protocols->ProtoBuf，将.proto文件加入ProtoBuf搜索路径。
   
   - 勾选“Show details of message, fields and enums.”与“Show all fields of bytes type as string.”选项。

# 解析效果

![](C:\Users\chenzhuozhuo\AppData\Roaming\marktext\images\2024-09-30-14-21-17-image.png)
