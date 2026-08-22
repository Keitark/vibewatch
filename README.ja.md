# Morse Vibe

[English](README.md) | **日本語** | [简体中文](README.zh-CN.md)

Morse Vibe は M5StickS3 を Codex Micro / Vibewatch 互換のモールス操作端末に
するファームウェアです。Button A またはGrove接続した実物の電鍵を押すと、
短点は短い「トン」、長点は長い
「ツー」として 880 Hz の歯切れよい音が鳴ります。画面には入力中の符号、
復号文字、送信コマンド、BLE 接続、バッテリーを表示します。

本プロジェクトは MIT ライセンスの
[GOROman/vibewatch](https://github.com/GOROman/vibewatch) を基にしています。

## 操作

| 操作 | 動作 |
|---|---|
| KEY モードで Button A | モールスキー。押している間だけ 880 Hz を発音 |
| KEY モードで Grove 電鍵 | GPIO10 の乾接点入力。Button A と同じ復号・発音 |
| Button A または Grove 電鍵を 1.2 秒長押し | 符号を入力せず SIMPLIFIED / NORMAL 復号モード切替 |
| MIC モードで Button A | 周囲ノイズの 1 秒再校正 |
| Button B 1 回 | 現在の復号モードで即時確定 |
| Button B 2 回 | 入力中の符号を消去 |
| Button B を 800 ms 長押し | KEY / MIC 入力切替 |
| Button A + Button B を1.5秒長押し | Codex操作 / モールストレーナー切替。操作は符号として入力されない |
| 起動中に A+B を 3 秒長押し | BLE ボンドを消去して再ペアリング |

## モールストレーナー

動作中にButton AとButton Bを同時に1.5秒長押しすると、単独動作するトレーナーへ
切り替わります。画面が `MORSE TRAIN` になります。Button AまたはGrove電鍵を
1.2秒長押しすると、トレーナー専用のSIMPLE/NORMAL設定を切り替えます。SIMPLEでは
0-9を短縮数字モールスで、NORMALではA-Z/0-9を標準モールスで出題します。
Button A、Grove電鍵、またはマイクのトーンで回答し、無入力タイムアウトまたは
Button Bで確定します。

正解なら短い確認後に次の問題へ進みます。不正解または無効な回答は
`RETRY 1/3`、`RETRY 2/3` と表示し、3回目の失敗で正しいモールス符号を2.2秒表示して
から自動的に次へ進みます。トレーナーを終了するとCodex側の復号モードへ戻ります。

トレーナーの回答判定は端末内だけで行い、Codex MicroのBLEコマンドへ変換・送信
しません。ペアリングしていない状態でも練習できます。同じA+Bの1.5秒長押しで
Codex操作へ戻ります。両ボタンを離すまで組み合わせ操作を消費するため、誤って
長点、確定、消去、KEY/MIC切替が発生しません。

## Grove 電鍵

無電圧・通常開のストレートキーを、Groveの**白（GPIO10）**と**黒（GND）**の間に
接続します。赤（5 V）と黄（GPIO9）は接続せず、それぞれ個別に絶縁してください。
図解した配線図、3.5 mm TSプラグの場合、および安全上の注意は
[Grove 電鍵接続図](docs/grove-key-connection.md)にまとめています。
Button AとGrove電鍵は併用でき、両方を押した場合は両方を離すまでキーアップしません。

起動時は SIMPLIFIED（短縮数字）モードです。Button A または Grove 電鍵を
1.2 秒長押しすると NORMAL（英字）モードへ切り替わり、もう一度長押しすると
SIMPLIFIED に戻ります。この長押しはモード操作として消費され、長点にはなりません。
Button B の短押しは、現在のモードを変えずに即時確定します。

新しい文字の最初の打鍵で、前回の復号文字とアクション表示を消去します。入力中は
未確定の符号をリアルタイム表示し、押下中は末尾の `_` で示します。キーを離すたびに、
その時点の現在モードによる文字（例: SIMPLIFIEDでは `.-` -> `1`、NORMALでは
`.-` -> `A`）を `PENDING` として仮表示します。3単位の自動タイムアウト、または
Button B の短押しで文字が確定するまでコマンドは送信しません。

押している間は `_` で接点ONを示します。押下時間が、キーを離したときに長点と
判定するのと同じ2単位へ達すると、仮の符号と現在モードの文字をその場で更新します
（例: SIMPLIFIEDでは `-_` と `0 PENDING`）。この仮表示は復号結果を変更せず、
送信もしません。そのまま1.2秒まで押し続けると仮表示を取り消してモードを切り替えます。

- SIMPLIFIED: `.-` -> 短縮 `1` -> Agent 1
- NORMAL: `.-` -> `A` -> 未割当として表示
- SIMPLIFIED: `-.` -> 短縮 `9` -> 未割当として表示
- NORMAL: `-.` -> `N` -> 未割当として表示

## コマンド

| 復号 | BLE イベント | 用途 |
|---|---|---|
| F | ACT06 | Fast |
| O | ACT07 | OK / Approve |
| X | ACT08 | NG / Decline |
| P | ACT09 | Plan 切替 |
| C | ACT12 | AI / Codex |
| M | ACT10 + ACT11 | ホストのマイク切替。30 秒で安全解除 |
| 1-6 | AG00-AG05 | Agent 1-6 |

ACT09 と ACT12 の実動作は、各 macOS / Windows ホストの Codex Micro 設定で
Plan と目的の AI 操作に割り当ててください。未割当文字は表示のみで送信しません。

短縮数字は既定で `0=T, 1=A, 2=U, 3=V, 4=4, 5=E, 6=6, 7=B, 8=D, 9=N`
の対応です。英字側を選ぶ場合は Button A または Grove 電鍵を 1.2 秒長押しして
NORMAL に切り替えます。

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

ネイティブテスト12件、`morse-v1.3` ビルド、COM3への実機書き込み、書き込みハッシュ、
起動、BLE再接続、RPC交換は合格済みです。Button A / Grove電鍵の1.2秒長押しによる
SIMPLIFIED / NORMAL切替は `USER_REVIEW` です。以前の
`morse-v1.0` 実機書き込み、StickS3 自動認識、PSRAM、NimBLE 起動も合格済みです。
`morse-v1.1` の実機書き込みと macOS での Codex Micro 認識はユーザー報告で
合格しています。短縮数字の動作確認、Grove電鍵、
表示、キー音、マイク、消費電力、および全コマンドのホスト試験は
`USER_REVIEW` です。書き込み記録は
[docs/flash-verification.md](docs/flash-verification.md) にあります。
