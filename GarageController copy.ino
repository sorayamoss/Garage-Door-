/*
* Project GarageController
* Description:
* Author:
* Date:
*/

#include "GarageHardware.h"




//variables for delta timing
const String topic = "cse222Garage/thisGarage/garage";
const int deltaDelay = 100;
int lastButtonState;
unsigned long ocButton;
unsigned long upButton;
unsigned long downButton;
unsigned long faultButton;

int openClose=0;

int acTime=25000;

int theState;

//timer to check for on/off button, debounce timer
Timer togTimer(100, checkTog);
Timer autoCloseTimer(5000, acFunc, true);

// Timer closeTimer(100, checkClose);
// Timer openTimer(100, checkOpen);
// Timer faultTimer(100, checkFault);
// Timer flagTimer(100, resetVTogFlag);


//timer check flag
bool vTogFlag=true;

//general flags
bool canChangeFlag=true;


bool startAutoFlag=true;


//initialize variables to manipulate
bool vTog=0;
bool vUp=0;
bool vDown=0;
bool vFault=0;
unsigned long startFade=0;
unsigned long switchTogTime=0;
int percentFade=255;

bool autoEnabled=false;

//create states
enum State {
  gOpen,        // open garage
  gClosing,        // closing garage
  gClosed,        // closed garage
  gOpening,        // opening garage
  gStopC,        // stoped midway through closing
  gStopO,        // stoped midway through opening
  gFaultC,        //fault button pressed during closing
  gFaultO,        //fault button pressed during opening
};


State counterState = gOpen;//start with garage open

int publishState(String arg) {
 // Goal: Publish a valid JSON string containing the state of the light:
 //   Ex:   "{"powered":true, "r":255, "g":255, "b":255}"
 //   Note that each property has double quotes!

 // TODO: Adjust variables to match your light's state variables
 // (NOTE: Be careful building the string.  C++ String operations often behave in
 //        counterintuitive ways when working with string literals (like "hi"))

 // String data = "";
 // data += theState;
 String data = "{";
 data += "\"state\":";
 data += theState;
 data += ",";
 data += "\"autoEn\":";
 data += autoEnabled;
 data += ",";
 data += "\"autoTi\":";
 data += acTime;
 data += "}";
Serial.println(counterState);

 Serial.println("Publishing:");
 Serial.println(data);

 Particle.publish(topic, data, 60, PRIVATE);
 return 0;
}

//next state function
State nextState(State state) {
  unsigned long now=millis();//get current run time
  switch (state) {
    //garage is open state
    case gOpen:
    //Serial.println("gOpen");
    // Serial.print("autoEnabled:");
    // Serial.println(autoEnabled);
    // Serial.print("startAutoFlag:");
    // Serial.println(startAutoFlag);
if(autoEnabled&&startAutoFlag){
  autoCloseTimer.start();
  startAutoFlag=false;
}

    //openClose=1;
    stopMotor();
    if(now-startFade<5000){ //fade the light
      percentFade=(int)(255.0-(255.0*((now-startFade)/5000.0)));
      setLightPWM(percentFade);
    }
    if(vTog&&canChangeFlag){//if on/off button pushed
      state=gClosing;
      theState = 1;
      canChangeFlag=false;
      vTogFlag=true;
      togTimer.stop();
      publishState("");
    }
    break;


    //garage is closing state
    case gClosing:
    autoCloseTimer.stop();
    startMotorClosing();
    setLight(true);
    if(vDown){ //if down sensor
      state=gClosed;
      theState = 2;
      startFade=millis();
      publishState("");
    }
    if(vTog&&canChangeFlag){ //if on off button pushed
      state=gStopC;
      theState = 4;
      startFade=millis();
      canChangeFlag=false;
      vTogFlag=true;
      togTimer.stop();
      publishState("");
    }
    if(vFault){ //if fault sensor
      startFade=millis();
      theState = 6;
      state=gFaultC;
      publishState("");
    }
    break;

    //garage is closed state
    case gClosed:
    //Serial.println("gClosed");
    openClose=1;
    stopMotor();
    if(now-startFade<5000){ //fade light
      percentFade=(int)(255.0-(255.0*((now-startFade)/5000.0)));
      setLightPWM(percentFade);
    }
    if(vTog&&canChangeFlag){ //if on/off button pushed
      state=gOpening;
      canChangeFlag=false;
      theState =3;
      vTogFlag=true;
      togTimer.stop();
      publishState("");
    }
    break;

    //garage is opening state
    case gOpening:
    //Serial.println("gOpening");
    startMotorOpening();
    setLight(true);
    if(vUp){ //if up sensor
      state=gOpen;
      startAutoFlag=true;
      startFade=millis();
      theState = 0;
      publishState("");
    }
    if(vTog&&canChangeFlag){ //if on/off button pushed
      state=gStopO;
      startFade=millis();
      canChangeFlag=false;
      vTogFlag=true;
      togTimer.stop();
      theState = 5;
      publishState("");
    }
    if(vFault){ //if fault sensor
      state=gFaultO;
      theState = 7;
      publishState("");
      startFade=millis();
    }
    break;

    //garage is stopped mid opening state
    case gStopO:
    //Serial.println("gStopO");
    stopMotor();
    if(now-startFade<5000){ //fade the light
      percentFade=(int)(255.0-(255.0*((now-startFade)/5000.0)));
      setLightPWM(percentFade);
    }
    if(vTog&&canChangeFlag){ //if on/off button pushed
      state=gClosing;
      canChangeFlag=false;
      vTogFlag=true;
      togTimer.stop();
      theState = 1;
      publishState("");
    }
    break;

    //garage is stopped mid closing state
    case gStopC:
    //Serial.println("gStopC");
    stopMotor();
    if(now-startFade<5000){ //fade the light
      percentFade=(int)(255.0-(255.0*((now-startFade)/5000.0)));
      Serial.println(percentFade);
      setLightPWM(percentFade);
    }
    if(vTog&&canChangeFlag){ //if on/off button pushed
      state=gOpening;
      theState = 3;
      publishState("");
      canChangeFlag=false;
      vTogFlag=true;
      togTimer.stop();
    }
    break;

    //garage fault mid closing state
    case gFaultC:
    //Serial.println("gFaultC");
    stopMotor();
    sendDebug("fault");
    if(now-startFade<5000){ //fade the light
      percentFade=(int)(255.0-(255.0*((now-startFade)/5000.0)));
      Serial.println(percentFade);
      setLightPWM(percentFade);
    }
    if(vTog&&canChangeFlag){ //if on/off button pushed
      state=gClosing;
      theState = 1;
      publishState("");
      canChangeFlag=false;
      vTogFlag=true;
      togTimer.stop();
    }
    break;

    //fault mid opening state
    case gFaultO:
    //Serial.println("gFaultC");
    stopMotor();
    sendDebug("fault");
    if(now-startFade<5000){ //fade the light
      percentFade=(int)(255.0-(255.0*((now-startFade)/5000.0)));
      Serial.println(percentFade);
      setLightPWM(percentFade);
    }
    if(vTog&&canChangeFlag){ //if on/off button pushed
      state=gOpening;
      theState = 3;
      publishState("");
      canChangeFlag=false;
      vTogFlag=true;
      togTimer.stop();
    }
    break;

  }//endswitch statemtn
  return state;
}






//particle function to set it to open
int TogDoor(String tog) {
  if(tog=="Hit"){
    vTog=1;
  }//turn lamp on
  return 1;
}

int autoClose(String ac){
if(ac.substring(0,1)=="t"){
  autoEnabled=true;
  Serial.println(autoEnabled);
}
else if(ac.substring(0,1)=="f"){
  autoEnabled=false;
}
  Serial.println(ac.substring(1,3).toInt());
  Serial.println(ac.substring(0,1));
  startAutoFlag=true;
  acTime= 1000*(ac.substring(1,3).toInt());
  autoCloseTimer.changePeriod(acTime);
  autoCloseTimer.stop();
  //autoCloseTimer.start();
  return 1;
}

void acFunc(){
  if(counterState==gOpen){
    counterState=gClosing;
    theState = 1;
    publishState("");
    //nextState(gClosing);
  }

}

int publish(String strang){
  publishState("");
  return 1;
}



void setup() {

  //set pins as inputs or outputs
  Serial.begin(9600);
  //Particle.variable("openBool", open);
   Particle.function("publishState", publishState);
   Particle.function("TogDoor", TogDoor);
   Particle.function("autoClose", autoClose);
   Particle.function("publish", publish);
   setupHardware();
}




void loop() {
  //Serial.print(vTogFlag);

  //set up debounce

  vUp=isDoorFullyOpen();
  vDown=isDoorFullyClosed();
  vFault=isFaultActive();

  //if on/off button pushed
  if(isButtonPressed()&&vTogFlag){

    vTogFlag=false;

    togTimer.reset();
    togTimer.start();
  }
  // if(isDoorFullyClosed()){
  //   closeTimer.reset();
  //   closeTimer.start();
  // }
  // if(isDoorFullyOpen()){
  //   openTimer.reset();
  //   openTimer.start();
  // }
  // if(isFaultActive()){
  //   faultTimer.reset();
  //   faultTimer.start();
  // }

  //update state
  counterState= nextState(counterState);


  //reseting flags
  if(isButtonPressed()==0){
    canChangeFlag=true;
    vTog=0;
    // flagTimer.reset();
    // flagTimer.start();
  }

  //reset our flag if no 1's reveived
  if(switchTogTime<millis()){
    if(isButtonPressed()==0){
      vTogFlag=true;
    }
    switchTogTime+=100;
  }


  //Serial.println(vTog);
}//end loop




// void checkClose(){
//
//   if(!isDoorFullyClosed()){
//     vDown=1;
//
//   }
//   else{
//     vDown=0;
//   }
//   closeTimer.stop();
// }//end check
//
//
// void checkOpen(){
//   if(!isDoorFullyOpen()==1){
//     vUp=1;
//   }
//   else{
//     vUp=0;
//   }
//   openTimer.stop();
// }//end check
//
//
// void checkFault(){
//   if(!isFaultActive()==1){
//     vFault=1;
//   }
//   else{
//     vFault=0;
//   }
//   faultTimer.stop();
// }//end check

//on off button debounce method
void checkTog(){
  if(isButtonPressed()==1){
    vTog=1;
  }
  else{
    vTog=0;
    vTogFlag=true;
    //reset to 0 so now can change state again
  }
  togTimer.stop();
}//end check

// void resetVTogFlag(){
//   if(!isButtonPressed()){
//     vTogFlag=true;
//     flagTimer.stop();
//   }
// }
