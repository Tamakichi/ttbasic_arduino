//
// file: tscreen.h
// ターミナルスクリーン制御ライブラリ ヘッダファイル for Arduino STM32
// V1.0 作成日 2017/03/22 by たま吉さん
//  修正日 2017/03/26, 色制御関連関数の追加
//  修正日 2017/03/30, moveLineEnd()の追加,[HOME],[END]の編集キーの仕様変更
//  修正日 2017/04/02, getScreenByteSize()の追加
//  修正日 2017/04/03, getScreenByteSize()の不具合対応
//  修正日 2017/04/06, beep()の追加
//  修正日 2017/04/11, TVout版に移行
//  修正日 2017/04/15, 行挿入の追加
//  修正日 2017/04/17, bitmap表示処理の追加
//  修正日 2017/04/18, シリアルポート設定機能の追加,cscroll,gscroll表示処理の追加
//  修正日 2017/04/25, gscrollの機能修正, gpeek,ginpの追加
//  修正日 2017/04/29, キーボード、NTSCの補正対応
//  修正日 2017/05/19, getfontadr()の追加
//  修正日 2017/05/28, 上下スクロール編集対応
//  修正日 2017/06/09, シリアルからは全てもコードを通すように修正
//  修正日 2017/06/22, シリアルからは全てもコードを通す切り替え機能の追加
//  修正日 2017/08/24, tone(),notone()の削除
//  修正日 2017/11/08, 関数の一部のインライン化,init()の修正
//

#ifndef __tTVscreen_h__
#define __tTVscreen_h__

#include <Arduino.h>
#include <TTVout.h>

#include "tGraphicScreen.h"

/*
#include "tscreenBase.h"
#include "tGraphicDev.h"

// PS/2キーボードの利用 0:利用しない 1:利用する
#define PS2DEV     1  
*/

// スクリーン定義
#define SC_FIRST_LINE  0  // スクロール先頭行
//#define SC_LAST_LINE  24  // スクロール最終行

#define SC_TEXTNUM   256  // 1行確定文字列長さ

extern uint16_t f_width;    // フォント幅(ドット)
extern uint16_t f_height;   // フォント高さ(ドット)

class tTVscreen : public tGraphicScreen {
	private:
//<-- 2017/08/19 追加	
    TTVout TV;

    uint16_t c_width;    // 横文字数
    uint16_t c_height;   // 縦文字数
    uint8_t* vram;       // VRAM先頭
    uint32_t *b_adr;     // フレームバッファビットバンドアドレス

    void tv_init(int16_t ajst, uint8_t* extmem=NULL, uint8_t vmode=SC_DEFAULT);
	  void tv_end();

  	uint8_t  drawCurs(uint8_t x, uint8_t y);
    
    // 文字の表示
    void  write(uint8_t x, uint8_t y, uint8_t c) { TV.print_char(x * f_width, y * f_height ,c); }; 
    // 指定行の1行クリア
    void clerLine(uint16_t l) {memset(this->vram + f_height*this->g_width/8*l, 0, f_height*this->g_width/8);};
  	void tv_dot(int16_t x, int16_t y, int16_t n, uint8_t c) {
       for (int16_t i = y ; i < y+n; i++)
           for (int16_t j= x; j < x+n; j++)
             b_adr[this->g_width*i+(j&0xf8)+7-(j&7)] = c;
    };
    void tv_bitmap(int16_t x, int16_t y, uint8_t* adr, uint16_t index, uint16_t w, uint16_t h, uint16_t n);

  public:	
	  // グラフィック描画
  void  ginit() {};
    
    
    uint8_t *getfontadr() {return font+3;};  // フォントアドレスの参照
    uint8_t* getGRAM() {return this->vram;}; // グラフィク表示用メモリアドレス参照
    uint16_t getGRAMsize() { return (this->g_width>>3)*this->g_height;}; // GVRAMサイズ取得
    void     pset(int16_t x, int16_t y, uint16_t c) {this->TV.set_pixel(x,y,c);	} ;// ドット描画
    void     line(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t c) {TV.draw_line(x1,y1,x2,y2,c);};
    void     circle(int16_t x, int16_t y, int16_t r, uint16_t c, int8_t f) {TV.draw_circle(x, y, r, c, f?f:-1);};
    void     rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t c, int8_t f) {TV.draw_rect(x, y, w, h, c, f?f:-1);};
    void     bitmap(int16_t x, int16_t y, uint8_t* adr, uint16_t index, uint16_t w, uint16_t h, uint16_t d, uint8_t rgb=0) 
             {tv_bitmap(x, y, adr, index, w, h, d);};
    void     gscroll(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t mode);
    int16_t  gpeek(int16_t x, int16_t y) {return b_adr[g_width*y+ (x&0xf8) +7 -(x&7)];};
    int16_t  ginp(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t c);
    void     set_gcursor(uint16_t x, uint16_t y) {TV.set_cursor(x,y);	} ; // グラフィック文字表示カーソル設定
    void     gputch(uint8_t c) {TV.write(c);}; // 文字のグラフィック表示

  protected:
    virtual void INIT_DEV(){};                           // デバイスの初期化
    virtual void WRITE(uint8_t x, uint8_t y, uint8_t c) {write(x, y, c);}; // 文字の表示
    virtual void CLEAR() {TV.cls(); } ;                  // 画面全消去
    virtual void CLEAR_LINE(uint8_t l) {clerLine(l);};   // 行の消去
    virtual void SCROLL_UP();                            // スクロールアップ
    virtual void SCROLL_DOWN();                          // スクロールダウン
    virtual void INSLINE(uint8_t l);                     // 指定行に1行挿入(下スクロール)

  public:
    uint16_t prev_pos_x;        // カーソル横位置
    uint16_t prev_pos_y;        // カーソル縦位置
 
    // スクリーンの初期設定
    virtual void init( const uint8_t* fnt,
    	       uint16_t ln=256, uint8_t kbd_type=false,
    	       uint8_t* extmem=NULL, uint8_t vmode=1, uint8_t NTSCajst=0, uint8_t ifmode=0);                

  void end();   // スクリーンの利用の終了
  virtual void refresh_line(uint16_t l); // 行の再表示
  
  // 垂直同期信号補正
  void  adjustNTSC(uint8_t ajst) {this->TV.TNTSC->adjust(ajst);};                      
};

#endif
