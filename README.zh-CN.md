# Morse Vibe

[English](README.md) | [日本語](README.ja.md) | **简体中文**

Morse Vibe 把 M5StickS3 变成兼容 Codex Micro / Vibewatch BLE 协议的摩尔斯
控制器。Button A 或连接在 Grove GPIO10 与 GND 之间的无源常开电键，均可提供
880 Hz 清脆侧音；长按 Button A 或 Grove 电键 1.2 秒可在简化数字与普通字母
解码模式之间切换，并且不会输入一个长划。

设备每次启动时默认使用无线电简化数字。长按 Button A 或 Grove 电键 1.2 秒切换到
普通字母模式；短按 Button B 只会按当前模式立即提交。例如：`.-` 在简化模式中是 `1`/Agent 1，
在普通模式中是字母 `A`。AI/Codex 命令已从 `A` 改为无冲突的 `C`，
NG/Decline 命令已从 `N` 改为 `X`。

控制和命令表、构建方法、配对步骤及当前验证状态请参阅
[English README](README.md)。实体 StickS3、macOS 和 Windows 的配对测试仍为
`USER_REVIEW`。

Grove 仅连接白线（GPIO10）与黑线（GND）；红线（5 V）和黄线（GPIO9）必须分别
绝缘且不连接。参阅[Grove 电键连接图](docs/grove-key-connection.md)。

本项目是 MIT 许可的
[GOROman/vibewatch](https://github.com/GOROman/vibewatch) 衍生项目。
