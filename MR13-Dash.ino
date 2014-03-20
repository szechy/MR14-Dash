#define MAXBRIGHT         1000
#define MAXFADE           4000
#define MINFADE           500
#define NUM_TLCS          1
#include <Tlc5940_Bitbang.h>

#define MAXGEAR           4
#include <Canbus.h>

enum mode_t {DISABLE, THROT, FIG8, GEAR} mode;


const float intensityLUT[100] = {
0,0.665321631,0.851367746,0.943481469,0.985764774,0.997238078,0.988462113,0.96605972,0.934481052,0.896844838,0.855394808,0.811770947
,0.767181879,0.722520113,0.67844217,0.635426025,0.593813288,0.553840784,0.515664551,0.479378306,0.445027784,0.41262195,0.382141824
,0.353547444,0.326783377,0.301783083,0.278472371,0.256772123,0.236600445,0.217874344,0.200511032,0.184428929,0.169548417,0.155792401
,0.143086714,0.131360389,0.120545834,0.110578927,0.101399044,0.092949042,0.0851752,0.078027132,0.07145768,0.065422788,0.059881367
,0.054795158,0.050128579,0.045848587,0.041924529,0.038328001,0.035032706,0.032014323,0.029250376,0.026720112,0.024404379
,0.022285521,0.020347263,0.018574621,0.016953801,0.015472114,0.014117891,0.012880409,0.011749818,0.010717072,0.009773866
,0.008912584,0.008126237,0.007408418,0.006753254,0.006155364,0.005609818,0.005112101,0.004658081,0.004243975,0.00386632
,0.003521949,0.003207967,0.002921722,0.002660795,0.002422971,0.002206227,0.002008715,0.001828745,0.001664777,0.001515399
,0.001379328,0.001255387,0.001142507,0.001039707,0.000946096,0.000860859,0.000783253,0.0007126,0.000648282,0.000589735
,0.000536445,0.000487944,0.000443803,0.000403634,0.000367082}; //SQRT(t)*e^(-t/10)/1.63

int LEDlut[]    = {1,14,13,12,11,9,8,7,6,4,3,2,5,10};
int LEDdeglut[] = {7,8,9,10,11,0,1,2,3,4,5,6};
int Fig8[]      = {0,1,2,3,13,12,9,8,7,6,5,4,13,12,10,11};

uint8_t buff[8];
uint8_t length;
    
long unsigned int fadeTime = 0;
long unsigned int snakeTime = 0;
long unsigned int CanTime = 0;
long unsigned int lastGoodComms = 0;
long unsigned int countdownTime = 0;

uint8_t fig8Pos = 0;
uint8_t digit = 0;

uint16_t filt;
uint8_t gear = MAXGEAR+1;

void setup() {
  
//  ********  ELI!!!  This is that line that you will want to edit!! Change other things at your onw risk *******
  mode = DISABLE; // options are DISABLE, THROT, FIG8, and GEAR
  
  
  Serial.begin(115200);
  
  if     (mode == GEAR)  filt = 0x777;
  else if(mode == THROT) filt = 0x773;
  else                   filt = 0x00;
  
  while(!Canbus.init(CANSPEED_1000, filt)) {
    Serial.println("Can't init CAN");
    delay(1);
  }
  Serial.println("CAN Init ok");
  delay(1);
  
  Tlc.init();
  delay(1);
  Tlc.clear();
  Tlc.update();
  
  if(0){
    for(int j = 9; j>=0; j--) {
      Tlc.clear();
      segmentPut(j);
      Tlc.update();
      for(int i = 0; i<200; i++){
        fadeDown(0.98, MINFADE);
        Tlc.update();
        delay(2);
      }
    }
    Tlc.clear();
    Tlc.update();
  }
}

void loop() {
  unsigned long int loopTime = millis();
  
  switch(mode) {
    case(GEAR):  // ********* Gear readout ***************
      if(Canbus.message_rx(buff, &length )) {
        uint8_t temp = buff[0];
        if(gear != temp) { // Gear has changed, update display
          gear = temp;
          Tlc.clear();
          if(temp > MAXGEAR) segmentPut(10);  // Gear is unknown, indicate Nutral position
          else if(temp)      segmentPut(temp);
          else               segmentPut(10);  // Gear is 0,       indicate Nutral position
          fadeTime = loopTime;  // reset fade timer
          Tlc.update();
        }
        if(temp <= MAXGEAR) lastGoodComms = loopTime;
      }
      
      if(loopTime-lastGoodComms > 500) { // I have not recieved a message in a "while"
          Tlc.clear();
          segmentPut(11);
          Tlc.update();
      }
    
      if((loopTime-fadeTime) > 10) {  // Message comms good, regular fade
        fadeTime = loopTime;
        fadeDown(0.90, MINFADE);
        Tlc.update();
      }
      break;
    case(THROT):  // ********* Throttle Angle readout ***************
      if(Canbus.message_rx(buff, &length )) {
        for(int i=0; i<12; i++) {
          float ang = buff[5];
          //float ang = buff[0]*50;
          int16_t offset = (int)(ang/2.6-8.0*i);
          if(offset<0) offset = 0;
          Tlc.set(LEDlut[LEDdeglut[i]], MAXBRIGHT*intensityLUT[offset] );
          }
        Tlc.update();
        }
      break;
    case(FIG8):
      if(loopTime-countdownTime > 10000) { // I have not recieved a message in a "while"
        for(int j = 9; j>=0; j--) {
          Tlc.clear();
          segmentPut(j);
          Tlc.update();
          for(int i = 0; i<200; i++){
            fadeDown(0.98, 0);
            Tlc.update();
            delay(2);
          }
        }
        countdownTime = millis();
        fig8Pos = 0;
      }
      if((loopTime-fadeTime) > 10) {
        fadeTime = loopTime;
        fadeDown(0.90, 0);
        Tlc.update();
      }
      if((loopTime-snakeTime) > 50) {
        snakeTime = loopTime;
        fig8Pos ++;
        fig8Pos %= 16;
        Tlc.set(LEDlut[Fig8[fig8Pos]], MAXBRIGHT);
        Tlc.update();
      }
      break;
    case(DISABLE):
    default:
      Tlc.clear();
      Tlc.update();
      delay(100);
      break;
    



//  if((loopTime-fadeTime) > 10) {
//    fadeTime = loopTime;
//    fadeDown(0.90, MINFADE);
//    Tlc.update();
//  }
  
//  if((loopTime-snakeTime) > 50) {
//    snakeTime = loopTime;
//    fig8Pos ++;
//    fig8Pos %= 16;
//    Tlc.set(LEDlut[Fig8[fig8Pos]], MAXBRIGHT);
//    Tlc.update();
//  }




  }
}

void fadeDown(float f, unsigned int minFade) {
  unsigned int temp = 0;
  for(int i = 0; i<16; i++) {
    temp = Tlc.get(i);
    if( temp > minFade) Tlc.set(i, (temp-minFade)*f+minFade);
  }
  return;
}
void segmentPut(int i) {
  switch (i) {
    case 0:
      Tlc.set(LEDlut[0], MAXFADE);
      Tlc.set(LEDlut[1], MAXFADE);
      Tlc.set(LEDlut[2], MAXFADE);
      Tlc.set(LEDlut[3], MAXFADE);
      Tlc.set(LEDlut[4], MAXFADE);
      Tlc.set(LEDlut[5], MAXFADE);
      Tlc.set(LEDlut[6], MAXFADE);
      Tlc.set(LEDlut[7], MAXFADE);
      Tlc.set(LEDlut[8], MAXFADE);
      Tlc.set(LEDlut[9], MAXFADE);
      Tlc.set(LEDlut[10], MAXFADE);
      Tlc.set(LEDlut[11], MAXFADE);
      break;
    case 1:
      Tlc.set(LEDlut[2], MAXFADE);
      Tlc.set(LEDlut[3], MAXFADE);
      Tlc.set(LEDlut[4], MAXFADE);
      Tlc.set(LEDlut[5], MAXFADE);
      break;
    case 2:
      Tlc.set(LEDlut[0], MAXFADE);
      Tlc.set(LEDlut[1], MAXFADE);
      Tlc.set(LEDlut[2], MAXFADE);
      Tlc.set(LEDlut[3], MAXFADE);
      Tlc.set(LEDlut[6], MAXFADE);
      Tlc.set(LEDlut[7], MAXFADE);
      Tlc.set(LEDlut[8], MAXFADE);
      Tlc.set(LEDlut[9], MAXFADE);
      Tlc.set(LEDlut[12], MAXFADE);
      Tlc.set(LEDlut[13], MAXFADE);
      break;
    case 3:
      Tlc.set(LEDlut[0], MAXFADE);
      Tlc.set(LEDlut[1], MAXFADE);
      Tlc.set(LEDlut[2], MAXFADE);
      Tlc.set(LEDlut[3], MAXFADE);
      Tlc.set(LEDlut[4], MAXFADE);
      Tlc.set(LEDlut[5], MAXFADE);
      Tlc.set(LEDlut[6], MAXFADE);
      Tlc.set(LEDlut[7], MAXFADE);
      Tlc.set(LEDlut[12], MAXFADE);
      Tlc.set(LEDlut[13], MAXFADE);
      break;
    case 4:
      Tlc.set(LEDlut[2], MAXFADE);
      Tlc.set(LEDlut[3], MAXFADE);
      Tlc.set(LEDlut[4], MAXFADE);
      Tlc.set(LEDlut[5], MAXFADE);
      Tlc.set(LEDlut[10], MAXFADE);
      Tlc.set(LEDlut[11], MAXFADE);
      Tlc.set(LEDlut[12], MAXFADE);
      Tlc.set(LEDlut[13], MAXFADE);
      break;
    case 5:
      Tlc.set(LEDlut[0], MAXFADE);
      Tlc.set(LEDlut[1], MAXFADE);
      Tlc.set(LEDlut[4], MAXFADE);
      Tlc.set(LEDlut[5], MAXFADE);
      Tlc.set(LEDlut[6], MAXFADE);
      Tlc.set(LEDlut[7], MAXFADE);
      Tlc.set(LEDlut[10], MAXFADE);
      Tlc.set(LEDlut[11], MAXFADE);
      Tlc.set(LEDlut[12], MAXFADE);
      Tlc.set(LEDlut[13], MAXFADE);
      break;
    case 6:
      Tlc.set(LEDlut[0], MAXFADE);
      Tlc.set(LEDlut[1], MAXFADE);
      Tlc.set(LEDlut[4], MAXFADE);
      Tlc.set(LEDlut[5], MAXFADE);
      Tlc.set(LEDlut[6], MAXFADE);
      Tlc.set(LEDlut[7], MAXFADE);
      Tlc.set(LEDlut[8], MAXFADE);
      Tlc.set(LEDlut[9], MAXFADE);
      Tlc.set(LEDlut[10], MAXFADE);
      Tlc.set(LEDlut[11], MAXFADE);
      Tlc.set(LEDlut[12], MAXFADE);
      Tlc.set(LEDlut[13], MAXFADE);
      break;
    case 7:
      Tlc.set(LEDlut[0], MAXFADE);
      Tlc.set(LEDlut[1], MAXFADE);
      Tlc.set(LEDlut[2], MAXFADE);
      Tlc.set(LEDlut[3], MAXFADE);
      Tlc.set(LEDlut[4], MAXFADE);
      Tlc.set(LEDlut[5], MAXFADE);
      break;
    case 8:
      Tlc.set(LEDlut[0], MAXFADE);
      Tlc.set(LEDlut[1], MAXFADE);
      Tlc.set(LEDlut[2], MAXFADE);
      Tlc.set(LEDlut[3], MAXFADE);
      Tlc.set(LEDlut[4], MAXFADE);
      Tlc.set(LEDlut[5], MAXFADE);
      Tlc.set(LEDlut[6], MAXFADE);
      Tlc.set(LEDlut[7], MAXFADE);
      Tlc.set(LEDlut[8], MAXFADE);
      Tlc.set(LEDlut[9], MAXFADE);
      Tlc.set(LEDlut[10], MAXFADE);
      Tlc.set(LEDlut[11], MAXFADE);
      Tlc.set(LEDlut[12], MAXFADE);
      Tlc.set(LEDlut[13], MAXFADE);
      break;
    case 9:
      Tlc.set(LEDlut[0], MAXFADE);
      Tlc.set(LEDlut[1], MAXFADE);
      Tlc.set(LEDlut[2], MAXFADE);
      Tlc.set(LEDlut[3], MAXFADE);
      Tlc.set(LEDlut[4], MAXFADE);
      Tlc.set(LEDlut[5], MAXFADE);
      Tlc.set(LEDlut[6], MAXFADE);
      Tlc.set(LEDlut[7], MAXFADE);
      Tlc.set(LEDlut[10], MAXFADE);
      Tlc.set(LEDlut[11], MAXFADE);
      Tlc.set(LEDlut[12], MAXFADE);
      Tlc.set(LEDlut[13], MAXFADE);
      break;
    case 10:  // Special character (Nutral)
      Tlc.set(LEDlut[12], MAXFADE);
      Tlc.set(LEDlut[13], MAXFADE);
      break;
    case 11:  // Special character (Error)
      Tlc.set(LEDlut[2], MINFADE);
      Tlc.set(LEDlut[5], MINFADE);
      Tlc.set(LEDlut[8], MINFADE);
      Tlc.set(LEDlut[11], MINFADE);
      break;
      
  }
  return;
}


