# Morse Vibe

[English](README.md) | **日本語** | [简体中文](README.zh-CN.md)

Morse Vibe は M5StickS3 を Codex Micro / Vibewatch 互換のモールス操作端末に
するファームウェアです。Button A を押すと、短点は短い「トン」、長点は長い
「ツー」として 880 Hz の歯切れよい音が鳴ります。画面には入力中の符号、
復号文字、送信コマンド、BLE 接続、バッテリーを表示します。

本プロジェクトは MIT ライセンスの
[GOROman/vibewatch](https://github.com/GOROman/vibewatch) を基にしています。

## 操作

| 操作 | 動作 |
|---|---|
| KEY モードで Button A | モールスキー。押している間だけ 880 Hz を発音 |
| MIC モードで Button A | 周囲ノイズの 1 秒再校正 |
| Button B 1 回 | 強制確定。短縮数字と同じ符号なら数字側を選択 |
| Button B 2 回 | 入力中の符号を消去 |
| Button B を 800 ms 長押し | KEY / MIC 切替 |
| 起動中に A+B を 3 秒長押し | BLE ボンドを消去して再ペアリング |

3 単位の無入力で自動確定した場合は必ず英字として扱います。短縮数字を
使う場合だけ、符号を入力して自動確定前に Button B を押します。

- `.-` + 自動確定: `A` -> AI
- `.-` + Button B: 短縮 `1` -> Agent 1
- `-.` + 自動確定: `N` -> NG
- `-.` + Button B: 短縮 `9` -> 未割当として表示

## コマンド

| 復号 | BLE イベント | 用途 |
|---|---|---|
| F | ACT06 | Fast |
| O | ACT07 | OK / Approve |
| N | ACT08 | NG / Decline |
| P | ACT09 | Plan 切替 |
| A | ACT12 | AI / Codex |
| M | ACT10 + ACT11 | ホストのマイク切替。30 秒で安全解除 |
| 1-6 | AG00-AG05 | Agent 1-6 |

ACT09 と ACT12 の実動作は、各 macOS / Windows ホストの Codex Micro 設定で
Plan と目的の AI 操作に割り当ててください。未割当文字は表示のみで送信しません。

短縮数字は `0=T, 1=A, 2=U, 3=V, 4=4, 5=E, 6=6, 7=B, 8=D, 9=N`
の対応です。

## MIC モード

内蔵マイクで 600-1000 Hz の連続したビープ音を検出します。音声で発音した
「トン」「ツー」は対象外です。StickS3 はマイクとスピーカーを同時利用できない
ため、MIC モード中はキー音を停止します。

## ビルド

```sh
python -m platformio test -e native
python -m platformio run -e m5stack-sticks3
```

実機へ書き込む場合のみ、USB 接続後に次を実行します。

```sh
python -m platformio run -e m5stack-sticks3 --target upload
```

ネイティブテスト、ファームウェアビルド、実機への書き込み、StickS3 自動認識、
PSRAM、NimBLE 起動、および `morse-v1.0` の起動表示は合格済みです。実機の表示、
キー音、マイク、消費電力、および macOS / Windows での BLE 操作は
`USER_REVIEW` です。書き込み記録は
[docs/flash-verification.md](docs/flash-verification.md) にあります。
