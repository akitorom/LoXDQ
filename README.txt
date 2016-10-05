http://chocobo256.blog.fc2.com/blog-entry-6.html

history 履歴

28 Sep. 2016 removed LoXDQ.dll.if you need it, compile LoXDQDll.cpp yourself.

Install インストール

Copy LoXDQ.exe and LoXDQ.ini to any directory. LoXDQ.exe と LoXDQ.ini を任意の場所にコピー。

Usage 使い方

Run LoXDQ.exe. If game process is already there, LoXDQ start searching log memory address. or wait game process. LoXDQ.exe を実行。ゲームのプロセスが既に存在したら、LoXDQ はすぐにログのアドレスをサーチ開始します。あるいはゲームのプロセスを待ちます。

Setting 設定

Edit LoXDQ.ini directly. LoXDQ.ini を直接編集。

LogFile

log file name w/ or w/o path. ログファイル名。パスを含んでもよい。

CheckPerSecond

new log check rate. if CheckPerSecond=10, LoXDQ will check new log each 100ms. You may feel CheckPerSecond=100 is too heavy. 新ログをチェックする割合。10 で 100ms ごと。100 は重いと思うでしょう。

BaseAddress
SystemOffset
ChatOffset

memory address that indicate logs and offset addresses. you must update these value after new DQX patch. ログのメモリ上のアドレスとオフセット。新しい DQX のパッチの後、あなた自身で変更する必要があります。

Format フォーマット

tab separated value(TSV) format that is compatible with DQ_ACT_Plugin. "date<tab>number<tab>flag<tab>id1<tab>id2<tab>char1<tab>char2<tab>body". <lf> (0x0a) in original log data is replaced to <tab> (0x09), so "body" may contain additional <tab>. タブ分離値(TSV)フォーマット。"日付<tab>番号<tab>フラグ<tab>ID1<tab>ID2<tab>名前1<tab>名前2<tab>本体". DQX_ACT_Pluginと互換性があります。

Source Code ソースコード

If you can not trust binary files, see source code files then compile them. バイナリァイル *.exe を信頼できなければ、ソースコードの *.cpp を調べてコンパイルしてください。

License ライセンス
GPLv3

Copyright (C) 2016 Voxxy256

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see .
