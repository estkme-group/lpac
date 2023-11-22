# LPAC

[English](README_en.md)

lpac 是一个跨平台的本地 Profile 代理程序。致力于成为兼容性最好，功能最强大，代码最简洁的 LPA 程序。

下载地址: 
[Github Release](https://github.com/estkme-group/lpac/releases/latest)

加入我们的 [Telegram 群组](https://t.me/estkme)进行讨论！

特性：
- 支持激活码(Activate Code)和确认码(Confirm Code)
- 支持自定义请求用的 IMEI
- Profile 管理 -- 启用，禁用，删除，设置昵称等
- Discovery 功能(Push Model) -- 查询下载电信业者推送的配置文件
- Notification 管理 -- 查询，发送，删除
- 查看 eUICC 元数据 -- 查询 eUICC 的 eID，根 SM-DP+ 服务器和 SM-DS 服务器地址
- etc


## 目录
- [LPAC](#lpac)
  - [目录](#目录)
  - [编译](#编译)
  - [使用](#使用)
    - [Linux pcscd](#linux-pcscd)
    - [图形前端](#图形前端)
    - [库说明](#库说明)
    - [CLI](#cli)
    - [返回值](#返回值)
    - [命令选项](#命令选项)
      - [info](#info)
      - [profile](#profile)
      - [notification](#notification)
      - [download](#download)
      - [defaultsmdp](#defaultsmdp)
      - [purge](#purge)
      - [discovery](#discovery)
  - [FAQ](#faq)
  - [贡献](#贡献)
  - [TODO 列表](#todo-列表)
  - [致谢](#致谢)
  - [许可](#许可)


## 编译

<details>
<summary>Linux</summary>

- Debian/Ubuntu

```bash
sudo apt install build-essential cmake git g++ libpcsclite-dev libcurl4-openssl-dev
git clone --depth=1 https://github.com/estkme-group/lpac
cd lpac && mkdir build && cd build 
cmake .. 
make
```
编译后的二进制在 output 目录内. 
</details>


<details>
<summary>macOS</summary>

- 您需要先安装 [Homebrew](https://brew.sh/ "Homebrew").
- 依赖安装--WIP
```bash
git clone --depth=1 https://github.com/estkme-group/lpac
cd lpac && mkdir build && cd build 
cmake .. 
make
```
编译后的二进制在 output 目录内. 
</details>


<details>
<summary>Windows</summary>

- 在 Linux 上编译

```bash
sudo apt install build-essential cmake git g++ libpcsclite-dev libcurl4-openssl-dev gcc-mingw-w64 g++-mingw-w64
git clone --depth=1 https://github.com/estkme-group/lpac
cd lpac && mkdir build && cd build
cmake -DLINUX_MINGW32=ON .. && make
# 下载 libcurl
wget https://curl.se/windows/dl-8.4.0_6/curl-8.4.0_6-win64-mingw.zip -O curl.zip && unzip curl.zip && mv curl-8.4.0_6-win64-mingw/bin/libcurl-x64.dll output/libcurl.dll
```
编译后的二进制在 output 目录内.

- 在 Windows 上编译(MSYS2)

```bash
# MSYS2 MINGW64 环境
pacman -S mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja mingw-w64-x86_64-gcc
git clone --depth=1 https://github.com/estkme-group/lpac
cd lpac && mkdir build && cd build
cmake -DLINUX_MINGW32=ON .. && ninja
# 下载 libcurl
wget https://curl.se/windows/dl-8.4.0_6/curl-8.4.0_6-win64-mingw.zip -O curl.zip && unzip curl.zip && mv curl-8.4.0_6-win64-mingw/bin/libcurl-x64.dll output/libcurl.dll
```
编译后的二进制在 output 目录内.
</details>


## 使用

### Linux pcscd

在 Linux 下访问 PCSC 读卡器需要安装 `pcscd` 并启动。
```bash
sudo apt install pcscd
sudo systemctl start pcscd
```

### 图形前端

- [lpa-agent](https://github.com/estkme-group/lpa-agent): 基于 Web 的 lpac 前端

### 库说明
需要通过 `APDU_INTERFACE` 和 `HTTP_INTERFACE` 环境变量告诉 lpac 使用的 APDU 库和 HTTP 库。库文件在 lpac 程序目录下，根据系统不同，后缀为 `dll`, `so`, `dylib`

APDU:
- `libapduinterface_at` 适用于 LTE 模块的 `/dev/ttyUSB0` 的 AT 指令接口
- `libapduinterface_pcsc` PCSC 读卡器接口
- `libapduinterface_stdio` 标准输入输出

HTTP:
- `libhttpinterface_curl` curl http库
- `libhttpinterface_stdio` 使用标准输入输出的 HTTP 接口，开发中
  
下面以使用 PCSC 读卡器和 cURL 库下载 profile 的典型使用场景为例定义环境变量：
<details>

<summary>Windows(使用PowerShell)</summary>

   ```
		$env:APDU_INTERFACE=".\libapduinterface_pcsc.dll"
		$env:HTTP_INTERFACE=".\libhttpinterface_curl.dll"
   ```

</details>

<details>

<summary>Linux</summary>

   ```
		export APDU_INTERFACE=./libapduinterface_pcsc.so
		export HTTP_INTERFACE=./libhttpinterface_curl.so
   ```

</details>

<details>

<summary>macOS</summary>

   ```
		export APDU_INTERFACE=./libapduinterface_pcsc.dylib
		export HTTP_INTERFACE=./libhttpinterface_curl.dylib
   ```

</details>

### CLI
lpac 的命令格式如下
```
lpac <选项1> [选项2] [参数]
  选项1:
    info		查看您的 eUICC 卡片的信息
    profile		管理您的 eUICC 卡片的配置文件
    notification	管理您的 eUICC 卡片内的通知
    download		下载配置文件
    defaultsmdp		修改您的 eUICC 的默认 SM-DP+ 服务器
    purge		清空卡片，慎用
    discovery		执行 discovery 请求
  选项2:
    请参考下面详细的指令介绍
```

### 返回值
lpac 的指令返回内容均为 json 格式，所有指令的返回都遵守下面的格式。
```json
{
   "type":"lpa",
   "payload":{
      "code":0,
      "message":"success",
      "data":{[]}
   }
}
```
说明：
- `"type":"lpa"` 固定内容
- `code`为 0 表示成功执行，其它为错误码
- `message` 操作成功为 success，出现错误则返回错误类型
- `data` 操作成功时返回需要的内容，错误时则为空（但不是 NULL）


### 命令选项

#### info

查看 eUICC 卡的 EID，默认 SM-DP+ 服务器和 SM-DS 服务器.

<details>

<summary>以下是一个返回示例</summary>

```json
{
   "type":"lpa", 
   "payload":{
      "code":0, 
      "message":"success", 
      "data":{
         "eid":"这里是EID", 
         "default_smds":"testrootsmds.example.com", 
         "default_smdp":""
      }
   }
}
```

</details>

#### profile
Profile 管理，您可以枚举(list)，设置别名(rename)，启用(enable)，禁用(disable)和删除(delete)配置文件. 
```
lpac profile <选项> [参数]
	选项:
		list			枚举出您的 eUICC Profile
		rename			给指定的 Profile 设置别名
		示例: lpac profile rename <Profile 的 ICCID> <别名>
		enable			启用指定的 Profile，括号内为 RefreshFlag 状态，默认启用，可省略
		示例: lpac profile enable <Profile 的 ICCID/AID> (1/0)
		disable			禁用指定的 Profile，括号内为 RefreshFlag 状态，默认启用，可省略
		示例: lpac profile disable <Profile 的 ICCID/AID> (1/0)
		delete			删除指定的 Profile
		示例: lpac profile delete <Profile 的 ICCID/AID>
```
删除 Profile 操作无二次确认，请谨慎执行( 提示:本功能仅会删除 Profile 并签发 Notification，但是不会自动发送，您需要手动发送之. )
<details>

<summary>以下为 lpac profile list 的返回示例</summary>

```json
{
   "type":"lpa", 
   "payload":{
      "code":0, 
      "message":"success", 
      "data":[
         {
            "iccid":"8999990000... ",
            "isdpAid":"A0000005591010...",
            "profileState":1,
            "profileNickname":"tel-u",
            "serviceProviderName":"eSIM",
            "profileName":"NEWARE_CUG_V001",
            "profileClass":2
         }, 
         {
            "iccid":"894447860000...", 
            "isdpAid":"A0000005591011...", 
            "profileState":0,
            "serviceProviderName":"BetterRoaming", 
            "profileName":"BetterRoaming", 
            "profileClass":2
         }
      ]
   }
}
```
其中：
- `iccid`: Profile 的 ICCID
- `isdpAid`: Profile 的 Aid
- `profileState`: Profile 的状态，1 为启用，仅在设备支持 MEP(Multiple Enabled Profiles) 时才能启用多个 Profile
- `profileNickname`: Profile 的别名
- `serviceProviderName`: Profile 的电信业者名字
- `profileName`: Profile 的名字
- `profileClass`: Profile 版本

</details>

#### notification
用于 Notification 的管理，Notification 是在对 Profile 的操作过程中由电信业者发送的。您可以枚举(list)，发送(process)，移除(remove) Notification.
```
lpac notification <选项> [参数]
	选项:
		list                枚举出您的 eUICC 待发 Notification 列表
		process             发送 Notification 报告
		示例: lpac notification process <序列ID>
		remove              移除 Notification 报告
		示例: lpac notification remove <序列ID>
```
注意: 下游开发或者最终用户应该在存在 Notification 报告时尽快处理，以遵守 GSMA 规范。lpac 发送 Notification 后不会自动删除 Notification，需要手动进行删除操作。 

<details>

<summary>以下为 lpac notification list 的返回示例</summary>

```json
{
   "type":"lpa", 
   "payload":{
      "code":0, 
      "message":"success", 
      "data":[
         {
            "seqNumber":7,
            "profileManagementOperation":32,
            "notificationAddress":"rsp-eu.simlessly.com",
            "iccid":"8999990000"
         }, 
         {
            "seqNumber":8, 
            "profileManagementOperation":64, 
            "notificationAddress":"rsp.truphone.com", 
            "iccid":"894447860000"
         }
      ]
   }
}
```
其中：
- `seqNumber`: 序列 ID
- `profileManagementOperation`: Profile 状态标识
- `notificationAddress`: Profile 的通知报告服务器地址

</details>

#### download
连接 SM-DP+ 服务器完成 Profile 的下载(Pull Model)

需要以下额外的环境变量才能完成：
- `SMDP`: SMDP服务器，必填且不能为空
- `MATCHINGID`: 激活码，**必须要有**但是可留空
- `CONFIRMATION_CODE`: 确认码，可选
- `IMEI`: 欲下载Profile的设备的IMEI,可选

设置完环境变量后执行 `lpac download` 下载 Profile

这里用 BetterRoaming 的 公共 Profile 举例：
```
SMDP=rsp.truphone.com MATCHINGID="QR-G-5C-1LS-1W1Z9P7" CONFIRMATION_CODE=""  IMEI="" ./lpac download
```

#### defaultsmdp
用于设置 eUICC 的默认 SM-DP+ 服务器。

`lpac defaultsmdp <sm-dp+ 服务器地址>`

若您需要查看您的默认 SM-DP+ 服务器地址，请使用 info 功能. 

#### purge
用于使 eUICC 恢复出厂状态，会恢复默认配置并清空所有 Profile，且不会发送 Notification 报告。慎用！

用法：`lpac purge yes`

#### discovery
用于连接 SM-DS 服务器完成 Profile 的推送查询(Push Model)

可以设置以下环境变量以自定义IMEI和SM-DS服务器：
- `IMEI`: 欲自定义的IMEI，可不注入
- `SMDS`: 欲请求的SM-DS服务器，若为空则为 gsma 官方服务器"lpa.ds.gsma.com"

示例：`SMDS=lpa.ds.gsma.com IMEI="" ./lpac discovery`


## FAQ

<details>
<summary>Q: 执行 lpac 任意指令都返回类似 "SCardListReaders() failed:" 加8位的错误码，例如 80100069, 80100066, 8010002E, 8010000F等</summary>

    A:80100069 意味你没插好你的 UICC;
       80100066 意味你的卡并未响应请求，请取下来擦拭触点再次尝试插入;
       8010002E 意味通信错误;
       8010000F 意味您插入的卡并非 eUICC 芯片;
       其他错误码请善用搜索引擎，或者加群询问

</details>

<details>
<summary>Q: xxx (电信业者)的卡下不了啊?</summary>

    A: 电信业者的 SM-DP+ 服务器的验证是多种多样的，请检查您输入的参数是否与电信业者提供给您的相符，部分电信业者以推送形式下发 profile，可能需要用到 lpac 的 discovery 和自定义 IMEI 功能

</details>


## 贡献
欢迎 PR。如果遇到问题，可以[提 issue 报告](https://github.com/estkme-group/lpac/issues)。在提问前请先搜索类似的问题和 issue。也可前往 [TG 群组](https://t.me/estkme) 讨论


## TODO 列表
- 优化代码
- 继续完善文档
- stdio 文档待更新
- etc


## 致谢

<a href="https://github.com/estkme-group/lpac/graphs/contributors">
  <img src="https://contrib.rocks/image?repo=estkme-group/lpac"  alt=""/>
</a>

---

## 许可
AGPL-3.0
Copyright (c) 2023-2024 eSTKme Group
