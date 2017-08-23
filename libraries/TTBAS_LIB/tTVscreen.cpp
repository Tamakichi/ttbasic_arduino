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
//

#include <string.h>
#include <TTVout.h>	
#include "tTVscreen.h"
extern uint8_t* ttbasic_font;
extern uint8_t* tvfont;     // 利用フォント	
extern uint16_t f_width;    // フォント幅(ドット)
extern uint16_t f_height;   // フォント高さ(ドット)
#define NTSC_VIDEO_SPI 2

uint8_t* tv_getFontAdr() ;
void tv_fontInit();
void    endPS2();

	
void    tv_tone(int16_t freq, int16_t tm);
void    tv_notone();


void setupPS2(uint8_t kb_type);
uint8_t ps2read();

int16_t getPrevLineNo(int16_t lineno);
int16_t getNextLineNo(int16_t lineno);
char* getLineStr(int16_t lineno);
int16_t getTopLineNum();
int16_t getBottomLineNum();

#define VPEEK(X,Y)      (screen[width*(Y)+(X)])
#define VPOKE(X,Y,C)    (screen[width*(Y)+(X)]=C)

//******************************************************************************
// tvutilライブラリの統合
//******************************************************************************

//
// NTSC表示の初期設定
// 
void tTVscreen::tv_init(int16_t ajst, uint8_t* extmem, uint8_t vmode) { 
  tv_fontInit();
  this->TV.TNTSC->adjust(ajst);
  this->TV.begin(vmode, NTSC_VIDEO_SPI, extmem); // SPI2を利用
	
#if NTSC_VIDEO_SPI == 2
  // SPI2 SCK2(PB13ピン)が起動直後にHIGHになっている修正
   pinMode(PB13, INPUT);  
#endif
  this->TV.select_font(tvfont);                 // フォントの設定
  this->g_width  = this->TV.TNTSC->width();     // 横ドット数
  this->g_height = this->TV.TNTSC->height();    // 縦ドット数
	
  this->c_width  = this->g_width  / f_width;    // 横文字数
  this->c_height = this->g_height / f_height;   // 縦文字数
  this->vram = this->TV.VRAM();                 // VRAM先頭
  
  this->b_adr =  (uint32_t*)(BB_SRAM_BASE + ((uint32_t)this->vram - BB_SRAM_REF) * 32);
}


// GVRAMサイズ取得
uint16_t tTVscreen::tv_getGVRAMSize() {
  return (this->g_width>>3)*this->g_height;
}

//
// カーソル表示
//
uint8_t tTVscreen::drawCurs(uint8_t x, uint8_t y) {
  for (uint16_t i = 0; i < f_height; i++)
     for (uint16_t j = 0; j < f_width; j++)
       this->TV.set_pixel(x*f_width+j, y*f_height+i,2);
}

//
// 文字の表示
//
void tTVscreen::write(uint8_t x, uint8_t y, uint8_t c) {
  this->TV.print_char(x * f_width, y * f_height ,c);  
}

//
// 指定行の1行クリア
//
void tTVscreen::clerLine(uint16_t l) {
  memset(this->vram + f_height*this->g_width/8*l, 0, f_height*this->g_width/8);
}

// 指定サイズのドットの描画
void tTVscreen::tv_dot(int16_t x, int16_t y, int16_t n, uint8_t c) {
  for (int16_t i = y ; i < y+n; i++) {
    for (int16_t j= x; j < x+n; j++) {
      this->b_adr[this->g_width*i+ (j&0xf8) +7 -(j&7)] = c;
    }
  }
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

	
// 初期化
void tTVscreen::ginit() {
  //g_width   = this->tv_get_gwidth();
  //g_height  = this->tv_get_gheight();  
}

// フォントアドレスの参照
uint8_t *tTVscreen::getfontadr() {
	return tv_getFontAdr()+3; 
}  

// グラフィク表示用メモリアドレス参照
uint8_t* tTVscreen::getGRAM() {
  return this->vram;	
}

// グラフィク表示用メモリサイズ取得
int16_t tTVscreen::getGRAMsize() {
  return this->tv_getGVRAMSize();
}

	
// ドット描画
void tTVscreen::pset(int16_t x, int16_t y, uint8_t c) {
  this->TV.set_pixel(x,y,c);	
}

// 線の描画
void tTVscreen::line(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t c) {
 this->TV.draw_line(x1,y1,x2,y2,c);	
}

// 円の描画
void tTVscreen::circle(int16_t x, int16_t y, int16_t r, uint8_t c, int8_t f) {
  if (f==0) f=-1;
  this->TV.draw_circle(x, y, r, c, f);	
}

// 四角の描画
void tTVscreen::rect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t c, int8_t f) {
  if (f==0) f=-1;
  this->TV.draw_rect(x, y, w, h, c, f);
	
}

// ビットマップの描画
void tTVscreen::bitmap(int16_t x, int16_t y, uint8_t* adr, uint16_t index, uint16_t w, uint16_t h, uint16_t d) {
  this->tv_bitmap(x, y, adr, index, w, h, d);
}

// グラフィックスクロール
void tTVscreen::gscroll(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t mode) {
  uint8_t* bmp = this->vram+(this->g_width>>3)*y; // フレームバッファ参照位置 
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


// 指定位置のピクセル情報取得
int16_t tTVscreen::gpeek(int16_t x, int16_t y) {
  return b_adr[g_width*y+ (x&0xf8) +7 -(x&7)];	
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

void tTVscreen::set_gcursor(uint16_t x, uint16_t y) {
  this->TV.set_cursor(x,y);	
}

void tTVscreen::gputch(uint8_t c) {
  this->TV.write(c);
}

		
	
//****************************************************************************
// 差分実装
//***************************************************************************
        
// カーソルの移動
// ※pos_x,pos_yは本関数のみでのみ変更可能
void tTVscreen::MOVE(uint8_t y, uint8_t x) {
  uint8_t c;
  if (flgCur) {
    c = VPEEK(pos_x,pos_y);
    this->write(pos_x, pos_y, c?c:32);  
    this->drawCurs(x, y);  
  }
  pos_x = x;
  pos_y = y;
}

// 文字の表示
void tTVscreen::WRITE(uint8_t x, uint8_t y, uint8_t c) {
   this->write(x, y, c); // 画面表示
}

// 画面全消去
void tTVscreen::CLEAR() {
  this->TV.cls();   
}

// 行の消去
void tTVscreen::CLEAR_LINE(uint8_t l) {
  this->clerLine(l);  
}

// スクロールアップ
void tTVscreen::SCROLL_UP() {
  this->TV.shift(*(tvfont+1), UP);
  this->clerLine(this->c_height-1);
}

// スクロールダウン
void tTVscreen::SCROLL_DOWN() {
  uint8_t h = *(tvfont+1);
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
void tTVscreen::init(uint16_t ln, uint8_t kbd_type, int16_t NTSCajst, uint8_t* extmem, uint8_t vmode) {
  
  // ビデオ出力設定
  this->serialMode = 0;
  this->tv_init(NTSCajst, extmem, vmode);
  if (extmem == NULL) {
    tscreenBase::init(this->c_width,this->c_height, ln);
  } else {
    tscreenBase::init(this->c_width,this->c_height, ln, extmem + this->tv_getGVRAMSize());
  }	
  //this->ginit();

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

void tTVscreen::reset_kbd(uint8_t kbd_type) {
  endPS2();
  setupPS2(kbd_type);
}

// 文字の出力
void tTVscreen::putch(uint8_t c) {
  tscreenBase::putch(c);
 if(this->serialMode == 0) {
    Serial.write(c);       // シリアル出力
  } else if (this->serialMode == 1) {
    Serial1.write(c);     // シリアル出力  
  }
}

// 改行
void tTVscreen::newLine() {  
  tscreenBase::newLine();
  if (this->serialMode == 0) {
    Serial.write(0x0d);
    Serial.write(0x0a);
  } else if (this->serialMode == 1) {
    Serial1.write(0x0d);
    Serial1.write(0x0a);
  }  
}

// 行のリフレッシュ表示
void tTVscreen::refresh_line(uint16_t l) {
  CLEAR_LINE(l);
  for (uint16_t j = 0; j < width; j++) {
//    if( IS_PRINT( VPEEK(j,l) )) { 
      WRITE(j,l,VPEEK(j,l));
//    }
  }
}

	
// キー入力チェック&キーの取得
uint8_t tTVscreen::isKeyIn() {
  if(this->serialMode == 0) {
    if (Serial.available())
      return Serial.read();
  } else if (serialMode == 1) {
    if (Serial1.available())
      return Serial1.read();
  }
#if PS2DEV == 1
 return ps2read();
#endif
}

// 文字入力
uint8_t tTVscreen::get_ch() {
  char c;
  while(1) {
    if(serialMode == 0) {
      if (Serial.available()) {
        c = Serial.read();
        dev = 1;
        break;
      }
    } else if (serialMode == 1) {
      if (Serial1.available()) {
        c = Serial1.read();
        dev = 2;
        break;
      }
    }
#if PS2DEV == 1
    c = ps2read();
    if (c) {
      dev = 0;
      break;
    }
#endif
  }
  return c;  
}

// カーソルの表示/非表示
// flg: カーソル非表示 0、表示 1、強調表示 2
void tTVscreen::show_curs(uint8_t flg) {
    flgCur = flg;
    if(!flgCur)
      this->draw_cls_curs();
    
}

// カーソルの消去
void tTVscreen::draw_cls_curs() {
  uint8_t c = VPEEK(pos_x,pos_y);
  this->write(pos_x, pos_y, c?c:32);
}

// スクリーン編集
uint8_t tTVscreen::edit() {
  uint8_t ch;  // 入力文字

  do {
    //MOVE(pos_y, pos_x);
    ch = get_ch ();
    if (dev != 1 || allowCtrl) { // USB-Serial(dev=1)は入力コードのキー解釈を行わない
      switch(ch) {
        case KEY_CR:         // [Enter]キー
          return this->enter_text();
          break;
  
        case SC_KEY_CTRL_L:  // [CTRL+L] 画面クリア
          cls();
          locate(0,0);
          Serial_Ctrl(SC_KEY_CTRL_L);
          break;
   
        case KEY_HOME:      // [HOMEキー] 行先頭移動
          locate(0, pos_y);
          break;
          
        case KEY_NPAGE:     // [PageDown] 表示プログラム最終行に移動
          if (pos_x == 0 && pos_y == height-1) {
            this->edit_scrollUp();
          } else {
            this->moveBottom();
          }
          break;
          
        case KEY_PPAGE:     // [PageUP] 画面(0,0)に移動
          if (pos_x == 0 && pos_y == 0) {
            this->edit_scrollDown();
          } else {
            locate(0, 0);
          }  
          break;
  
        case SC_KEY_CTRL_R: // [CTRL_R(F5)] 画面更新
          this->refresh();  break;
  
        case KEY_END:       // [ENDキー] 行の右端移動
           this->moveLineEnd();
           break;
  
        case KEY_IC:         // [Insert]キー
          flgIns = !flgIns;
          break;        
  
        case KEY_BACKSPACE:  // [BS]キー
            this->movePosPrevChar();
            this->delete_char();
           Serial_Ctrl(KEY_BACKSPACE);
          break;        
  
        case KEY_DC:         // [Del]キー
        case SC_KEY_CTRL_X:
          this->delete_char();
          break;        
        
        case KEY_RIGHT:      // [→]キー
          this->movePosNextChar();
          break;
  
        case KEY_LEFT:       // [←]キー
          this->movePosPrevChar();
          break;
  
        case KEY_DOWN:       // [↓]キー
          this->movePosNextLineChar();
          break;
        
        case KEY_UP:         // [↑]キー
          this->movePosPrevLineChar();
          break;
  
        case SC_KEY_CTRL_N:  // 行挿入 
          this->Insert_newLine(pos_y);       
          break;
  
        case SC_KEY_CTRL_D:  // 行削除
          this->clerLine(pos_y);
          break;
        
        default:             // その他
        
        if (IS_PRINT(ch)) {
          this->Insert_char(ch);
        }        
        break;
      }
    } else {
      // PS/2キーボード以外からの入力
      if (ch == KEY_CR) {
        return this->enter_text(); 
      } else {
        this->Insert_char(ch);      
      }
    }
  } while(1);
}

// シリアルポートスクリーン制御出力
void tTVscreen::Serial_Ctrl(int16_t ch) {
  char* s=NULL;
  switch(ch) {
    case KEY_BACKSPACE:
     s = "\x08\x1b[P";
     break;
    case SC_KEY_CTRL_L:
     s = "\x1b[2J\x1b[H";
     break;
  }
  if(s) {
    if(this->serialMode == 0) {
      // Serial.print(s);     // USBシリアル出力
      while(*s) {
        Serial.write(*s);
        s++;
      }  
    } else if (this->serialMode == 1) {
      //Serial1.print(s);     // シリアル出力  
      while(*s) {
        Serial1.write(*s);
        s++;
      }  
    }  
  }
}


// キャラクタ画面スクロール
void tTVscreen::cscroll(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t d) {
  switch(d) {
    case 0: // 上
      for (uint16_t i= 0; i < h-1; i++) {
        memcpy(&VPEEK(x,y+i), &VPEEK(x,y+i+1), w);
      }
      memset(&VPEEK(x, y + h - 1), 0, w);
      break;            

    case 1: // 下
      for (uint16_t i= 0; i < h-1; i++) {
        memcpy(&VPEEK(x,y + h-1-i), &VPEEK(x,y+h-1-i-1), w);
      }
      memset(&VPEEK(x, y), 0, w);
      break;            

    case 2: // 右
      for (uint16_t i=0; i < h; i++) {
        memmove(&VPEEK(x+1, y+i) ,&VPEEK(x,y+i), w-1);
        VPOKE(x,y+i,0);
      }
      break;
      
    case 3: // 左
      for (uint16_t i=0; i < h; i++) {
        memmove(&VPEEK(x,y+i) ,&VPEEK(x+1,y+i), w-1);
        VPOKE(x+w-1,y+i,0);
      }
      break;
  }
  uint8_t c;
  for (uint8_t i = 0; i < h; i++) 
    for (uint8_t j=0; j < w; j++) {
      c = VPEEK(x+j,y+i);
      this->write(x+j,y+i, c?c:32);
    }
}

void tTVscreen::tone(int16_t freq, int16_t tm) {
  ::tv_tone(freq, tm);  
}

void tTVscreen::notone() {
  ::tv_notone();    
}

void tTVscreen::adjustNTSC(int16_t ajst) {
  this->TV.TNTSC->adjust(ajst);  	
}



