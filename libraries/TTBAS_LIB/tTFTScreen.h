//
// file: tTFTScreen.h
// ILI9341利用ターミナルスクリーン制御クラス
// 2017/08/12 修正 SPI2を利用に修正
// 2017/08/25 修正 グラフィック描画対応
// 2017/08/28 スクリーン用メモリに確保済領域指定対応
// 2018/08/18 修正 init()に横位置補正、縦位置補正引数の追加（抽象クラスとのインタフェース互換のため）
//

#ifndef __tTFTScreen_h__
#define __tTFTScreen_h__

//#include <Arduino.h>
#include "tGraphicScreen.h"
#include <SPI.h>
#include <Adafruit_GFX_AS.h>      // Core graphics library, with extra fonts.
#include <Adafruit_ILI9341_STM_TT.h> // STM32 DMA Hardware-specific library

// PS/2キーボードの利用 0:利用しない 1:利用する
#define PS2DEV     1  
#define TV_FONT_EX 1    // フォント倍率

class tTFTScreen :public tGraphicScreen {
 private:
  Adafruit_ILI9341_STM_TT* tft;
  uint16_t f_width;    // フォント幅(ドット)
  uint16_t f_height;   // フォント高さ(ドット)
  uint16_t fgcolor;
  uint16_t bgcolor;
  uint16_t fontEx;     // フォント拡大率
  uint16_t pos_gx;     // グラフィックカーソル
  uint16_t pos_gy;     // グラフィックカーソル
	
 protected:
    void INIT_DEV();                             // デバイスの初期化
    void WRITE(uint8_t x, uint8_t y, uint8_t c); // 文字の表示
    void CLEAR();                                // 画面全消去
    void CLEAR_LINE(uint8_t l);                  // 行の消去
    void SCROLL_UP();                            // スクロールアップ
    void SCROLL_DOWN();                          // スクロールダウン
    void INSLINE(uint8_t l);                     // 指定行に1行挿入(下スクロール)
    void scrollFrame(uint16_t vsp);              // 指定利用のスクロール
    void drawBitmap_x2(int16_t x, int16_t y,
           const uint8_t *bitmap, int16_t w, int16_t h,
           uint16_t color, uint16_t ex, uint8_t f=0);

  void colorDrawBitmap(int16_t x, int16_t y,
           const uint8_t *bitmap, int16_t w, int16_t h,
           uint16_t ex, uint8_t f=0);

  
 public:
    virtual void init(const uint8_t* fnt, uint16_t ln=256, uint8_t kbd_type=false, uint8_t* extmem=NULL, uint8_t vmode=TV_FONT_EX, int8_t rt=3,int8_t Hajst=0, int8_t Vajst=0,uint8_t ifmode=0);

    void setColor(uint16_t fc, uint16_t bc);     // 文字色指定
    void setAttr(uint16_t attr);                 // 文字属性

    uint8_t drawCurs(uint8_t x, uint8_t y);      // カーソル表示
	  virtual void setScreen(uint8_t mode,uint8_t rt=3);     // スクリーンモード設定
	
	 // グラフィック描画
	void  ginit() {};
	inline uint8_t *getfontadr(){ return 0;};// フォントアドレスの参照
    uint8_t* getGRAM(){return 0;};
	  uint16_t getGRAMsize(){return 0;};
    void     pset(int16_t x, int16_t y, uint16_t c);
    void     line(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t c);
    void     circle(int16_t x, int16_t y, int16_t r, uint16_t c, int8_t f);
    void     rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t c, int8_t f);
    void     bitmap(int16_t x, int16_t y, uint8_t* adr, uint16_t index, uint16_t w, uint16_t h, uint16_t d, uint8_t rgb=0);
    void     gscroll(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t mode){};
    void     cscroll(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t d) {};
    int16_t  gpeek(int16_t x, int16_t y){return 0;};
    int16_t  ginp(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t c){return 0;};
    void     set_gcursor(uint16_t, uint16_t);
    void     gputch(uint8_t c);
    uint8_t  bmpDraw(char *filename, uint8_t x, uint16_t y, uint16_t bx=0, uint16_t by=0, uint16_t bw=0, uint16_t bh=0);
};

#endif
