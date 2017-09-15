//
// file: tOLEDScreen.cpp
// SH1106/SSD1306/SSD1309利用ターミナルスクリーン制御クラス
// 2017/09/14 作成
//

#include <string.h>
#include "tOLEDScreen.h"

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

#define OLED_CS      PB11
#define OLED_RST     -1
#define OLED_DC      PB12
#define OLED_SCL     PB6
#define OLED_SDA     PB7

#define BOT_FIXED_AREA 0 // Number of lines in bottom fixed area (lines counted from bottom of screen)
#define TOP_FIXED_AREA 0 // Number of lines in top fixed area (lines counted from top of screen)
#define ILI9341_VSCRDEF  0x33
#define ILI9341_VSCRSADD 0x37

void setupPS2(uint8_t kb_type);
uint8_t ps2read();
void    endPS2();

#define OLED_BLACK  0
#define OLED_WHITE  1

static const uint16_t tbl_color[]  = { OLED_BLACK, OLED_WHITE };

// GVRAMサイズ取得
__attribute__((always_inline)) uint16_t tOLEDScreen::getGRAMsize() {
  return this->g_width*(this->g_height>>3);
}

// グラフィク表示用メモリアドレス参照
__attribute__((always_inline)) uint8_t* tOLEDScreen::getGRAM() {
  return this->oled->VRAM();	
}

void tOLEDScreen::update() {
  this->oled->display();
}  

// 初期化
void tOLEDScreen::init(const uint8_t* fnt, uint16_t ln, uint8_t kbd_type, uint8_t* extmem, uint8_t vmode, uint8_t rt) {
  this->font = (uint8_t*)fnt;
/*
  this->oled = new Adafruit_SH1106(OLED_DC, OLED_RST, OLED_CS, 2); // Use hardware SPI2
  this->oled->begin();
*/

  this->oled = new Adafruit_SH1106( OLED_RST); // Use hardware I2C1
  this->oled->begin(SH1106_SWITCHCAPVCC, SH1106_I2C_ADDRESS); 


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
void tOLEDScreen::INIT_DEV() {

}


// スクリーンモード設定
void tOLEDScreen::setScreen(uint8_t mode, uint8_t rt) {
  this->oled->setRotation(rt);
  this->g_width  = this->oled->width();             // 横ドット数
  this->g_height = this->oled->height();            // 縦ドット数

  this->fontEx = mode;
  this->f_width  = *(font+0)*this->fontEx;         // 横フォントドット数
  this->f_height = *(font+1)*this->fontEx;         // 縦フォントドット数
  this->width  = this->g_width  / this->f_width;   // 横文字数
  this->height = this->g_height / this->f_height;  // 縦文字数
  this->fgcolor = OLED_WHITE;
  this->bgcolor = OLED_BLACK;
  this->oled->setCursor(0, 0);
  pos_gx =0;  pos_gy =0;
}

// カーソル表示
uint8_t tOLEDScreen::drawCurs(uint8_t x, uint8_t y) {
  uint8_t c;
  c = VPEEK(x, y);
#if TFT_FONT_MODE == 0
  this->oled->fillRect(x*f_width,y*f_height,f_width,f_height,fgcolor);
 if (fontEx == 1)
    this->oled->drawBitmap(x*f_width,y*f_height,font+3+((uint16_t)c)*8,f_width,f_height, bgcolor);
 else
	drawBitmap_x2(x*f_width,y*f_height,font+3+((uint16_t)c)*8,f_width/fontEx, f_height/fontEx, bgcolor, fontEx);
#endif
#if TFT_FONT_MODE > 0
  this->oled->drawChar(x*f_width,y*f_height, c, ILI9341_BLACK, ILI9341_WHITE, TFT_FONT_MODE);
#endif
  this->oled->display();
}

// 文字の表示
void tOLEDScreen::WRITE(uint8_t x, uint8_t y, uint8_t c) {
#if TFT_FONT_MODE == 0
  this->oled->fillRect(x*f_width,y*f_height,f_width,f_height, bgcolor);
  if (fontEx == 1)
    this->oled->drawBitmap(x*f_width,y*f_height,font+3+((uint16_t)c)*8,f_width,f_height, fgcolor);
  else
    drawBitmap_x2(x*f_width,y*f_height,font+3+((uint16_t)c)*8,f_width/fontEx,f_height/fontEx, fgcolor, fontEx);
#endif
#if TFT_FONT_MODE > 0
  this->oled->drawChar(x*f_width,y*f_height, c, fgcolor, bgcolor, TFT_FONT_MODE);
#endif
  //this->oled->display();
}

// グラフィックカーソル設定
void tOLEDScreen::set_gcursor(uint16_t x, uint16_t y) {
  this->pos_gx = x;
  this->pos_gy = y;
}

// グラフィック文字表示
void  tOLEDScreen::gputch(uint8_t c) {
  drawBitmap_x2( this->pos_gx, this->pos_gy, this->font+3+((uint16_t)c)*8,
    	           this->f_width/ this->fontEx,  this->f_height/this->fontEx,  this->fgcolor, this->fontEx);

  this->pos_gx += this->f_width;
  if (this->pos_gx + this->f_width >= this->g_width) {
     this->pos_gx = 0;
     this->pos_gy += this->f_height;
  }
  this->oled->display();
};


// 画面全消去
void tOLEDScreen::CLEAR() {
  this->oled->fillScreen( bgcolor);
  this->oled->display();
  pos_gx =0;  pos_gy =0;
}

// 行の消去
void tOLEDScreen::CLEAR_LINE(uint8_t l) {
  this->oled->fillRect(0,l*f_height,g_width,f_height,bgcolor);
  this->oled->display();
}


// スクロールアップ
void tOLEDScreen::SCROLL_UP() {
  switch ( this->oled->getRotation() ) {
  case 0: // 横
    memmove(this->oled->VRAM(), this->oled->VRAM()+128, 128*7);
    memset(this->oled->VRAM()+128*7,0,128);
    break;
  case 2: // 横（逆)
    memmove(this->oled->VRAM()+128, this->oled->VRAM(), 128*7);
    memset(this->oled->VRAM(),0,128);
    break;
  case 1: // 縦
    for (uint16_t i=0; i < 8; i++) {
      memmove(this->oled->VRAM()+128*i+this->f_height ,this->oled->VRAM()+128*i, 128-this->f_height);
      memset(this->oled->VRAM()+128*i, 0, this->f_height);
    }
    break;
  case 3: // 縦(逆)
    for (uint16_t i=0; i < 8; i++) {
      memmove(this->oled->VRAM()+128*i ,this->oled->VRAM()+128*i+this->f_height, 128-this->f_height);
      memset(this->oled->VRAM()+128*i+128-this->f_height, 0, this->f_height);
    }
    break;
  }  
  this->oled->display();

}

// スクロールダウン
void tOLEDScreen::SCROLL_DOWN() {
  //INSLINE(0);
  switch ( this->oled->getRotation() ) {
  case 2: // 横
    memmove(this->oled->VRAM(), this->oled->VRAM()+128, 128*7);
    memset(this->oled->VRAM()+128*7,0,128);
    break;
  case 0: // 横（逆)
    memmove(this->oled->VRAM()+128, this->oled->VRAM(), 128*7);
    memset(this->oled->VRAM(),0,128);
    break;
  case 3: // 縦
    for (uint16_t i=0; i < 8; i++) {
      memmove(this->oled->VRAM()+128*i+this->f_height ,this->oled->VRAM()+128*i, 128-this->f_height);
      memset(this->oled->VRAM()+128*i, 0, this->f_height);
    }
    break;
  case 1: // 縦(逆)
    for (uint16_t i=0; i < 8; i++) {
      memmove(this->oled->VRAM()+128*i ,this->oled->VRAM()+128*i+this->f_height, 128-this->f_height);
      memset(this->oled->VRAM()+128*i+128-this->f_height, 0, this->f_height);
    }
    break;
  }  
  this->oled->display();
}

// 指定行に1行挿入(下スクロール)
void tOLEDScreen::INSLINE(uint8_t l) {
  refresh();
}

// 文字色指定
void tOLEDScreen::setColor(uint16_t fc, uint16_t bc) {
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
void tOLEDScreen::setAttr(uint16_t attr) {

}

// ビットマップの拡大描画
void tOLEDScreen::drawBitmap_x2(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h,uint16_t color, uint16_t ex, uint8_t f) {
  int16_t i, j,b=(w+7)/8;
  for( j = 0; j < h; j++) {
    for(i = 0; i < w; i++ ) { 
      if(*(bitmap + j*b + i / 8) & (128 >> (i & 7))) {
        // ドットあり
        if (ex == 1)
           this->oled->drawPixel(x+i, y+j, color); //1倍
        else
          this->oled->fillRect(x + i * ex, y + j * ex, ex, ex, color); // ex倍
      } else {
        // ドットなし
        if (f) {
          // 黒を透明扱いしない
          if (ex == 1)      
            this->oled->drawPixel(x+i, y+j, bgcolor);
          else
            this->oled->fillRect(x + i * ex, y + j * ex, ex, ex, bgcolor);
       }
     }
   }
 }
  //this->oled->display();
}
/*
// カラービットマップの拡大描画
// 8ビット色: RRRGGGBB 
void tOLEDScreen::colorDrawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t ex, uint8_t f) {
  int16_t i, j;
  uint16_t color8,color16,r,g,b;
  for( j = 0; j < h; j++) {
    for(i = 0; i < w; i++ ) { 
      color8   = *(bitmap+w*j+i);
      r =  color8 >>5;
      g = (color8>>2)&7;
      b =  color8 & 3;
      color16 = this->oled->color565(r<<5,g<<5,b<<6); // RRRRRGGGGGGBBBBB
      if ( color16 || (!color16 && f) ) {
        // ドットあり
        if (ex == 1) {
          // 1倍
           this->oled->drawPixel(x+i, y+j, color16);
        } else {
            this->oled->fillRect(x + i * ex, y + j * ex, ex, ex, color16);
        }
     }
   }
 }
}
*/

// ドット描画
void tOLEDScreen::pset(int16_t x, int16_t y, uint16_t c) {
  if (c<=8)
    c = tbl_color[c];  
  this->oled->drawPixel(x, y, c);
  this->oled->display();
}

// 線の描画
void tOLEDScreen::line(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t c){
  if (c<=8)
    c = tbl_color[c];
  this->oled->drawLine(x1, y1, x2, y2, c);
  this->oled->display();
}

// 円の描画
void tOLEDScreen::circle(int16_t x, int16_t y, int16_t r, uint16_t c, int8_t f) {
 if (c<=8)
    c = tbl_color[c];
 if(f)
   this->oled->fillCircle(x, y, r, c);
 else
   this->oled->drawCircle(x, y, r, c);
  this->oled->display();
}

// 四角の描画
void tOLEDScreen::rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t c, int8_t f) {
 if(f)
   this->oled->fillRect(x, y, w, h, c);
 else
   this->oled->drawRect(x, y, w, h, c);
 this->oled->display();
}

// ビットマップの描画
void  tOLEDScreen::bitmap(int16_t x, int16_t y, uint8_t* adr, uint16_t index, uint16_t w, uint16_t h, uint16_t d, uint8_t rgb) {
  uint8_t*bmp;
  if (rgb == 0) {
    bmp = adr + ((w + 7) / 8) * h * index;
    this->drawBitmap_x2(x, y, (const uint8_t*)bmp, w, h, fgcolor, d, 1);
    this->oled->display();
  } else {
/*
    bmp = adr + w * h * index;
    this->colorDrawBitmap(x, y, (const uint8_t*)bmp, w, h, d, 1);
*/
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
/*
uint8_t tOLEDScreen::bmpDraw(char *filename, uint8_t x, uint16_t y, uint16_t bx, uint16_t by, uint16_t bw, uint16_t bh) {

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
  
  if((x >= this->oled->width()) || (y >= this->oled->height())) return 10;
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
        
        if((x+w-1) >= this->oled->width())  w = this->oled->width()  - x;
        if((y+h-1) >= this->oled->height()) h = this->oled->height() - y;
       
        // Set TFT address window to clipped image bounds
        this->oled->setAddrWindow(x, y, x+bw-1, y+bh-1);
        for (row = by; row < h; row++) {
          // ビットマップ画像のライン毎のデータ処理
          
          // 格納画像の向きの補正
          if(flip)
            pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize + bx*3;
          else 
            pos = bmpImageoffset + row * rowSize + bx*3;
          Serial.print("pos=");
          Serial.println(pos,DEC);
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
              this->oled->pushColor(this->oled->color565(r,g,b));
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
*/
