//
// 豊四季Tiny BASIC for Arduino STM32 変数型定義
// 2017/11/07 by たま吉さん
// 2018/08/18 by たま吉さん
//

#ifndef __ttbasic_types_h__
#define __ttbasic_types_h__

// システム環境設定情報構造体
typedef struct SystemConfig {
  int16_t NTSC;        // NTSC垂直同期補正 (0,1,2,3) または画面向き
  int16_t KEYBOARD;    // キーボード設定 (0:JP, 1:US)
  int16_t STARTPRG;    // 自動起動(-1,なし 0～9:保存プログラム番号)
  int16_t NTSC_HPOS;   // NTSC横表示開始位置補正(-16 ～ 16)
  int16_t NTSC_VPOS;   // NTSC横表示開始位置縦(-16 ～ 16)
} SystemConfig;

#endif