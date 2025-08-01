Nikon Type0025 Module SDK Revision.7 概要


■用途
 カメラのコントロールを行う。


■サポートするカメラ
 Z 50


■動作環境
 [Windows]
    Windows 10 64bit版
    Windows 11 64bit版
    ※"Visual Studio 2017 の Visual C++ 再頒布可能パッケージ"のインストールが必要

 [Macintosh]
    macOS 12.5(Monterey)
    macOS 13(Ventura)
    macOS 14(Sonoma)
    ※64bitモードのみ（32bitモードは非サポート）


■内容物
 [Windows]
    Documents
      MAID3(J).pdf : 基本インターフェース仕様
      MAID3Type0025(J).pdf : Type0025 Moduleで使用される拡張インターフェース仕様
      Usage of Type0025 Module(J).pdf : Type0025 Module を使用する上での注意事項
      Type0025 Sample Guide(J).pdf : サンプルプログラムの使用方法

    Binary Files
      Type0025.md3 : Windows用 Type0025 Module本体
      NkdPTP.dll : Windows用　PTPドライバ
      dnssd.dll ：Windows用　PTPドライバ
      NkRoyalmile.dll：Windows用　PTPドライバ

    Header Files
      Maid3.h : MAIDインターフェース基本ヘッダ
      Maid3d1.h : Type0025用MAIDインターフェース拡張ヘッダ
      NkTypes.h : 本プログラムで使用する型の定義
      NkEndian.h : 本プログラムで使用する型の定義
      Nkstdint.h : 本プログラムで使用する型の定義

    Sample Program
      Type0025CtrlSample(Win) : Microsoft Visual Studio 2017 用プロジェクト

 [Macintosh]
    Documents
      MAID3(J).pdf : 基本インターフェース仕様
      MAID3Type0025(J).pdf : Type0025 Moduleで使用される拡張インターフェース仕様
      Usage of Type0025 Module(J).pdf : Type0025 Module を使用する上での注意事項
      Type0025 Sample Guide(J).pdf : サンプルプログラムの使用方法
      [Mac OS] Notice about using Module SDK(J).txt : Mac OSで使用する上での注意事項

    Binary Files
      Type0025 Module.bundle : Macintosh用 Type0025 Module本体 
      libNkPTPDriver2.dylib : Macintosh用 PTP ドライバ 
      Royalmile.framework : Macintosh用 PTP ドライバ

    Header Files
      Maid3.h : MAIDインターフェース基本ヘッダ
      Maid3d1.h : Type0025用MAIDインターフェース拡張ヘッダ
      NkTypes.h : 本プログラムで使用する型の定義
      NkEndian.h : 本プログラムで使用する型の定義
      Nkstdint.h : 本プログラムで使用する型の定義

    Sample Program
      Type0025CtrlSample(Mac) : Xcode 13.2.1用のサンプルプログラムプロジェクト(BaseSDK : macOS)


■制限事項
 本Module SDKを利用してコントロールできるカメラは1台のみです。
 複数台のコントロールには対応していません。

■注意事項
 新SDKを使用する場合、dnssd.dll及びNkRoyalmile.dllが必要となります。
 以下のドキュメントをご参照の上、ご注意ください。
 - Type0025 Sample Guide(J).pdf
 「ファイル - Windows」
