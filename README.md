# CoAVolFog

**简体中文** | [English](README.en.md)

[下载安装](#下载安装) · [游戏内设置](#游戏内设置) · [从源码构建](#从源码构建)

**CoAVolFog（体积雾与水面）** 为《魔兽世界》3.3.5a（12340）Direct3D 9 客户端添加体积雾和新版水面效果。原项目由 jealous-sound 开发。这个 fork 提供游戏内 **Ctrl+F7** 设置窗口的简体中文界面。

### 画面效果

- **体积雾**会随地形、时间和天气变化。经典版光照数据覆盖的区域使用对应的雾层，其他区域根据客户端的昼夜光照推算雾效。
- **光照**会在雾中散射，形成日月光晕。附近的点光源也能照亮雾气。进入建筑或洞穴时，室外雾和直射光会随镜头位置减弱。
- **新版水面**为湖泊、河流、海洋和室内水池加入波浪、折射、泡沫、倒影及日月高光。角色、生物、宠物和坐骑涉水时会产生涟漪和尾流。
- **可选效果**包括阳光光束、Forever 辉光、色彩分级、半透明特效雾和经典版散射能量归一化。雾与水面在驱动支持复制多重采样深度时可保留游戏的多重采样抗锯齿。

### 下载安装

从[汉化版构建页面](https://github.com/moshoutegong/coa-vfog-CN/actions/runs/43427238623)的 **Artifacts** 下载 `CoAVolFog-Win32-…`。这个版本已通过 Win32 构建和 CPU 测试，游戏内显示仍待实测。压缩包里的 `BUILD.txt` 记录构建提交和验证范围。

1. 完全退出游戏。使用 ArdenWoW 登录器时，先关闭它的「Volumetric fog and water」并完成原版模组的移除，再手动安装。登录器会核对它管理的文件，手动安装的汉化版可能会显示为冲突。
2. 将 `version.dll`、`CoAVolFog.dll`、`fogdata.bin` 和 `waterdata.bin` 放到游戏可执行文件所在目录。
3. 首次安装时，同时放入 `CoAVolFog.ini`。已有配置时先保留原文件，以便继续使用自己的设置。如果目录里有其他模组使用的 `version.dll`，请先确认该文件的用途再替换。
4. 重新进入游戏，按 **Ctrl+F7** 打开设置。卸载时，在退出游戏后移除手动安装的 `version.dll` 和 `CoAVolFog.dll`。

### 游戏内设置

- **Ctrl+F7** 打开或关闭设置窗口。笔记本电脑若默认将 F7 当作媒体键，可试 **Ctrl+Fn+F7**。快捷键也可以在 `CoAVolFog.ini` 中通过 `OverlayKey` 修改。
- 画质、雾层与密度、光照、水面等选项带有中文悬停说明。大部分修改会在下一帧预览。点击 **保存** 才会把修改写入 `CoAVolFog.ini`，点击 **还原** 可重新读取文件。
- 可拖动窗口标题栏移动窗口，也可拖动右下角改变大小。窗口会在本次游戏会话中记住位置和尺寸。按住 **Ctrl** 单击滑块可直接输入数值。
- 窗口显示雾与水面的渲染状态，以及抗锯齿启用情况。启用多重采样后需重启游戏。关闭多重采样会在下一次重置显示时生效，例如调整游戏分辨率时。
- `Enable`、`EngineHooks`、`Overlay` 和 `OverlayKey` 可在配置文件中调整。所有配置键及其默认值见英文文档的 [Settings](README.en.md#settings)。

### 从源码构建

需要 Visual Studio 2022 的 x86 C++ 工具链、包含 `fxc.exe` 的 Windows 10/11 SDK，以及 CMake 3.20 或更新版本。首次配置会下载固定版本的 Dear ImGui。

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A Win32
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

构建文件位于 `build/Release`。若要自行构建中文版，请检出 [`zh-cn-settings` 分支](https://github.com/moshoutegong/coa-vfog-CN/tree/zh-cn-settings)后运行上述命令。简体中文字形内嵌在 `CoAVolFog.dll` 中，字体授权见 [CoAVolFog Sans 许可证](https://github.com/moshoutegong/coa-vfog-CN/blob/zh-cn-settings/data/CoAVolFogSans-OFL.txt)。`ctest` 的完整检查需要可用的 D3D9 设备。只运行无需显卡的检查时，在 CTest 命令末尾加 `-LE gpu`。

### 兼容性与排查

- 设置窗口与渲染效果使用 D3D9。使用 `gxApi d3d9ex` 或显卡缺少 INTZ 深度纹理支持时，窗口可能无法显示。
- 帧率下降时可以分别调低 **画质** 和 **水面画质**。多重采样能否保留取决于显卡驱动的深度复制能力。
- 游戏目录下的 `CoAVolFog.log` 会记录加载状态、渲染跳过的原因和抗锯齿检测结果。Steam Deck 与 Linux 上的 Proton 配置见 [Steam Deck and Linux (Proton)](README.en.md#steam-deck-and-linux-proton)。
- 项目采用 [GNU GPL v3](LICENSE)。渲染原理、数据来源和客户端地址等技术细节见[英文文档](README.en.md)。
