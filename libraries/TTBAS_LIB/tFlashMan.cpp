//
// 豊四季Tiny BASIC for Arduino STM32 フラッシュメモリ統合管理クラス
// 2017/11/07 by たま吉さん
//

#include <tFlashMan.h>
#include <ttbasic_error.h>
#define FLASH_START_ADDRESS    ((uint32_t)(0x8000000))   // STM32 共通フラッシュメモリ先頭アドレス
#define FLATH_EEPROM_USE       2                         // 仮想EEPROM利用ページ数

#include <EEPROM.h>
extern EEPROMClass EEPROM;

// EEPROMステータスをTTBASICエラーコードに変換する
static uint8_t eeperrorTo(uint16_t status) {
  uint8_t rc; 
  switch(status) {
      case EEPROM_OUT_SIZE:     rc = ERR_EEPROM_OUT_SIZE;      break;
      case EEPROM_BAD_ADDRESS:  rc = ERR_EEPROM_BAD_ADDRESS;   break;
      case EEPROM_NOT_INIT:     rc = ERR_EEPROM_NOT_INIT;      break;
      case EEPROM_NO_VALID_PAGE:rc = ERR_EEPROM_NO_VALID_PAGE; break;
      case EEPROM_OK:           rc = ERR_OK;                   break;
      case EEPROM_BAD_FLASH:
      default:                  rc = ERR_EEPROM_BAD_FLASH;     break;
   }
   return rc;
}

// 初期設定
//   フラッシュメモリ利用のための初期化を行う
// 引数
//   totalPageNum  ：総ページ数
//   pageSize      ：1ページ内バイト数
//   prgPageNum    ：1プログラム当たりのページ数
//
void tFlashMan::init(uint16_t totalPageNum, uint16_t pageSize, uint16_t maxPrgNum, uint16_t prgPageNum) {
  _totalPageNum = totalPageNum ; // 総ページ数
  _pageSize     = pageSize ;     // 1ページ内バイト数
  _maxPrgNum    = maxPrgNum;     // プログラム保存可能数
  _prgPageNum   = prgPageNum ;   // 1プログラム当たりのページ数
  
  // プログラム保存先頭ページ番号の計算
  _topPrgPageNo = _totalPageNum - _prgPageNum * _maxPrgNum - FLATH_EEPROM_USE;
  
  // EEPROM(エミュレーション)の利用設定
  EEPROM.PageBase0 = (uint32_t)(FLASH_START_ADDRESS + (_totalPageNum-2) * (uint32_t)_pageSize);
  EEPROM.PageBase1 = (uint32_t)(FLASH_START_ADDRESS + (_totalPageNum-1) * (uint32_t)_pageSize);
  EEPROM.PageSize  = _pageSize;

}

//
// 指定プログラムの消去
// 引数
//   prgNo  ：プログラム番号
// 戻り値
//   0: 正常終了、0以外:異常終了
//
uint8_t tFlashMan::eraceProgram( uint8_t prgNo ) {
  TFLASH_Status status; // フラッシュメモリ操作ステータス
  uint8_t rc = 0;       // 関数戻り値

  // 書き込みロック解除
  TFlash.unlock();

  // ページ単位の消去処理
  for (uint8_t i=0; i < _prgPageNum; i++) {
    status = TFlash.erasePage(FLASH_START_ADDRESS + _pageSize*(_topPrgPageNo + prgNo * _prgPageNum + i));
    if (status != TFLASH_COMPLETE) {
      // フラッシュメモリ消去エラー
      rc = ERR_SYS;
      break;
    };
  }
  
  //  書き込みロック
  TFlash.lock();        
  return rc;
}

// 仮想EEPROMから指定アドレスのデータ読み込み
// 引数
//  address  : アドレス
//  data     : 読み込みデータ格納アドレス
// 戻り値
//  0:正常終了 0以外異常
uint8_t tFlashMan::EEPRead(uint16_t address, uint16_t* pData) {
  uint8_t  rc =0 ;  // 関数戻り値
  uint16_t Status;  // 仮想EEPROM操作ステータス

  // 仮想EEPROMからデータ取得
  Status = EEPROM.read(address, pData);  
  rc = eeperrorTo(Status);
  if (rc == ERR_EEPROM_BAD_ADDRESS) {
    // 保存データが無い場合は0を返す
    rc = 0;
    *pData = 0;
  }  
  return rc;
}

// 仮想EEPROMの指定アドレスへのデータ書込み
// 引数
//  address  : アドレス
//  data     : 書込みデータ
// 戻り値
//  0:正常終了 0以外異常
uint16_t tFlashMan::EEPWrite(uint16_t address, uint16_t data) {
  uint8_t  rc = 0;  // 関数戻り値
  uint16_t Status;  // 仮想EEPROM操作ステータス
  
  // データの書込み
  Status = EEPROM.write(address, data);
  rc = eeperrorTo(Status);
  return rc;
}

// 仮想EEPROMにシステム環境設定を保存
// 引数
//  config  :システム環境設定情報
// 戻り値
//  0:正常終了 0以外異常
uint8_t tFlashMan::saveConfig(SystemConfig& config) {
  int16_t  rc = 0;   // 関数戻り値
  uint16_t data;     // 仮想EEPROM書き込みデータ
  uint16_t Status;   // 仮想EEPROM書き込みステータス

  // EEPROM書き込み回数を取得し、エラーならフォーマット
  rc = EEPROM.count(&data);
  if (rc != EEPROM_OK) {
     EEPFormat();
     if (EEPFormat())
      return ERR_EEPROM_BAD_FLASH;
  }

  // 仮想EEPROMに設定値を保存する
  Status = EEPROM.write(CONFIG_NTSC, (uint16_t)config.NTSC);
  if (Status != EEPROM_OK) goto ERR_EEPROM;
  Status = EEPROM.write(CONFIG_KBD, (uint16_t)config.KEYBOARD);
  if (Status != EEPROM_OK) goto ERR_EEPROM;
  Status = EEPROM.write(CONFIG_PRG, (uint16_t)config.STARTPRG);
  if (Status != EEPROM_OK) goto ERR_EEPROM;
  goto DONE;

ERR_EEPROM:
    // エラーの場合
  rc = eeperrorTo(Status);
  return rc;

DONE:
  // 正常の場合
  return rc;
}

// 仮想EEPROMからシステム環境設定をロー
// 引数
//  config  :システム環境設定情報
// 戻り値
//  0:正常終了 0以外異常
uint8_t tFlashMan::loadConfig(SystemConfig& config) {
  uint8_t rc =1;           // 関数戻り値
  uint16_t data;           // 仮想EEPROMからの取り出しデータ
  config.NTSC      =  0;   // NTSC補正値のデフォルト値
  config.KEYBOARD  =  0;   // キーボード設定のデフォルト値(JP)
  config.STARTPRG  = -1;   // 自動起動のデフォルト値(自動起動なし)

  while(1) {
  // NTSC設定の参照
    if (EEPROM.read(CONFIG_NTSC, &data) == EEPROM_OK) {
      config.NTSC = data;
      rc = 0;
    } else {
      break;
    }
      
    // キーボード設定の参照
    if (EEPROM.read(CONFIG_KBD, &data) == EEPROM_OK) {
      config.KEYBOARD = data;
      rc = 0;
    } else {
      break;
    }
    if (EEPROM.read(CONFIG_PRG, &data) == EEPROM_OK) {
      config.STARTPRG = data;  
      rc = 0;
    }
    break;
  }  
  return rc;
}

// 仮想EEPROMのフォーマット
// 引数
//  prgNo  :プログラム番号
// 戻り値
//  0:正常終了 0以外異常
uint8_t tFlashMan::EEPFormat() {
  uint8_t rc =0;           // 関数戻り値
  uint16_t Status;
  Status = EEPROM.format();
  rc = eeperrorTo(Status);
  return rc;
}

// 指定プログラム 格納先頭アドレスの取得
// 引数
//  prgNo  :プログラム番号
// 戻り値
//  格納先頭アドレスの取得
uint8_t* tFlashMan::getPrgAddress(uint8_t prgNo) {
  return (uint8_t*)(FLASH_START_ADDRESS + (uint32_t)(_topPrgPageNo+prgNo*_prgPageNum) * (uint32_t)_pageSize); 
}

// 指定プログラムの消去
// 引数
//  prgNo   :プログラム番号
// 戻り値
//  0:正常終了 0以外異常
uint8_t tFlashMan::eraseProgram(uint8_t prgNo) {
  TFLASH_Status status = TFLASH_COMPLETE;
  TFlash.unlock();
  uint32_t flash_adr = (uint32_t)getPrgAddress(prgNo); // アドレスの取得  
    
  for (uint8_t i = 0; i < _prgPageNum; i++) {
    status = TFlash.erasePage(flash_adr + _pageSize*i );
      if (status != TFLASH_COMPLETE) {
         break;
      }
  }
  TFlash.lock();
  return status == TFLASH_COMPLETE ? 0:1;
}


// 指定プログラムの保存
// 引数
//  prgNo   :プログラム番号
//  prgData :プログラム格納アドレス
// 戻り値
//  0:正常終了 0以外異常
uint8_t tFlashMan::saveProgram(uint8_t prgNo, uint8_t* prgData) {
  TFLASH_Status status;
  uint32_t flash_adr = (uint32_t)getPrgAddress(prgNo);
  
  // フラッシュメモリ消去
  if (eraseProgram(prgNo)) 
    return 1;
  
  // 内部フラッシュメモリへの保存
  TFlash.unlock();
  for (uint8_t i = 0; i < _prgPageNum; i++) {
    status = TFlash.write((uint16_t*)flash_adr, prgData + _pageSize * i, _pageSize);
    flash_adr += _pageSize;
  }
  TFlash.lock();
  return 0;
}

// 指定プログラムの有無のチェック
// 引数
//  prgNo   :プログラム番号
// 戻り値
//  1:有り 0 無し

uint8_t tFlashMan::isExistPrg(uint8_t prgNo) {
  // 指定領域に保存されているかチェックする
  uint8_t* flash_adr = getPrgAddress(prgNo);
  if ( *flash_adr == 0xff && *(flash_adr+1) == 0xff)
    return false;
  else 
    return true;
}

// 指定プログラムのロード
// 引数
//  prgNo   :プログラム番号
//  prgData :プログラム格納アドレス
// 戻り値
//  0:正常終了 0以外異常
uint8_t tFlashMan::loadProgram(uint8_t prgNo, uint8_t* prgData) {
 uint8_t* flash_adr = getPrgAddress(prgNo);

  // 指定領域に保存されているかチェックする
  if (!isExistPrg(prgNo)) {
    return ERR_NOPRG;
  }
  // 現在のプログラムの削除とロード
  memcpy(prgData , flash_adr, _pageSize * _prgPageNum);
  return 0;
}
