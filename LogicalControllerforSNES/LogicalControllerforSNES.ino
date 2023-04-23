
/**
 * https://github.com/joshmarinacci/nespad-arduino/
 */
#include <M5Stack.h>
#include <M5GFX.h>
#include <WiiChuck.h>

#include "image.h"

#define IN0 21  //入力端子
#define IN1 22

const uint16_t TFT_BACKGROUND = M5GFX::color565(151, 38, 38);
const uint16_t TFT_BASEGROUND = M5GFX::color565(208, 185, 151);
const uint16_t TFT_USE_BOTTON = M5GFX::color565(255, 0, 12);

// ボタンとHIDの変換マップ
const uint8_t padMap[16] = {5, 8, 6, 9,
                          2, 5, 3, 5,
                          4, 7, 5, 5,
                          1, 5, 5, 5};

M5GFX display;

// 画像の情報
struct Img {
  const unsigned char *img;   
  uint8_t width;
  uint8_t height;
};

struct SNESPad {
  bool a = false;
  bool b = false;
  bool x = false;
  bool y = false;
  bool l = false;
  bool r = false;
  bool start = false;
  bool select = false;
  bool up = false;
  bool down = false;
  bool left = false;
  bool right = false;

  void clear(){
    a = false;
    b = false;
    x = false;
    y = false;
    l = false;
    r = false;
    start = false;
    select = false;
    up = false;
    down = false;
    left = false;
    right = false;
  }

  bool compareTo(SNESPad pad){
    return a == pad.a &&
            b == pad.b &&
            x == pad.x &&
            y == pad.y &&
            l == pad.l &&
            r == pad.r &&
            start == pad.start &&
            select == pad.select &&
            up == pad.up &&
            down == pad.down &&
            left == pad.left &&
            right == pad.right;
  }
};

// アイコンの情報
Img icons[] = {
      {ICON_AND, 233, 139},
      {ICON_OR, 233, 134},
      {ICON_XOR, 233, 125},
      {ICON_NAND, 233, 130},
      {ICON_NOR, 233, 127},
      {ICON_XNOR, 233, 119},
};

// ロゴの情報
Img logos[] = {
      {LOGO_AND, 60, 22},
      {LOGO_OR, 37, 21},
      {LOGO_XOR, 58, 22},
      {LOGO_NAND, 80, 22},
      {LOGO_NOR, 58, 22},
      {LOGO_XNOR, 80, 20},
};

Accessory snespad_a(0);
Accessory snespad_b(1);

SNESPad beforeSNESPad;

bool notFlag = false;
uint8_t logicType = 0;

void setup() {
  M5.begin();
  display.begin();
  
  M5.Speaker.begin();
  M5.Speaker.mute();

  // GCの初期化
  Serial2.begin(115200, SERIAL_8N1, 21, 22);// Grove

  display.fillScreen(TFT_BACKGROUND);
  display.fillRect(10, 10,  100, 40, TFT_BASEGROUND);
  display.fillRect(10, 40,  300, 190, TFT_BASEGROUND);
  display.drawPng(LOGO_TITLE, sizeof(LOGO_TITLE), 20, 61);

	snespad_a.begin(2, 19);
	if (snespad_a.type == Unknown) {
		snespad_a.type = NUNCHUCK;
	}
	snespad_b.begin(5, 13);
	if (snespad_b.type == Unknown) {
		snespad_b.type = NUNCHUCK;
	}

  delay(3000);

  drawViewBackground(logicType, notFlag);
}

void loop() {

  SNESPad pad = getController(logicType + (notFlag ?  3: 0));

  if(!beforeSNESPad.compareTo(pad)){
    sendGC(pad);
    beforeSNESPad = pad;
  }


  M5.update();
  // put your main code here, to run repeatedly:

  if (M5.BtnA.wasReleased()) {

    logicType ++;
    if(logicType > 2){
      logicType = 0;
    }
    beforeSNESPad.clear();
      
    drawViewBackground(logicType, notFlag);    
  }
  // Notモードに切り替え
  else if (M5.BtnB.wasReleased()) {
    notFlag = !notFlag;
    beforeSNESPad.clear();
    drawViewBackground(logicType, notFlag);
  }
  // コントローラーモードの切り替え
  else if (M5.BtnC.wasReleased()) {
    switchReset();    
  }
  
  display.display();
}

void switchReset(){

 //ボタン系
  byte btnCmd[4];
  memset(btnCmd, 0x00, 4);  
  btnCmd[0] = 0xc1;
  btnCmd[1] = 5;

  btnCmd[2] |= 0x40;
  btnCmd[2] |= 0x80;

  btnCmd[3] = 0;

  Serial2.write(btnCmd, 4);  
}

// & and
// | or
// ^ xor
// ~ NOT
// ~& (nandA)
bool operation(uint8_t type, bool a, bool b){
  switch(type){
    case 0:
      return a & b;
    case 1:
      return a | b;
    case 2:
      return a ^ b;
    case 3:
      return !(a & b);
    case 4:
      return !(a | b);
    case 5:
      return !(a ^ b);
  }
  return a & b;
}


SNESPad getController(uint8_t opeType){

  // コントローラーの状態を取得
  snespad_a.readData();
  snespad_b.readData();

  SNESPad controller;

  // AボタンとBボタン
  controller.a = operation(opeType, snespad_a.getButtonA(), snespad_b.getButtonA());
  controller.b = operation(opeType, snespad_a.getButtonB(), snespad_b.getButtonB());

  controller.x = operation(opeType, snespad_a.getButtonX(), snespad_b.getButtonX());
  controller.y = operation(opeType, snespad_a.getButtonY(), snespad_b.getButtonY());

  controller.l = operation(opeType, snespad_a.getTriggerLeft(), snespad_b.getTriggerLeft());
  controller.r = operation(opeType, snespad_a.getTriggerRight(), snespad_b.getTriggerRight());

  // スタートと、セレクトは論理演算の対象外とする
  controller.start = operation(1, snespad_a.getButtonPlus(), snespad_b.getButtonPlus());
  controller.select = operation(1, snespad_a.getButtonMinus(), snespad_b.getButtonMinus());

  //十字キー
  bool up = operation(opeType, snespad_a.getPadUp(), snespad_b.getPadUp());
  bool down = operation(opeType, snespad_a.getPadDown(), snespad_b.getPadDown());
  bool left = operation(opeType, snespad_a.getPadLeft(), snespad_b.getPadLeft());
  bool right = operation(opeType, snespad_a.getPadRight(), snespad_b.getPadRight());

  if(!(up &&  down)){
    if(up) controller.up = true;
    if(down) controller.down = true;
  }

  // ← と → の同時入力は不可能なため無効とする
  if(!(left && right)){
    if(left)  controller.left = true;
    if(right) controller.right = true;
  }

  // Serial.print("a:");
  // Serial.print(controller_a);
  // Serial.print("  b:");
  // Serial.print(controller_b);
  // Serial.print("  =>:");
  // Serial.println(controller);

  return controller;
}

/**
0: Y
1: B
2: A
3: X
4: L
5: R
6: ZL
7: ZR
8: -
9: +
10: L押し込み
11: R押し込み
*/
void sendGC(SNESPad snesPad){
  //ボタン系
  byte btnCmd[4];
  memset(btnCmd, 0x00, 4);  
  btnCmd[0] = 0xc1;
  btnCmd[1] = 0;

  // // 各種ボタン
  if(snesPad.a) btnCmd[2] |= 0x04;
  if(snesPad.b) btnCmd[2] |= 0x02;

  if(snesPad.x) btnCmd[2] |= 0x08;
  if(snesPad.y) btnCmd[2] |= 0x01;

  if(snesPad.l) btnCmd[2] |= 0x10;
  if(snesPad.r) btnCmd[2] |= 0x20;

  // スタートと、セレクトは論理演算の対象外とする
  if(snesPad.select) btnCmd[3] |= 0x01; 
  if(snesPad.start) btnCmd[3] |= 0x02;


  Serial2.write(btnCmd, 4);
  
  // // PAD
  byte padCmd[4];
  memset(padCmd, 0x00, 4);  
  padCmd[0] = 0xc0;
  padCmd[1] = -1;

  byte pad = 0;

  // ↑ と ↓ の同時入力は不可能なため無効とする
  if(!(snesPad.up && snesPad.down)){
    if(snesPad.up) pad |= 1;
    if(snesPad.down) pad |= 4;
  }

  // ← と → の同時入力は不可能なため無効とする
  if(!(snesPad.left && snesPad.right)){
    if(snesPad.left) pad |= 8;
    if(snesPad.right) pad |= 2;
  }

  padCmd[2] = padMap[pad];
  Serial2.write(padCmd, 4);

  // Serial.print("pad:");
  // Serial.print(pad);
  // Serial.print(" padCmd:");
  // Serial.println(padCmd[2]);
}

// 背景を表示
void drawViewBackground(uint8_t mode, bool notFlag){
  uint8_t flag = mode + (notFlag ?  3: 0);

  display.startWrite();     
  display.fillScreen(TFT_BACKGROUND);

  display.fillRect(10, 10,  100, 40, TFT_BASEGROUND);
  display.fillRect(10, 40,  300, 190, TFT_BASEGROUND);

  display.fillRect(110, 10, 210, 40, TFT_BACKGROUND);

  int x = 185 + (60 - logos[flag].width)/2;
  int y = 17;
  switch(flag){
    case 0: display.drawPng(LOGO_AND, sizeof(LOGO_AND), x , y); break;
    case 1: display.drawPng(LOGO_OR, sizeof(LOGO_OR), x , y); break;
    case 2: display.drawPng(LOGO_XOR, sizeof(LOGO_XOR), x , y); break; 
    case 3: display.drawPng(LOGO_NAND, sizeof(LOGO_NAND), x , y); break;
    case 4: display.drawPng(LOGO_NOR, sizeof(LOGO_NOR), x , y); break;
    case 5: display.drawPng(LOGO_XNOR, sizeof(LOGO_XNOR), x , y); break; 
  }

  display.drawPng(FRAME_MENU, sizeof(FRAME_MENU), 25, 195);
  display.fillRect(25, 60, 260, 120, TFT_BASEGROUND);
  
  x = 160 -icons[flag].width / 2;
  y =  51  + (139 - icons[flag].height) /2;
  switch(flag){
    case 0: display.drawPng(ICON_AND, sizeof(ICON_AND), x , y); break;
    case 1: display.drawPng(ICON_OR, sizeof(ICON_OR), x , y); break;
    case 2: display.drawPng(ICON_XOR, sizeof(ICON_XOR), x , y); break; 
    case 3: display.drawPng(ICON_NAND, sizeof(ICON_NAND), x , y); break;
    case 4: display.drawPng(ICON_NOR, sizeof(ICON_NOR), x , y); break;
    case 5: display.drawPng(ICON_XNOR, sizeof(ICON_XNOR), x , y); break; 
  }

  display.endWrite();
}

