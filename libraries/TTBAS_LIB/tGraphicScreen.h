//
// 2017/08/25 
//

#ifndef __tGraphicScreen_h__
#define __tGraphicScreen_h__

//#include <Arduino.h>
#include "tscreenBase.h"
#include "tGraphicDev.h"

// PS/2キーボードの利用 0:利用しない 1:利用する
#define PS2DEV     1  

class tGraphicScreen : public tscreenBase , public tGraphicDev {
 
 protected:
  uint8_t* font;                                        // 利用フォント(tTVscreenと同じ)
  
 protected:
  virtual void MOVE(uint8_t y, uint8_t x);              // キャラクタカーソル移動(tTVscreenと同じ)
	virtual uint8_t drawCurs(uint8_t x, uint8_t y) = 0;   // カーソル表示


  public:
    virtual void init(const uint8_t* fnt, uint16_t ln, uint8_t kbd_type, uint8_t* extmem, uint8_t vmode, uint8_t rt, uint8_t ifmode) =0;
    virtual void setScreen(uint8_t mode, uint8_t rt) {};     // スクリーンモード設定
    inline uint8_t IS_PRINT(uint8_t ch)          // 表示可能文字判定(tTVscreenと同じ)
	   { return (ch); };
	  void set_allowCtrl(uint8_t flg)              // シリアルからの入力制御許可設定(tTVscreenと同じ)
	   { allowCtrl = flg;}; 
    void Serial_Ctrl(int16_t ch);                // シリアルポートスクリーン制御出力(tTVscreenと同じ)
	  void reset_kbd(uint8_t kbd_type=false);      // PS/2キーボードリセット(tTVscreenと同じ)
	  void putch(uint8_t c);                       // 文字の出力(tTVscreenと同じ)
	  uint8_t get_ch();                            // 文字の取得(tTVscreenと同じ)
    inline uint8_t getDevice() {return dev;};    // 文字入力元デバイス種別の取得(tTVscreenと同じ)
    uint8_t isKeyIn();                           // キー入力チェック(tTVscreenと同じ)
    void newLine();                              // 改行出力(tTVscreenと同じ)
	  void show_curs(uint8_t flg);                 // カーソルの表示/非表示
	  void draw_cls_curs();                        // カーソルの消去
	  uint8_t edit();                              // スクリーン編集
    void cscroll(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t d); // キャラクタースクロール
};

#endif
