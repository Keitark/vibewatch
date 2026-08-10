# Morse Vibe

[English](README.md) | [日本語](README.ja.md) | **简体中文**

Morse Vibe 把 M5StickS3 变成兼容 Codex Micro / Vibewatch BLE 协议的摩尔斯
控制器。Button A 是带 880 Hz 清脆侧音的电键；长按 Button B 可在按键输入和
600-1000 Hz 麦克风音调输入之间切换。

自动的三单位间隔始终按字母解释。若要选择与字母冲突的无线电缩略数字，请在
自动提交前按一次 Button B。例如：`.-` 自动提交是 `A`/AI，按 Button B 提交
则是缩略 `1`/Agent 1。

控制和命令表、构建方法、配对步骤及当前验证状态请参阅
[English README](README.md)。实体 StickS3、macOS 和 Windows 的配对测试仍为
`USER_REVIEW`。

本项目是 MIT 许可的
[GOROman/vibewatch](https://github.com/GOROman/vibewatch) 衍生项目。
