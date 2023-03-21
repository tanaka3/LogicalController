
/**
 * https://github.com/joshmarinacci/nespad-arduino/
 */

#include <M5Stack.h>
#include <M5GFX.h>
#include <NESpad.h>
#include <driver/i2s.h>

#include "image.h"

#define IN0 32  //入力端子
#define IN1 33

// Basic
// #define FC_STROBE  26
// #define FC_CLOCK  12
// #define FC_DATA  13

// Fire
#define FC_STROBE  22
#define FC_CLOCK  12
#define FC_DATA  21

// #define FC_STROBE  22
// #define FC_CLOCK  21
// #define FC_DATA  12

const uint16_t TFT_BACKGROUND       = M5GFX::color565(151, 38, 38);
const uint16_t TFT_BASEGROUND       = M5GFX::color565(208, 185, 151);
const uint16_t TFT_USE_BOTTON       = M5GFX::color565(255, 0, 12);

// ボタンとHIDの変換マップ
const uint8_t padMap[16] = {5, 8, 6, 9,
                          2, 5, 3, 5,
                          4, 7, 5, 5,
                          1, 5, 5, 5};

//strobe / clock / data
// Fire
NESpad nespad_a = NESpad(26, 17, 16); //25, 26, 27
NESpad nespad_b =  NESpad(13, 2, 5); //13, 14 19

// Basic
// NESpad nespad_a = NESpad(2, 16, 17);
// NESpad nespad_b =  NESpad(5, 21, 22);

volatile byte beforeController = 0;

M5GFX display;

// 画像の情報
typedef struct _img {
  const unsigned char *img;   
  uint8_t width;
  uint8_t height;
} Img;

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

const uint8_t ButtonMap[4] ={ NES_A, NES_B,
                              NES_SELECT, NES_START };

const uint8_t PadMap[4] ={ NES_UP, NES_DOWN,
                              NES_LEFT, NES_RIGHT };

bool notFlag = false;
bool ctlMode = false;
bool isRnd = false;
bool isTouch = false;
uint8_t logicType = 0;
uint8_t randButtonMap[4];
uint8_t randPadMap[4];

volatile int sendIndex = 0;
volatile byte sendData = 0;

void setup() {
  M5.begin();
  display.begin();

  M5.Speaker.begin(); // これが無いとmuteしても無意味です。
  M5.Speaker.mute();

  generateMap(isRnd);
  
  // GCの初期化
  //Serial2.begin(115200, SERIAL_8N1, 32, 33);// Grove

  display.fillScreen(TFT_BACKGROUND);
  display.fillRect(10, 10,  100, 40, TFT_BASEGROUND);
  display.fillRect(10, 40,  300, 190, TFT_BASEGROUND);
  display.drawPng(LOGO_TITLE, sizeof(LOGO_TITLE), 20, 61);

  delay(3000);

  drawViewBackground(ctlMode, isRnd, logicType, notFlag);

  // put your setup code here, to run once:
  pinMode(FC_STROBE, INPUT);
  pinMode(FC_CLOCK,  INPUT);
  pinMode(FC_DATA, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(FC_STROBE),func_strobe,RISING);
  attachInterrupt(digitalPinToInterrupt(FC_CLOCK),func_clock,RISING);  
}

void func_strobe(){
  sendIndex = 0;
  sendData = beforeController;

  data(sendIndex, sendData);  
}

void func_clock(){
  sendIndex++;
  if(sendIndex < 8){
    data(sendIndex, sendData);
  }
}

void data(int index, int data){
  int flag = data & (1 << index);
  digitalWrite(FC_DATA, flag == 0 ? HIGH : LOW);
}


void loop() {

  // コントローラーの状態を取得
  byte controller_a = nespad_a.buttons();
  delay(1);
  byte controller_b =  nespad_b.buttons();

  // Serial.print("A:");
  // Serial.println(controller_a);
  // Serial.print("B:");
  // Serial.println(controller_b);


  byte controller = getController(logicType + (notFlag ?  3: 0), nespad_a.buttons(), nespad_b.buttons());

  if(beforeController != controller){
    //sendGC(controller);
    beforeController = controller;
    if(ctlMode){
      drawControllerView(controller);
    }
  }

  M5.update();

  if (M5.BtnA.wasReleased()) {

    if(ctlMode){
      ctlMode = false;
    }
    else{
      logicType ++;
      if(logicType > 2){
        logicType = 0;
      }
      beforeController = -1;
      
    }
    drawViewBackground(ctlMode, isRnd, logicType, notFlag);    
  }
  // Notモードに切り替え
  else if (M5.BtnB.wasReleased()) {
    if(!ctlMode){
      notFlag = !notFlag;
    }
    beforeController = -1;
    drawViewBackground(ctlMode, isRnd, logicType, notFlag);
  }
  // コントローラーモードの切り替え
  else if (M5.BtnC.wasReleased()) {
    ctlMode = true;
    drawViewBackground(ctlMode, isRnd, logicType, notFlag);

  }
  
  // if( M5.Touch.ispressed() && !isTouch){
  //   TouchPoint_t point = M5.Touch.getPressPoint();
  //     if(point.y >70 && point.y < 250 ){
  //       isTouch = true;
  //     }
  // }
  // else if(!M5.Touch.ispressed() && isTouch){
  //     isTouch = false;
  //     isRnd = !isRnd;
  //     generateMap(isRnd);
  //     drawRndIcon(isRnd);

  // }

  display.display();
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


  //#define NES_A       B00000001
  //#define NES_B       B00000010
  //#define NES_SELECT  B00000100
  //#define NES_START   B00001000   
  //#define NES_UP      B00010000
  //#define NES_DOWN    B00100000
  //#define NES_LEFT    B01000000
  //#define NES_RIGHT   B10000000
byte getController(uint8_t opeType, byte a, byte b){

  byte controller = 0;
  
  // コントローラーの状態を取得
  byte controller_a = nespad_a.buttons();
  byte controller_b = nespad_b.buttons();

  // AボタンとBボタン
  if(operation(opeType, controller_a & NES_A, controller_b & NES_A)) controller |= NES_A;
  if(operation(opeType, controller_a & NES_B, controller_b & NES_B)) controller |= NES_B;

  // スタートと、セレクトは論理演算の対象外とする
  if(operation(1, controller_a & NES_SELECT, controller_b & NES_SELECT)) controller |= NES_SELECT; 
  if(operation(1, controller_a & NES_START, controller_b & NES_START)) controller |=  NES_START;

  bool up = operation(opeType, controller_a & NES_UP, controller_b & NES_UP);
  bool down = operation(opeType, controller_a & NES_DOWN, controller_b & NES_DOWN);
  bool left = operation(opeType, controller_a & NES_LEFT, controller_b & NES_LEFT);
  bool right = operation(opeType, controller_a & NES_RIGHT, controller_b & NES_RIGHT);

  if(!(up &  down)){
    if(up) controller |= NES_UP;
    if(down) controller |= NES_DOWN;
  }

  // ← と → の同時入力は不可能なため無効とする
  if(!(left &  right)){
    if(left) controller |= NES_LEFT;
    if(right) controller |= NES_RIGHT;
  }

  // Serial.print("a:");
  // Serial.print(controller_a);
  // Serial.print("  b:");
  // Serial.print(controller_b);
  // Serial.print("  =>:");
  // Serial.println(controller);


  return controller;
}

 // 8 - : select
  // 9 + : start
  // 2 A : A
  // 1 B : B
void sendGC(byte controller){
  //ボタン系
  byte btnCmd[4];
  memset(btnCmd, 0x00, 4);  
  btnCmd[0] = 0xc1;
  btnCmd[1] = 0;

  // AボタンとBボタン
  //if(controller & NES_A) btnCmd[2] |= 0x04;
  //if(controller & NES_B) btnCmd[2] |= 0x02;
  if(controller & ButtonMap[randButtonMap[0]]) btnCmd[2] |= 0x04;
  if(controller & ButtonMap[randButtonMap[1]] ) btnCmd[2] |= 0x02;

  // スタートと、セレクトは論理演算の対象外とする
  //if(controller & NES_SELECT) btnCmd[3] |= 0x01; 
  //if(controller & NES_START) btnCmd[3] |=  0x02;
  if(controller & ButtonMap[randButtonMap[2]]) btnCmd[3] |= 0x01; 
  if(controller & ButtonMap[randButtonMap[3]]) btnCmd[3] |= 0x02;

  // Serial.print("3:");
  // Serial.print(btnCmd[2]);
  // Serial.print(" 4:");
  // Serial.println(btnCmd[3]);

  Serial2.write(btnCmd, 4);
  
  // // PAD
  byte padCmd[4];
  memset(padCmd, 0x00, 4);  
  padCmd[0] = 0xc0;
  padCmd[1] = -1;

  byte pad = 0;

  // ↑ と ↓ の同時入力は不可能なため無効とする
  // if(!(controller & NES_UP &&  controller & NES_DOWN)){
  //   if(controller & NES_UP) pad |= 1;
  //   if(controller & NES_DOWN) pad |= 4;
  // }
  // ↑ と ↓ の同時入力は不可能なため無効とする
  if(!(controller & PadMap[randPadMap[0]] &&  controller & PadMap[randPadMap[1]])){
    if(controller & PadMap[randPadMap[0]]) pad |= 1;
    if(controller & PadMap[randPadMap[1]]) pad |= 4;
  }

  // ← と → の同時入力は不可能なため無効とする
  // if(!(controller & NES_LEFT && controller & NES_RIGHT)){
  //   if(controller & NES_LEFT) pad |= 8;
  //   if(controller & NES_RIGHT) pad |= 2;
  // }
  // ← と → の同時入力は不可能なため無効とする
  if(!(controller & PadMap[randPadMap[2]] && controller & PadMap[randPadMap[3]])){
    if(controller & PadMap[randPadMap[2]]) pad |= 8;
    if(controller & PadMap[randPadMap[3]]) pad |= 2;
  }

  padCmd[2] = padMap[pad];
  Serial2.write(padCmd, 4);

  // Serial.print("pad:");
  // Serial.print(pad);
  // Serial.print(" padCmd:");
  // Serial.println(padCmd[2]);
}

void generateMap(boolean isRnd){

    for(uint8_t i=0; i<4; i++){
      randButtonMap[i] = i;
      randPadMap[i] = i;
    }

    if(isRnd){
      randomSeed(millis());

      //シャッフル
      for(uint8_t i=3; i>0; i--){
        uint8_t index = rand() % (i+1);

        uint8_t buf = randButtonMap[i];
        randButtonMap[i] = randButtonMap[index];
        randButtonMap[index] = buf;

        index = rand() % (i+1);
        buf = randPadMap[i];
        randPadMap[i] = randPadMap[index];
        randPadMap[index] = buf;      
      }

      //PADのシャッフル（必UP/DOWN と LEFT/RRIGHTは、相対するようにする（でないとボタン押せない）
      uint8_t position = rand() % 2;
      uint8_t target = rand() % 2;

      randPadMap[position * 2 + target] = 0;
      randPadMap[position * 2 + (1 - target)] = 1;

      target = rand() % 2;
      randPadMap[(1 - position) * 2 + target] = 2;
      randPadMap[(1 - position) * 2 + (1 - target)] = 3;

      // Serial.print("MAP:");
      // for(int i=0; i<4; i++){
      //   Serial.print(randButtonMap[i]);
      //   Serial.print(",");
      // }
      // Serial.println();

      // Serial.print("PAD:");
      // for(int i=0; i<4; i++){
      //   Serial.print(randPadMap[i]);
      //       Serial.print(",");
      // }
      // Serial.println();

    }
}

// コントローラー画面を表示
void drawControllerView(byte controller){
  
  // Serial.print("g:");
  // Serial.print(controller);
  // Serial.print(" - ");
  // Serial.print(ButtonMap[randButtonMap[1]]);
  // Serial.print(" - ");
  // Serial.println(controller &  ButtonMap[randButtonMap[1]]); 

  display.fillCircle(224, 120, 15, controller &  ButtonMap[randButtonMap[1]] ? TFT_USE_BOTTON : TFT_BLACK);
  display.fillCircle(281, 120, 15, controller & ButtonMap[randButtonMap[0]] ? TFT_USE_BOTTON : TFT_BLACK);

  display.fillRect(18, 110, 22, 22, controller & PadMap[randPadMap[2]] ? TFT_USE_BOTTON : TFT_BLACK);
  display.fillRect(52, 75, 22, 22, controller & PadMap[randPadMap[0]] ? TFT_USE_BOTTON : TFT_BLACK);
  display.fillRect(86, 110, 22, 22, controller & PadMap[randPadMap[3]] ? TFT_USE_BOTTON : TFT_BLACK);
  display.fillRect(52, 143,  22, 22, controller & PadMap[randPadMap[1]] ? TFT_USE_BOTTON : TFT_BLACK);

  display.fillRect(123, 173,  23, 5, controller &  ButtonMap[randButtonMap[2]] ? TFT_USE_BOTTON : TFT_BLACK);
  display.fillRect(163, 173,  23, 5, controller &  ButtonMap[randButtonMap[3]] ? TFT_USE_BOTTON : TFT_BLACK);

}

// 背景を表示
void drawViewBackground(bool ctlMode, bool isRnd, uint8_t mode, bool notFlag){
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

  if(isRnd){
    // 33-28
    display.drawPng(ICON_RANDOM, sizeof(ICON_RANDOM), 44, 16);
  }

  if(ctlMode){
    uint8_t flag = mode + (notFlag ?  3: 0);
    display.drawPng(FRAME_CONTROLLER, sizeof(FRAME_CONTROLLER), 13, 70);
  }
  else{
    display.drawPng(FRAME_MENU, sizeof(FRAME_MENU), 25, 195);
    display.fillRect(25, 60, 260, 120, TFT_BASEGROUND);
    int x = 160 -icons[flag].width / 2;
    int y =  51  + (139 - icons[flag].height) /2;
    switch(flag){
      case 0: display.drawPng(ICON_AND, sizeof(ICON_AND), x , y); break;
      case 1: display.drawPng(ICON_OR, sizeof(ICON_OR), x , y); break;
      case 2: display.drawPng(ICON_XOR, sizeof(ICON_XOR), x , y); break; 
      case 3: display.drawPng(ICON_NAND, sizeof(ICON_NAND), x , y); break;
      case 4: display.drawPng(ICON_NOR, sizeof(ICON_NOR), x , y); break;
      case 5: display.drawPng(ICON_XNOR, sizeof(ICON_XNOR), x , y); break; 
    }
  }
  
  display.endWrite();
}

// 画面を表示
void drawRndIcon(bool isRnd){
  display.fillRect(10, 10,  100, 40, TFT_BASEGROUND);

  if(isRnd){
    display.drawPng(ICON_RANDOM, sizeof(ICON_RANDOM), 44, 16);
  }
}

