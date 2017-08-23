//
// NTSC TV表示デバイスの管理
// 作成日 2017/04/11 by たま吉さん
// 修正日 2017/04/13, 横文字数算出ミス修正(48MHz対応)
// 修正日 2017/04/15, 行挿入の追加
// 修正日 2017/04/17, bitmap表示処理の追加
// 修正日 2017/04/18, cscroll,gscroll表示処理の追加
// 修正日 2017/04/25, gscrollの機能修正, gpeek,ginpの追加
// 修正日 2017/04/28, tv_NTSC_adjustの追加
// 修正日 2017/05/19, tv_getFontAdr()の追加
// 修正日 2017/05/30, SPI2(PA7 => PB15)に変更
// 修正日 2017/06/14, SPI2時のPB13ピンのHIGH設定対策
// 修正日 2017/07/25, tv_init()の追加
// 修正日 2017/07/29, スクリーンモード変更対応
// 修正日 2017/08/19, TVscreenへの移行

#include <TTVout.h>
#include "tTVscreen.h"
#define NTSC_VIDEO_SPI 2

const int pwmOutPin = PB9;      // tone用 PWM出力ピン

/*
#define TV_DISPLAY_FONT font6x8
#include <font6x8.h>
*/
/*
#define TV_DISPLAY_FONT font6x8tt
#include <font6x8tt.h>
*/
/*
#define TV_DISPLAY_FONT font8x8
#include <font8x8.h>
*/

/*
#define TV_DISPLAY_FONT ichigoFont8x8 
#include <ichigoFont8x8.h>
*/

extern uint8_t* ttbasic_font;

uint8_t* tvfont;     // 利用フォント

uint8_t* vram;       // VRAM先頭
uint16_t f_width;    // フォント幅(ドット)
uint16_t f_height;   // フォント高さ(ドット)


// フォント利用設定 (この関数はNTSCを利用しない場合も利用する。現在TVscreenで機能重複)
void tv_fontInit() {
  tvfont = ttbasic_font;
  f_width  = *(tvfont+0);             // 横フォントドット数
  f_height = *(tvfont+1);             // 縦フォントドット数  
}

// フォントアドレス取得(NTSC以外でも使われている)
uint8_t* tv_getFontAdr() {
  return tvfont;
}

//
// 音の停止
// 引数
// pin     : PWM出力ピン (現状はPB9固定)
//
void tv_noToneEx() {
    Timer4.pause();
  Timer4.setCount(0xffff);
}

//
// PWM単音出力初期設定
//
void tv_toneInit() {
  pinMode(pwmOutPin, PWM);
  tv_noToneEx();
}

//
// 音出し
// 引数
//  pin     : PWM出力ピン (現状はPB9固定)
//  freq    : 出力周波数 (Hz) 15～ 50000
//  duration: 出力時間(msec)
//
void tv_toneEx(uint16_t freq, uint16_t duration) {
  if (freq < 15 || freq > 50000 ) {
    tv_noToneEx();
  } else {
    uint32_t f =1000000/(uint16_t)freq;
#if F_CPU == 72000000L
    Timer4.setPrescaleFactor(72); // システムクロックを1/72に分周
#else if  F_CPU == 48000000L
    Timer4.setPrescaleFactor(48); // システムクロックを1/48に分周
#endif
    Timer4.setOverflow(f);
    Timer4.refresh();
    Timer4.resume(); 
    pwmWrite(pwmOutPin, f/2);  
    if (duration) {
      delay(duration);
      Timer4.pause(); 
      Timer4.setCount(0xffff);
    }
  }
}

// 音の再生
void tv_tone(int16_t freq, int16_t tm) {
  //TV.tone(freq, tm);
  tv_toneEx(freq, tm);
}

// 音の停止
void tv_notone() {
  //TV.noTone();
  tv_noToneEx();    
}


