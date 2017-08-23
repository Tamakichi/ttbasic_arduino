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
//

#ifndef __tTVscreen_h__
#define __tTVscreen_h__

#include <Arduino.h>
#include <TTVout.h>
#include "tscreenBase.h"
//#include "tSerialDev.h"
#include "tGraphicDev.h"

// PS/2キーボードの利用 0:利用しない 1:利用する
#define PS2DEV     1  

//uint8_t* tv_getFontAdr() ;

// スクリーン定義
#define SC_FIRST_LINE  0  // スクロール先頭行
//#define SC_LAST_LINE  24  // スクロール最終行

#define SC_TEXTNUM   256  // 1行確定文字列長さ

class tTVscreen : public tscreenBase, public tGraphicDev {
  private:
//<-- 2017/08/19 追加	
    TTVout TV;

    uint16_t c_width;    // 横文字数
    uint16_t c_height;   // 縦文字数
    uint8_t* vram;       // VRAM先頭

    uint32_t *b_adr;     // フレームバッファビットバンドアドレス

	//void tv_fontInit();
    void tv_init(int16_t ajst, uint8_t* extmem=NULL, uint8_t vmode=SC_DEFAULT);
	void tv_end();


	uint16_t tv_getGVRAMSize();
	uint8_t  drawCurs(uint8_t x, uint8_t y);
	void     write(uint8_t x, uint8_t y, uint8_t c);
	void     clerLine(uint16_t l);


 	void tv_dot(int16_t x, int16_t y, int16_t n, uint8_t c);
    void tv_bitmap(int16_t x, int16_t y, uint8_t* adr, uint16_t index, uint16_t w, uint16_t h, uint16_t n);

  public:	
	 // グラフィック描画
    void  ginit() ;
    inline uint8_t *getfontadr();// フォントアドレスの参照
    uint8_t* getGRAM();
    int16_t  getGRAMsize();
    void     pset(int16_t x, int16_t y, uint8_t c);
    void     line(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t c);
    void     circle(int16_t x, int16_t y, int16_t r, uint8_t c, int8_t f);
    void     rect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t c, int8_t f);
    void     bitmap(int16_t x, int16_t y, uint8_t* adr, uint16_t index, uint16_t w, uint16_t h, uint16_t d);
    //virtual void cscroll(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t d);
    void     gscroll(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t mode);
    int16_t  gpeek(int16_t x, int16_t y);
    int16_t  ginp(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t c);
    void     set_gcursor(uint16_t, uint16_t);
    void     gputch(uint8_t c);

	
// -->
  protected:
    void INIT_DEV(){};                           // デバイスの初期化
    void MOVE(uint8_t y, uint8_t x);             // キャラクタカーソル移動 **
    void WRITE(uint8_t x, uint8_t y, uint8_t c); // 文字の表示
    void CLEAR();                                // 画面全消去
    void CLEAR_LINE(uint8_t l);                  // 行の消去
    void SCROLL_UP();                            // スクロールアップ
    void SCROLL_DOWN();                          // スクロールダウン
    void INSLINE(uint8_t l);                     // 指定行に1行挿入(下スクロール)

  public:
    uint16_t prev_pos_x;        // カーソル横位置
    uint16_t prev_pos_y;        // カーソル縦位置
 
    void init( uint16_t ln=256, uint8_t kbd_type=false,
    	       int16_t NTSCajst=0, uint8_t* extmem=NULL, 
               uint8_t vmode=SC_DEFAULT);                // スクリーンの初期設定
	void end();                                          // スクリーンの利用の終了
	void set_allowCtrl(uint8_t flg) { allowCtrl = flg;}; // シリアルからの入力制御許可設定
    void Serial_Ctrl(int16_t ch);
    void reset_kbd(uint8_t kbd_type=false);
    void putch(uint8_t c);                            // 文字の出力
    uint8_t get_ch();                                 // 文字の取得
    inline uint8_t getDevice() {return dev;};         // 文字入力元デバイス種別の取得
    uint8_t isKeyIn();                                // キー入力チェック 
    uint8_t edit();                                   // スクリーン編集
    void newLine();                                   // 改行出力
    void refresh_line(uint16_t l);                    // 行の再表示
	
    void show_curs(uint8_t flg);                      // カーソルの表示/非表示制御
    void draw_cls_curs();                             // カーソルの消去

    void beep() {/*addch(0x07);*/};

    inline uint8_t IS_PRINT(uint8_t ch) {
      //return (((ch) >= 32 && (ch) < 0x7F) || ((ch) >= 0xA0)); 
      return (ch >= 32); 
    };

    void tone(int16_t freq, int16_t tm);
    void notone();
    void cscroll(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t d);

    // システム設定
    void  adjustNTSC(int16_t ajst);
    
};

#endif
