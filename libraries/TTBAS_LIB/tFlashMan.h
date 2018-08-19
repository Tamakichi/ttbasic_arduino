//
// 豊四季Tiny BASIC for Arduino STM32 フラッシュメモリ統合管理クラス
// 2017/11/07 by たま吉さん
// 2018/08/18 by たま吉さん,システム設定にNTSC横・縦補正の追加
//

#ifndef __tFlashMan_h__
#define __tFlashMan_h__

#include <Arduino.h>
#include <TFlash.h>
#include <EEPROM.h>
#include <ttbasic_types.h>

// *** システム設定関連 **************
#define CONFIG_NTSC      65534  // EEPROM NTSC垂直同期補正
#define CONFIG_KBD       65533  // EEPROM キーボード設定
#define CONFIG_PRG       65532  // 自動起動設定
#define CONFIG_NTSC_HPOS 65531  // EEPROM NTSC横位置補正
#define CONFIG_NTSC_VPOS 65530  // EEPROM NTSC縦位置補正

class tFlashMan {
 private:
  uint16_t _totalPageNum;  // ページ総数
  uint16_t _pageSize;      // 1ページ内バイト数
  uint16_t _prgPageNum;    // 1プログラム当たりのページ数
  uint16_t _maxPrgNum;     // プログラム保存可能数
  uint16_t _topPrgPageNo;  // プログラム保存用先頭ページ番号
 
 public:

  // コンストラクタ
  //  totalPageNum 総ページ数, pageSize 1ページ内バイト数,
  //  maxPrgNum プログラム保存可能数, prgPageNum 1プログラム当たりのページ数
  tFlashMan(uint16_t totalPageNum, uint16_t pageSize,
  uint16_t maxPrgNum, uint16_t prgPageNum )
  { init(totalPageNum,pageSize,maxPrgNum, prgPageNum); };
   
  // 初期設定
  //  totalPageNum 総ページ数, pageSize 1ページ内バイト数,
  //  maxPrgNum プログラム保存可能数, prgPageNum 1プログラム当たりのページ数
  void init(uint16_t totalPageNum,uint16_t pageSize,
            uint16_t maxPrgNum,uint16_t prgPageNum) ;  
  
  // 総ページ数の取得
  uint16_t getTotalPageNum()
    {return _totalPageNum; };
  
  // 1ページ内バイト数取得
  uint16_t getPageSize()
    {return _pageSize; };
  
  // １プログラム当たりのページ数の取得
  uint16_t getPrgPageNum()
    {return _prgPageNum; };

  // 指定プログラムの消去
  //  prgNo  プログラム番号
  uint8_t eraceProgram(uint8_t prgNo);

  // 仮想EEPROMから指定アドレスのデータ読み込み
  //  address アドレス,pData 読み込みデータ格納アドレス
  uint8_t EEPRead(uint16_t address,uint16_t* pData);

  // 仮想EEPROMの指定アドレスへのデータ書込み
  //  address  アドレス,data 書込みデータ
  uint16_t EEPWrite(uint16_t address,uint16_t data);

  // 仮想EEPROMにシステム環境設定を保存
  // config :システム環境設定情報
  uint8_t saveConfig(SystemConfig& config);   

  // 仮想EEPROMからシステム環境設定をロード
  // config :  システム環境設定情報
  uint8_t loadConfig(SystemConfig& config);    

  // 仮想EEPROMのフォーマット
  uint8_t EEPFormat();         

  // 指定プログラム 格納先頭アドレスの取得
  // prgNo :  プログラム番号
  uint8_t* getPrgAddress(uint8_t prgNo);

  // 指定プログラムの有無のチェック
  //  prgNo : プログラム番号
  uint8_t isExistPrg(uint8_t prgNo);
  
  // 指定プログラムの削除
  //  prgNo : プログラム番号
  uint8_t eraseProgram(uint8_t prgNo);

  // 指定プログラムの保存
  //  prgNo : プログラム番号, prgData : プログラム格納アドレス
  uint8_t saveProgram(uint8_t prgNo,uint8_t* prgData);
  
  // 指定プログラムのロード
  //  prgNo : プログラム番号, prgData : プログラム格納アドレス
  uint8_t loadProgram(uint8_t prgNo,uint8_t* prgData);

};
#endif
