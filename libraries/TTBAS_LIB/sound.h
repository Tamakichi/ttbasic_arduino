//
// file: sound.h
// サウンド再生(Timer4 PWM端子 PB9を利用）
//
// 作成日 2017/10/25 by たま吉さん
//
  

#ifndef __sound_h__
#define __sound_h__

#include <Arduino.h>

void dev_toneInit() ;                              // PWM単音出力初期設定
void dev_tone(uint16_t freq, uint16_t duration) ;  // 音の停止
void dev_notone() ;                                // 音の停止
 
#endif
  