# LPAC

致力于成为最强大的跨平台eSIM本地Profile代理. 

下载地址: 
[Github download](https://github.com/estkme-group/lpac/releases/latest "Github download")


lpac是一个跨平台的本地Profile代理程序. 这个项目致力于成为兼容性最好, 功能最强大, 干净的LPA程序. 连接SM-DP+服务器(Pull Model)支持提交激活码和确认码, 且允许为空, 并且支持定义请求用的IMEI;支持Discovery Push Model;支持Profile管理. 本项目尽量不使用外部依赖, 也因此可以在多种平台无需刻意配置运行环境即可运行, 开箱即用. lpac是公开开发的免费开源软件, 接受社区贡献. 

## 功能特性

- 开箱即用
- Profile 管理 -- 可以启用, 禁用, 删除, 设置昵称, 修改默认项
- Download 功能(Pull Model) -- 强大的 Profile 主动下载功能, 配置灵活
- Discovery 功能(Push Model) -- 方便查询当前eUICC可以被下载的配置文件
- Notification 管理 -- 查询, 发送, 删除
- eUICC元数据 -- 查询eUICC的eID, 根SM-DP+服务器和SM-DS服务器地址
- etc

------------

## 贡献
- 编程

    该项目开源解码贡献并欢迎 PR. 查看我们的 TODO 列表以了解未解决的问题, 尤其是“首先要解决的问题”. 

- 功能请求和讨论

    加我们的[Telegram群](https://t.me/estkme "Telegram群")并开始讨论. 

- Bug报告

    [在此报告问题](https://github.com/estkme-group/lpac/issues "在此报告问题"). 请善用搜索并搜索类似的问题和请求. 如果它并非异常但是还是需要反映一下的话, 请前往讨论群咨询. 

## FAQ

<details>

<summary>Q1: 执行 lpac 任意指令都返回类似 "SCardListReaders() failed:" 加8位的错误码, 例如以下 80100069, 80100066, 8010002E, 8010000F等</summary>

    A1:80100069 意味你没插好你的UICC;
       80100066 意味你的卡并未响应请求, 请取下来擦拭触点再次尝试插入;
       8010002E 意味通信错误;
       8010000F 意味您插入的卡并非eUICC芯片;
       其他错误码请善用搜索引擎, 或者加群询问. 

</details>

<details>

<summary>Q2: xxx (电信业者)的卡下不了啊?</summary>

    A2: 电信业者的 SM-DP+ 服务器的验证是多种多样的, 请检查您输入的参数是否与电信业者提供给您的相符, 若电信业者询问了您其他参数(比如 IMEI 等), 请手动提供. 并咨询电信业者寻求帮助

</details>


## 编译

<details>

<summary>Linux系</summary>

- 这里用Deb系举例.

```shell
sudo apt install pcscd build-essential cmake git g++ libpcsclite-dev
git clone --depth=1 https://github.com/estkme-group/lpac
cd lpac && mkdir build && cd build 
cmake -DLINUX_MINGW32=ON .. 
make
```

- 编译后的二进制在 output 目录内. 

</details>

<details>

<summary>macOS</summary>

- 您需要先安装[Homebrew](https://brew.sh/ "Homebrew").
- WIP

</details>

<details>

<summary>Windows</summary>

- 这里用Deb系举例.

```shell
sudo apt install pcscd build-essential cmake git g++ libpcsclite-dev mingw-w64
git clone --depth=1 https://github.com/estkme-group/lpac
cd lpac && mkdir build && cd build
cmake .. && make
```

- 编译后的二进制在 output 目录内. 

</details>

## 使用
### 概览和公共内容
- lpac的执行格式如下
```
lpac <选项1> [<选项2>] <参数>
  选项1:
    info		查看您的eUICC卡片的信息
    profile		管理您的eUICC卡片的配置文件
    notification	管理您的eUICC卡片内的通知
    download		下载配置文件
    defaultsmdp		修改您的eUICC的默认SM-DP+服务器
    purge		清空卡片, 慎用
    discovery		执行discovery请求
  选项2:
    略, 请参考下面详细的指令介绍
```
- lpac的指令返回格式均为json, 无论采用shell直接执行或采用stdio方式. 以下是公共类型, 所有指令的返回都遵守本公共格式. 
```json
{
   "type":"lpa",		// 类型:lpa, 这是固定内容. 
   "payload":{
      "code":0,			/* code 为 0 代表指令被成功执行, 为 -1 代表出现了错误 */
      "message":"success",	// 执行的返回消息, 成功则为 success , 出现错误则返回错误类型
      "data":{[]}		// 执行的返回主载体, 成功返回需要的内容, 错误时则为空(NUL, "", 但是与字符串null不同)
   }
}
```
- 环境变量
     lpac需要环境变量告诉主程序需要使用的pcsc库和curl库. 以下为公共环境变量, 给出的示例依赖文件就放在与主程序相同的目录. 导入公共变量后才能正确执行lpac主程序

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

### info

- 本功能仅仅用于查看目前的eUICC卡的内置参数信息, 您可以在这查看您的eUICC卡的EID, 默认SM-DP+服务器和SM-DS服务器.

`lpac info`

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

### profile
- 本功能用于Profile的管理, 您可以枚举(list), 设置别名(rename), 启用(enable), 禁用(disable)和删除(delete)配置文件. 
```
lpac profile [<选项2>] <参数>
	选项1:
		profile			这里是 profile 子项的介绍
	选项2:
		list			枚举出您的eUICC Profile
		rename			给指定的Profile设置别名
		示例: lpac profile rename <Profile的ICCID> <别名>
		enable			启用指定的Profile
		示例: lpac profile enable <Profile的ICCID>
		disable			禁用指定的Profile
		示例: lpac profile disable <Profile的ICCID>
		delete			删除指定的Profile
		示例: lpac profile delete <Profile的ICCID>
```
- 删除 Profile 操作与实体卡的丢弃行为并无二致, 指令无二次确认, 请谨慎执行( 提示:本功能仅会删除 Profile 并签发 Notification, 但是不会自动发送, 您需要手动发送之. )
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
            "iccid":"8999990000... ",        // Profile的ICCID
            "isdpAid":"A0000005591010...",   // Profile的Aid
            "profileState":1,                // Profile的状态, 1为启用
            "profileNickname":"tel-u",       // Profile的别名
            "serviceProviderName":"eSIM",    // Profile自己带的电信业者名字
            "profileName":"NEWARE_CUG_V001", // Profile自己带的名字
            "profileClass":2                 // Profile版本
         }, 
         {
            "iccid":"894447860000...", 
            "isdpAid":"A0000005591011...", 
            "profileState":0,                // 仅在设备支持MEP(Multiple Enabled Profiles)时才能启用多个Profile
            "serviceProviderName":"BetterRoaming", 
            "profileName":"BetterRoaming", 
            "profileClass":2
         }
      ]
   }
}
```

</details>

### notification
- 本功能用于 Notification 的管理, Notification 会根据Profile的设定, 在您进行任何操作的时候都有可能产生. 在这里, 您可以枚举(list), 发送(process), 移除(remove)通知报告. 
```
lpac notification [<选项2>] <参数>
	选项1:
		notification        这里是 Notification 子项介绍
	选项2:
		list                枚举出您的 eUICC 待发 Notification 列表
		process             发送 Notification 报告
		示例: lpac notification process <序列ID>
		remove              移除 Notification 报告
		示例: lpac notification remove <序列ID>
```
- 提示: 下游开发或者最终用户应该在存在 Notification 报告时尽快发出, 以遵守 GSMA 规范. 对于开发者, 发送通知报告后 lpac 不会自动删除其, 请务必记住需要手动删除. 

<details>

<summary>以下为 lpac notification list 的返回示例.</summary>

```json
{
   "type":"lpa", 
   "payload":{
      "code":0, 
      "message":"success", 
      "data":[
         {
            "seqNumber":7,                                 // 序列ID
            "profileManagementOperation":32,               // Profile 状态标识
            "notificationAddress":"rsp-eu.simlessly.com",  // Profile 的通知报告服务器地址
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

</details>

### download
- 本功能用于 Profile 的下载(Pull Model), 主动连接 SM-DP+ 服务器完成 Profile 的签发和下载. 
`lpac download`
- 本功能需要额外的环境变量才能完成, 以下用 Windows 举例.
```
  $env:SMDP=<SMDP服务器, 必填且不能为空>"
  $env:MATCHINGID=<激活码, 必须要有但是可留空>"
  $env:CONFIRMATION_CODE=<确认码, 可选>"
  $env:IMEI=<欲下载Profile的设备的IMEI,可选>"
```
- 温馨提示:正确注入环境变量后不要忘记执行".\lpac.exe download"以下载您的 Profile 喔.
	
- 以下是 \*nix 的下载示例, 这里用 BetterRoaming 的 公共 Profile 举例.
```
	SMDP=rsp.truphone.com MATCHINGID="QR-G-5C-1LS-1W1Z9P7" CONFIRMATION_CODE=""  IMEI="" ./lpac download
```

### defaultsmdp
- 本功能用于设置 eUICC 的默认 SM-DP+ 服务器, 以方便下载多张 Profile 使用. 
`lpac defaultsmdp <sm-dp+ 服务器地址>`
- 温馨提示:若您需要查看您的默认 SM-DP+ 服务器地址, 请执行 info 功能. 

### purge
- 本功能用于令 eUICC 恢复出厂状态, 会恢复默认配置, 会清空所有 Profile, 且不会签发 Notification 报告. 慎用!
```
lpac purge [<选项2>] <参数>
	选项1:
		purge				这里是 purge 子项介绍
	选项2:
		yes				执行恢复出厂状态
		非yes的任何内容			什么事都不做, 直接退出
```
- 在执行本操作前, 请您知道您正在做什么. 
### discovery
- 本功能用于 Profile 的推送查询(Push Model), 连接 SM-DS 服务器完成可下载 Profile 的查询. 
```
lpac discovery
```
- 本功能可以注入环境变量以支持自定义IMEI和SM-DS服务器, 以下为 Windows 示例.
```
$env:IMEI=<欲自定义的IMEI, 可不注入>"
$env:SMDS=<欲请求的SM-DS服务器, 若为空则为 gsma 官方服务器"lpa.ds.gsma.com">"
```
- \*nix系直接如下.
```
SMDS=lpa.ds.gsma.com IMEI="" ./lpac discovery
```
## TODO列表
- 优化代码
- 继续完善文档
- stdio文档待更新
- etc

## 最后感谢各位

<a href="https://github.com/estkme-group/lpac/graphs/contributors">
  <img src="https://contrib.rocks/image?repo=estkme-group/lpac"  alt=""/>
</a>

---
 
## 许可证
AGPL-3.0
Copyright (c) 2023-2024 eSTKme Group
