//
// file: tTFTScreen.cpp
// ILI9341利用ターミナルスクリーン制御クラス
//
// 2017/08/12 修正 SPI2を利用に修正
// 2017/08/25 修正 グラフィック描画対応
// 2017/08/28 スクリーン用メモリに確保済領域指定対応
// 2017/11/05 デバッグ用出力消し忘れミスの対応
//

#include <string.h>
#include "tTFTScreen.h"

#include <SD.h>
#define SD_CS       (PA4)   // SDカードモジュールCS
#define BUFFPIXEL 20
#define SD_BEGIN() SD.begin(F_CPU/4,SD_CS)

//#define TFT_FONT_MODE 0 // フォント利用モード 0:TVFONT 1以上 Adafruit_GFX_ASフォント
//#define TV_FONT_EX 1    // フォント倍率

#define SD_ERR_INIT       1    // SDカード初期化失敗
#define SD_ERR_OPEN_FILE  2    // ファイルオープン失敗
#define SD_ERR_READ_FILE  3    // ファイル読込失敗
#define SD_ERR_NOT_FILE   4    // ファイルでない(ディレクトリ)
#define SD_ERR_WRITE_FILE 5    //  ファイル書込み失敗

//#define TFT_CS      PA0
#define TFT_CS      PB11
//#define TFT_RST     PA1
#define TFT_RST     -1
//#define TFT_DC      PA2
#define TFT_DC      PB12

#define BOT_FIXED_AREA 0 // Number of lines in bottom fixed area (lines counted from bottom of screen)
#define TOP_FIXED_AREA 0 // Number of lines in top fixed area (lines counted from top of screen)
#define ILI9341_VSCRDEF  0x33
#define ILI9341_VSCRSADD 0x37

void setupPS2(uint8_t kb_type);
uint8_t ps2read();
void    endPS2();

static const uint16_t tbl_color[]  =
     {  ILI9341_BLACK, ILI9341_RED, ILI9341_GREEN, ILI9341_MAROON, ILI9341_BLUE, ILI9341_MAGENTA, ILI9341_CYAN, ILI9341_WHITE, ILI9341_YELLOW};


// 初期化
void tTFTScreen::init(const uint8_t* fnt, uint16_t ln, uint8_t kbd_type, uint8_t* extmem, uint8_t vmode, uint8_t rt, uint8_t ifmode) {
  this->font = (uint8_t*)fnt;
  this->tft = new Adafruit_ILI9341_STM_TT(TFT_CS, TFT_DC, TFT_RST,2); // Use hardware SPI
  this->tft->begin();
  setScreen(vmode, rt); // スクリーンモード,画面回転指定
  if (extmem == NULL) {
    tscreenBase::init(this->width,this->height, ln);
  } else {
    tscreenBase::init(this->width,this->height, ln, extmem);
  }	
 
#if PS2DEV == 1
  setupPS2(kbd_type);
#endif

 // シリアルからの制御文字許可
  this->allowCtrl = true;
}


// 依存デバイスの初期化
void tTFTScreen::INIT_DEV() {

}


// スクリーンモード設定
void tTFTScreen::setScreen(uint8_t mode, uint8_t rt) {
  this->tft->setRotation(rt);
  this->g_width  = this->tft->width();             // 横ドット数
  this->g_height = this->tft->height();            // 縦ドット数

  this->fontEx = mode;
  this->f_width  = *(font+0)*this->fontEx;         // 横フォントドット数
  this->f_height = *(font+1)*this->fontEx;         // 縦フォントドット数
  this->width  = this->g_width  / this->f_width;   // 横文字数
  this->height = this->g_height / this->f_height;  // 縦文字数
  this->fgcolor = ILI9341_WHITE;
  this->bgcolor = ILI9341_BLACK;
  this->tft->setCursor(0, 0);
  pos_gx =0;  pos_gy =0;
}

// カーソル表示
uint8_t tTFTScreen::drawCurs(uint8_t x, uint8_t y) {
  uint8_t c;
  c = VPEEK(x, y);
#if TFT_FONT_MODE == 0
  tft->fillRect(x*f_width,y*f_height,f_width,f_height,fgcolor);
 if (fontEx == 1)
    tft->drawBitmap(x*f_width,y*f_height,font+3+((uint16_t)c)*8,f_width,f_height, bgcolor);
 else
	drawBitmap_x2(x*f_width,y*f_height,font+3+((uint16_t)c)*8,f_width/fontEx, f_height/fontEx, bgcolor, fontEx);
#endif
#if TFT_FONT_MODE > 0
  tft->drawChar(x*f_width,y*f_height, c, ILI9341_BLACK, ILI9341_WHITE, TFT_FONT_MODE);
#endif
  return 0;
}

// 文字の表示
void tTFTScreen::WRITE(uint8_t x, uint8_t y, uint8_t c) {
#if TFT_FONT_MODE == 0
  tft->fillRect(x*f_width,y*f_height,f_width,f_height, bgcolor);
  if (fontEx == 1)
    tft->drawBitmap(x*f_width,y*f_height,font+3+((uint16_t)c)*8,f_width,f_height, fgcolor);
  else
    drawBitmap_x2(x*f_width,y*f_height,font+3+((uint16_t)c)*8,f_width/fontEx,f_height/fontEx, fgcolor, fontEx);
#endif
#if TFT_FONT_MODE > 0
  tft->drawChar(x*f_width,y*f_height, c, fgcolor, bgcolor, TFT_FONT_MODE);
#endif

}

// グラフィックカーソル設定
void tTFTScreen::set_gcursor(uint16_t x, uint16_t y) {
  this->pos_gx = x;
  this->pos_gy = y;
}

// グラフィック文字表示
void  tTFTScreen::gputch(uint8_t c) {
  drawBitmap_x2( this->pos_gx, this->pos_gy, this->font+3+((uint16_t)c)*8,
    	           this->f_width/ this->fontEx,  this->f_height/this->fontEx,  this->fgcolor, this->fontEx);

  this->pos_gx += this->f_width;
  if (this->pos_gx + this->f_width >= this->g_width) {
     this->pos_gx = 0;
     this->pos_gy += this->f_height;
  }
};


// 画面全消去
void tTFTScreen::CLEAR() {
  tft->fillScreen( bgcolor);
  pos_gx =0;  pos_gy =0;
}

// 行の消去
void tTFTScreen::CLEAR_LINE(uint8_t l) {
  tft->fillRect(0,l*f_height,g_width,f_height,bgcolor);
}

void tTFTScreen::scrollFrame(uint16_t vsp) {
  tft->writecommand(ILI9341_VSCRSADD);
  tft->writedata(vsp >> 8);
  tft->writedata(vsp);
}

// スクロールアップ
void tTFTScreen::SCROLL_UP() {
 refresh();
}

// スクロールダウン
void tTFTScreen::SCROLL_DOWN() {
  INSLINE(0);
}

// 指定行に1行挿入(下スクロール)
void tTFTScreen::INSLINE(uint8_t l) {
 refresh();
}

// 文字色指定
void tTFTScreen::setColor(uint16_t fc, uint16_t bc) {
  if (fc>8)
	  fgcolor = fc;
	else
	  fgcolor = tbl_color[fc];
	if (bc>8)
    bgcolor = bc;
	else
   bgcolor = tbl_color[bc];
  
}

// 文字属性
void tTFTScreen::setAttr(uint16_t attr) {

}

// ビットマップの拡大描画
void tTFTScreen::drawBitmap_x2(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h,uint16_t color, uint16_t ex, uint8_t f) {
  int16_t i, j,b=(w+7)/8;
  for( j = 0; j < h; j++) {
    for(i = 0; i < w; i++ ) { 
      if(*(bitmap + j*b + i / 8) & (128 >> (i & 7))) {
        // ドットあり
        if (ex == 1)
           this->tft->drawPixel(x+i, y+j, color); //1倍
        else
          tft->fillRect(x + i * ex, y + j * ex, ex, ex, color); // ex倍
      } else {
        // ドットなし
        if (f) {
          // 黒を透明扱いしない
          if (ex == 1)      
            this->tft->drawPixel(x+i, y+j, bgcolor);
          else
            tft->fillRect(x + i * ex, y + j * ex, ex, ex, bgcolor);
       }
     }
   }
 }
}

// カラービットマップの拡大描画
// 8ビット色: RRRGGGBB 
void tTFTScreen::colorDrawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t ex, uint8_t f) {
  int16_t i, j;
  uint16_t color8,color16,r,g,b;
  for( j = 0; j < h; j++) {
    for(i = 0; i < w; i++ ) { 
      color8   = *(bitmap+w*j+i);
      r =  color8 >>5;
      g = (color8>>2)&7;
      b =  color8 & 3;
      color16 = this->tft->color565(r<<5,g<<5,b<<6); // RRRRRGGGGGGBBBBB
      if ( color16 || (!color16 && f) ) {
        // ドットあり
        if (ex == 1) {
          // 1倍
           this->tft->drawPixel(x+i, y+j, color16);
        } else {
            this->tft->fillRect(x + i * ex, y + j * ex, ex, ex, color16);
        }
     }
   }
 }
}


// ドット描画
void tTFTScreen::pset(int16_t x, int16_t y, uint16_t c) {
  if (c<=8)
    c = tbl_color[c];
    
  this->tft->drawPixel(x, y, c);
}

// 線の描画
void tTFTScreen::line(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t c){
  if (c<=8)
    c = tbl_color[c];
  this->tft->drawLine(x1, y1, x2, y2, c);
}

// 円の描画
void tTFTScreen::circle(int16_t x, int16_t y, int16_t r, uint16_t c, int8_t f) {
 if (c<=8)
    c = tbl_color[c];
 if(f)
   this->tft->fillCircle(x, y, r, c);
 else
   this->tft->drawCircle(x, y, r, c);
}

// 四角の描画
void tTFTScreen::rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t c, int8_t f) {
 if(f)
   this->tft->fillRect(x, y, w, h, c);
 else
   this->tft->drawRect(x, y, w, h, c);
}

// ビットマップの描画
void  tTFTScreen::bitmap(int16_t x, int16_t y, uint8_t* adr, uint16_t index, uint16_t w, uint16_t h, uint16_t d, uint8_t rgb) {
  uint8_t*bmp;
  if (rgb == 0) {
    bmp = adr + ((w + 7) / 8) * h * index;
    this->drawBitmap_x2(x, y, (const uint8_t*)bmp, w, h, fgcolor, d, 1);
  } else {
    bmp = adr + w * h * index;
    this->colorDrawBitmap(x, y, (const uint8_t*)bmp, w, h, d, 1);
  }  
}


// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.
// 本関数はAdafruit_ILI9341_STMライブラリのサンプルspitftbitmapを利用しています
//
static uint16_t read16(File &f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

static uint32_t read32(File &f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}

//
// ビットマックロード
// 本関数はAdafruit_ILI9341_STMライブラリのサンプルspitftbitmapを利用しています
//

uint8_t tTFTScreen::bmpDraw(char *filename, uint8_t x, uint16_t y, uint16_t bx, uint16_t by, uint16_t bw, uint16_t bh) {
  File     bmpFile;
  int      bmpWidth, bmpHeight;   // W+H in pixels
  uint8_t  bmpDepth;              // Bit depth (currently must be 24)
  uint32_t bmpImageoffset;        // Start of image data in file
  uint32_t rowSize;               // Not always = bmpWidth; may have padding
  uint8_t  sdbuffer[3*BUFFPIXEL]; // pixel buffer (R+G+B per pixel)
  uint8_t  buffidx = sizeof(sdbuffer); // Current position in sdbuffer
  boolean  goodBmp = false;       // Set to true on valid header parse
  boolean  flip    = true;        // BMP is stored bottom-to-top
  int      w, h, row, col;
  uint8_t  r, g, b;
  uint32_t pos = 0;
  uint8_t rc = 0;
  
  if((x >= this->tft->width()) || (y >= this->tft->height())) return 10;
  if (SD_BEGIN() == false) 
    return SD_ERR_INIT;
  if ((bmpFile = SD.open(filename)) == NULL) {
    rc = SD_ERR_OPEN_FILE;
  	goto ERROR;
  }
  
  // Parse BMP header
  if(read16(bmpFile) == 0x4D42) { // BMP signature
  	read32(bmpFile);
  	(void)read32(bmpFile);            // Read & ignore creator bytes
    bmpImageoffset = read32(bmpFile); // Start of image data

  	// Read DIB header
  	read32(bmpFile);
    bmpWidth  = read32(bmpFile);  // ビットマップ画像横ドット数
    bmpHeight = read32(bmpFile);  // ビットマック画像縦ドット数
    if(read16(bmpFile) == 1) { // # planes -- must be '1'
      bmpDepth = read16(bmpFile); // bits per pixel
      if((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed

        goodBmp = true; // Supported BMP format -- proceed!
      	
        rowSize = (bmpWidth * 3 + 3) & ~3;
        if(bmpHeight < 0) {
          bmpHeight = -bmpHeight;
          flip      = false;
        }

        // Crop area to be loaded
        w = bmpWidth;
        h = bmpHeight;
        if (bx+bw >= w) bw=w-bx;
        if (by+bh >= h) bh=h-by;
        w=bx+bw;
        h=by+bh;
        
        if((x+w-1) >= this->tft->width())  w = this->tft->width()  - x;
        if((y+h-1) >= this->tft->height()) h = this->tft->height() - y;
       
        // Set TFT address window to clipped image bounds
        this->tft->setAddrWindow(x, y, x+bw-1, y+bh-1);
        for (row = by; row < h; row++) {
          // ビットマップ画像のライン毎のデータ処理
          
          // 格納画像の向きの補正
          if(flip)
            pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize + bx*3;
          else 
            pos = bmpImageoffset + row * rowSize + bx*3;
          if(bmpFile.position() != pos) {
            bmpFile.seek(pos);
            buffidx = sizeof(sdbuffer); 
          }

          for (col=bx; col<w; col++) {
            // ドット毎の処理
            // Time to read more pixel data?
            if (buffidx >= sizeof(sdbuffer)) { // Indeed
              bmpFile.read(sdbuffer, sizeof(sdbuffer));
              buffidx = 0; // Set index to beginning
            }

            // Convert pixel from BMP to TFT format, push to display
              b = sdbuffer[buffidx++];
              g = sdbuffer[buffidx++];
              r = sdbuffer[buffidx++];
              this->tft->pushColor(this->tft->color565(r,g,b));
          } // end pixel
        } // end scanline
      } // end goodBmp
    }
  }

  bmpFile.close();
ERROR:
  SD.end();
	
  if(!goodBmp) {
    rc =  10;
  }
 return rc;
}
