# 北斗083安卓客户端

上游：[speedyHKjournalist/OpenMapleClient](https://github.com/speedyHKjournalist/OpenMapleClient)，下载提交 `7dd2f8f051e1158766964828d5a30a9c6ba187c4`。

这个客户端使用 GMS v83 原生 TCP 协议，服务端使用现有 BeiDou083 WSL 实例。客户端的连接配置、模拟器构建与网络分段处理已作本地适配。源码和许可证保留在本目录。

## 文件位置

- APK：`app/build/outputs/apk/debug/app-debug.apk`。
- NX 资源：`D:/MapleStory/investigations/android083-20261028/nx`。
- 资源大小及 SHA-256：`D:/MapleStory/investigations/android083-20261028/nx-manifest.json`。
- 计划、构建日志、测试证据：`D:/MapleStory/investigations/android083-20261028`。

本次已在 Android 14 模拟器中完成注册、登录、创建角色、切入 17575 频道服、进入蘑菇村以及键盘/触屏移动。验收记录见 `D:/MapleStory/investigations/android083-20261028/RESULT.md`；本地复验账号见同目录的 `TEST_ACCOUNT.md`。

APK 包含 ARM64 与 x86_64 两种原生库，最低 Android 12（API 31）；本次测试环境是 Android 14。APK 为本机调试签名。

## 模拟器启动

本机新增的模拟器名为 `BeiDou083_API34`，ADB 标识为 `emulator-5556`，数据盘 32 GB，配置和磁盘位于本次 investigations 目录中。

```powershell
cd D:\MapleStory\OpenMapleClient
.\scripts\start-beidou-emulator.ps1
```

脚本会安装 APK 并映射北斗四个游戏端口。使用脚本的 adb reverse 时，在启动页填写 `127.0.0.1`、`18484`，勾选“频道连接使用相同服务器地址”。

也可直接填写 `10.0.2.2`、`18484`，经 Windows 到 WSL 的 localhost 转发连接；本次已验证握手。

应用启动页提供“检测连接”和“开始游戏”。检测连接仅检查 v83 握手，不能代替账号登录或进入地图的验证。

资源尚未复制时，安装 APK 后执行一次：

```powershell
.\scripts\install-nx.ps1 -ResourcePath D:\MapleStory\investigations\android083-20261028\nx
```

## 安卓手机

安装同一个 APK，将 19 个 NX 文件放在以下目录的第一层：

```text
/storage/emulated/0/Android/data/com.OpenMapleClient/files/
```

现有北斗 PC 客户端的 `Data/*.img` 不能直接替代 NX。资源来自上游 README 指向的 [NXFiles](https://drive.google.com/drive/folders/1vloXiDOAQOUPfSO3aCjRBJDbtQ1PZTuU)，使用上游要求的 v83 和新版 UI/地图混合资源。

USB 调试连接手机后，可以给安装脚本传入手机的 ADB 标识。再给手机执行：

```powershell
adb -s <手机标识> reverse tcp:18484 tcp:18484
adb -s <手机标识> reverse tcp:17575 tcp:17575
adb -s <手机标识> reverse tcp:17576 tcp:17576
adb -s <手机标识> reverse tcp:17577 tcp:17577
```

此时手机填写 `127.0.0.1:18484` 并启用相同频道地址。USB 调试转发需要连接电脑；关闭后转发失效。

通过 Wi-Fi 独立连接时，填写手机可访问的服务端 IP；Windows/WSL 需要能从局域网访问上述四个游戏端口。`10.0.2.2` 只供安卓模拟器使用。当前测试没有修改 Windows 防火墙，也没有开放管理 API。

## 再次构建

Android Studio 打开本目录，或配置 `JAVA_HOME` 为 JDK 17 后执行：

```powershell
.\gradlew.bat :app:assembleDebug
```

依赖：Gradle 8.7、AGP 8.5.2、compile SDK 34、NDK 26.1.10909125、CMake 3.22.1。SDK 路径在本机的 `local.properties` 中。

## 实体键盘与触屏设置

支持 Android 识别的实体字母键盘，包括磁吸触点键盘保护套、USB 和蓝牙键盘，不依赖蓝牙连接状态判断。默认自动模式：实体键盘可用时隐藏虚拟摇杆和操作按钮，断开或系统报告键盘不可用时恢复。方向键移动，Alt 跳跃，其他按键沿用游戏内键位绑定。

启动页的“键盘与触屏设置”，以及游戏内菜单 → “Touch”，均可进入设置页，查看当前键盘状态并选择“自动”“始终显示”“始终隐藏”。选择自动保存，返回游戏生效。隐藏仅影响触屏摇杆和操作按键，顶部状态信息与功能入口仍保留。当前不提供布局拖动编辑。

输入设备切换、页面失焦和打开输入框时清理按下状态，避免拔出键盘后持续移动。实体键盘可用时不主动弹出软键盘；输入仍沿用客户端现有文字输入实现。

已在 Android 14 模拟器登录北斗并验证移动、方向与跳跃组合、设备接入/移除、按住方向键拔出后停止、手动显示/隐藏与保存、切后台恢复。具体磁吸保护套以及 ARM64 平板仍需真机验证。记录见 `D:\MapleStory\investigations\hardware-keyboard-20261028\PLAN_AND_RESULT.md`。

## 手机和平板分辨率

游戏内齿轮 → Options → Graphics → Resolution，选择后点 OK，立即生效并保存；下次进入地图恢复所选档位。

保留原6档，新增1280×800、1600×900、1600×1000、1440×1080、1600×1200、1560×720、1600×720，共13档。手机19.5:9/20:9可选1560×720/1600×720，16:10平板可选1280×800或1600×1000，4:3平板可选1024×768、1440×1080或1600×1200。

这是逻辑游戏分辨率，不要求匹配物理屏幕像素。较低档位使人物、文字与按钮更大；较高档位能看到更大地图范围。选定比例与屏幕不同则居中留边，保持画面比例。画面、触摸坐标、底栏、地图背景和虚拟按键同步刷新；游戏登录画面保留原有布局。

设置存储在应用私有目录 `files/Settings`，每台设备各自保存，不同步到账号。卸载或清除应用数据会删除设置。虚拟按键跟随档位定位，此次未加入自由拖动或独立尺寸设置。

打开功能菜单、Options、背包等窗口或聊天输入框时，虚拟操作覆盖层暂时隐藏，避免抢走窗口点击或遮挡文字输入；关闭后按原显示设置恢复。状态信息与地图小地图位于左上，商城、背包、菜单位于右上；底部移除了空的快捷栏背景，保留聊天栏和经验条。移动操作位于左下，攻击、跳跃、拾取和0–6快捷键位于右下，使用半透明圆形控件。

多分辨率调研与验收记录：`D:\MapleStory\investigations\resolution-20261028\PLAN_AND_RESULT.md`。

移动 HUD 参考 MapleStory M 官方的游戏主界面与菜单示意图：左上状态区、右上常用入口、左下移动区和右下战斗区。实现记录与模拟器截图见 `D:\MapleStory\investigations\mobile-hud-implementation-20261028\PLAN_AND_RESULT.md`。

## 已知限制

上游明确注明图形问题、地图问题和任务未实现；本次接入不承诺完整复现 PC 客户端，也不等于现有北斗中文 IMG、自定义地图和扩展玩法已经适配。具体实测结果以本次 investigations 目录内的验收记录为准。

创建角色时请选择 v83 支持的 Explorer、Cygnus Knights 或 Aran。上游借用了新版职业选择界面，其中显示的其他职业不代表北斗支持这些职业。登录界面背景沿用上游布局，进入地图后为全屏。
