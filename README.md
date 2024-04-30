# FingerTrackerOSCXR

*********CAUTION*********

現状SteamVRがOpenXRの複数アプリの起動を許していないため、本アプリは機能しません。

（起動と同時にVRChatを終了させてしまいます。）

機能しないですが、参考用と、それでも何か用途があるかもしれないのと、供養のため公開します。

*********CAUTION*********

VRChatのOSCで指を動かせるようになる。(OpenXR移植版)

下記対応したデバイスが必要

XR_EXT_hand_tracking

XR_MND_headless

例：Quest2 + ALVR ※テスト環境

## Build from source

openxr_loader.lib to lib

Open with VisualStudio

Build

## Option

ip port sleepTime mode

muscle.txt：角度からUnityマッスルへの変換定数

num.txt：指の開き具合調整値。親指根本から30個、開き具合10個

mode：0:256bit, 1:80bit, 2:160bit(default)

## Avatar setting

アバターへOSCの設定が必要。下記参照※non xr ver

https://a3s.booth.pm/items/3689147

## Donate

https://a3s.booth.pm/items/3689147

## Twitter

https://twitter.com/A3_yuu