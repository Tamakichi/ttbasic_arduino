//
// file:tTVscreen.h
// ターミナルスクリーン制御ライブラリ for Arduino STM32
//  V1.0 作成日 2017/03/22 by たま吉さん
//  修正日 2017/03/26, 色制御関連関数の追加
//  修正日 2017/03/30, moveLineEnd()の追加,[HOME],[END]の編集キーの仕様変更
//  修正日 2017/04/09, PS/2キーボードとの併用可能
//  修正日 2017/04/11, TVout版に移行
//  修正日 2017/04/15, 行挿入の追加(次行上書き防止対策）
//  修正日 2017/04/17, bitmaph表示処理の追加
//  修正日 2017/04/18, シリアルポート設定機能の追加
//  修正日 2017/04/19, 行確定時の不具合対応
//  修正日 2017/04/18, シリアルポート設定機能の追加,cscroll,gscroll表示処理の追加
//  修正日 2017/04/24, cscroll()関数 キャラクタ下スクロール不具合修正
//  修正日 2017/04/25, gscrollの機能修正, gpeek,ginpの追加
//  修正日 2017/04/29, キーボード、NTSCの補正対応
//  修正日 2017/05/10, システムクロック48MHz時のtv_get_gwidth()不具合対応
//  修正日 2017/05/28, 上下スクロール編集対応
//  修正日 2017/06/09, シリアルからは全てもコードを通すように修正
//  修正日 2017/06/18, Insert_char()のメモリ領域外アクセスの不具合対応
//  修正日 2017/06/22, シリアル上でBSキー、CTRL-Lが利用可能対応
//  修正日 2017/08/19, tvutl,tGraphicDevの関数群の移行
//  修正日 2017/08/24, tone(),notone()の削除,フォント指定方法の仕様変更（フォント汎用利用のため)
//  修正日 2017/11/08, 関数の一部のインライン化,init()の修正
//

#include <string.h>
#include <TTVout.h>	
#include "tTVscreen.h"

#define NTSC_VIDEO_SPI 2

void    endPS2();
void    setupPS2(uint8_t kb_type);
uint8_t ps2read();

int16_t getPrevLineNo(int16_t lineno);
int16_t getNextLineNo(int16_t lineno);
char*   getLineStr(int16_t lineno);
int16_t getTopLineNum();
int16_t getBottomLineNum();

//******************************************************************************
// tvutilライブラリの統合
//******************************************************************************

//
// NTSC表示の初期設定
// 
void tTVscreen::tv_init(int16_t ajst, int16_t Hajst, int16_t Vajst, uint8_t* extmem, uint8_t vmode) { 
  this->TV.TNTSC->adjust(ajst,Hajst,Vajst);
  this->TV.begin(vmode, NTSC_VIDEO_SPI, extmem); // SPI2を利用
	
#if NTSC_VIDEO_SPI == 2
  // SPI2 SCK2(PB13ピン)が起動直後にHIGHになっている修正
   pinMode(PB13, INPUT);  
#endif
  this->TV.select_font(this->font);            // フォントの設定
  this->g_width  = this->TV.TNTSC->width();     // 横ドット数
  this->g_height = this->TV.TNTSC->height();    // 縦ドット数
	
  this->c_width  = this->g_width  / f_width;    // 横文字数
  this->c_height = this->g_height / f_height;   // 縦文字数
  this->vram = this->TV.VRAM();                 // VRAM先頭
  
  this->b_adr =  (uint32_t*)(BB_SRAM_BASE + ((uint32_t)this->vram - BB_SRAM_REF) * 32);
}


//
// カーソル表示
//
uint8_t tTVscreen::drawCurs(uint8_t x, uint8_t y) {
  for (uint16_t i = 0; i < f_height; i++)
     for (uint16_t j = 0; j < f_width; j++)
       this->TV.set_pixel(x*f_width+j, y*f_height+i,2);
  return 0;
}

// ビットマップ表示
void tTVscreen::tv_bitmap(int16_t x, int16_t y, uint8_t* adr, uint16_t index, uint16_t w, uint16_t h, uint16_t n) {
    uint8_t  nb = (w+7)/8; // 横バイト数の取得
    uint8_t  d;
    int16_t xx, yy, b;
    adr+= nb*h*index;
  
  if (n == 1) {
    // 1倍の場合
    this->TV.bitmap(x, y, adr  , 0, w, h);
  } else {
    // N倍の場合
    yy = y;
    for (uint16_t i = 0; i < h; i++) {
      xx = x;
      for (uint16_t j = 0; j < nb; j++) {
        d = adr[nb*i+j];
        b = (int16_t)w-8*j-8>=0 ? 8:w % 8;
        for(uint8_t k = 0; k < b; k++) {
          this->tv_dot(xx, yy, n, d>>(7-k) & 1);
          xx+= n;
        }
      }
      yy+=n;
    }
  }
}

// グラフィックスクロール
void tTVscreen::gscroll(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t mode) {
  uint8_t* bmp = this->getGRAM()+(this->g_width>>3)*y; // フレームバッファ参照位置 
  uint16_t bl = (this->g_width+7)>>3;             // 横バイト数
  uint16_t sl = (w+7)>>3;                         // 横スクロールバイト数
  uint16_t xl = (x+7)>>3;                         // 横スクロール開始オフセット(バイト)
  
  uint16_t addr;                      // データアドレス
  uint8_t prv_bit;                    // 直前のドット
  uint8_t d;                          // 取り出しデータ
   
  switch(mode) {
      case 0: // 上
        addr=xl;   
        for (int16_t i=0; i<h-1;i++) {
          memcpy(&bmp[addr],&bmp[addr+bl], sl);
          addr+=bl;
        }
        memset(&bmp[addr], 0, sl);
        break;                   
      case 1: // 下
        addr=bl*(h-1)+xl;
        for (int16_t i=0; i<h-1;i++) {
          memcpy(&bmp[addr],&bmp[addr-bl], sl);
          addr-=bl;
        }
        memset(&bmp[addr], 0, sl);
       break;                          
     case 2: // 右
      addr=xl;
      for (int16_t i=0; i < h;i++) {
        prv_bit = 0;
        for (int16_t j=0; j < sl; j++) {
          d = bmp[addr+j];
          bmp[addr+j]>>=1;
          if (j>0)
            bmp[addr+j] |= prv_bit;
          prv_bit=d<<7;        
        }
        addr+=bl;
      } 
      break;                              
     case 3: // 左
        addr=xl;
        for (int16_t i=0; i < h;i++) {
          prv_bit = 0;
          for (int16_t j=0; j < sl; j++) {
            d = bmp[addr+sl-1-j];
            bmp[addr+sl-1-j]<<=1;
            if (j>0)
              bmp[addr+sl-1-j] |= prv_bit;
            prv_bit=d>>7;
          }
          addr+=bl;
        }
       break;              
   }
}

// 指定領域のピクセル有無チェック
int16_t tTVscreen::ginp(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t c) {
  for (int16_t i = y ; i < y+h; i++) {
    for (int16_t j= x; j < x+w; j++) {
      if (this->b_adr[g_width*i+ (j&0xf8) +7 -(j&7)] == c) {
          return 1;
      }
    }
  }
  return 0;	
}

//****************************************************************************
// 差分実装
//***************************************************************************

// スクロールアップ
void tTVscreen::SCROLL_UP() {
  this->TV.shift(*(font+1), UP);
  this->clerLine(this->c_height-1);
}

// スクロールダウン
void tTVscreen::SCROLL_DOWN() {
  uint8_t h = *(font+1);
  this->TV.shift(h, DOWN);
  h = this->g_height % h;
  if (h) {
    this->TV.draw_rect(0, this->g_height-h, this->g_width, h, 0, 0); 
  }
  this->clerLine(0);	
}

// 指定行に1行挿入(下スクロール)
void tTVscreen::INSLINE(uint8_t l) {
  if (l > c_height-1) {
    return;
  } else if (l == this->c_height-1) {
    this->clerLine(l);
  } else {
    uint8_t* src = this->vram + f_height*this->g_width/8*l;        // 移動元
    uint8_t* dst = this->vram + f_height*this->g_width/8*(l+1) ;   // 移動先
    uint16_t sz  = f_height*this->g_width/8*(this->c_height-1-l);  // 移動量
    memmove(dst, src, sz);
    this->clerLine(l);
  }
}

//****************************************************************************


// スクリーンの初期設定
// 引数
//  w  : スクリーン横文字数
//  h  : スクリーン縦文字数
//  l  : 1行の最大長
// 戻り値
//  なし
void tTVscreen::init(const uint8_t* fnt, uint16_t ln, uint8_t kbd_type, uint8_t* extmem,
                     uint8_t vmode, int8_t NTSCajst, int8_t Hajst, int8_t Vajst, uint8_t ifmode) {
  static const uint8_t tvmodde[]= {SC_224x216, SC_224x108, SC_112x108 };
	this->font = (uint8_t*)fnt; 

  // ビデオ出力設定
  this->serialMode = 0;
  this->tv_init(NTSCajst, Hajst, Vajst, extmem, tvmodde[vmode-1]);
  if (extmem == NULL) {
    tscreenBase::init(this->c_width,this->c_height, ln);
  } else {
    tscreenBase::init(this->c_width,this->c_height, ln, extmem + this->getGRAMsize());
  }	

#if PS2DEV == 1
  setupPS2(kbd_type);
#endif

 // シリアルからの制御文字許可
  this->allowCtrl = true;
}

// スクリーンの利用の終了
void tTVscreen::end() {
  // キーボード利用の終了
  endPS2();

  // ビデオ出力終了
  this->TV.end();	
  tscreenBase::end();
}

// 行のリフレッシュ表示
void tTVscreen::refresh_line(uint16_t l) {
  CLEAR_LINE(l);
  for (uint16_t j = 0; j < width; j++) {
      WRITE(j,l,VPEEK(j,l));
  }
}

