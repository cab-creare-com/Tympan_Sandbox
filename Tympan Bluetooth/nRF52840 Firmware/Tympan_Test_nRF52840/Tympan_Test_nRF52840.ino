/*
*   Tympan_Test_nRF52840
*
*   Created: Chip Audette, OpenAudio, April 2024
*   Purpose: Test the nRF52840 on the Tympan RevF.  Includes App interaction.
*
*   TympanRemote App: https://play.google.com/store/apps/details?id=com.creare.tympanRemote
*      
*  See the comments at the top of SerialManager.h and State.h for more description.
*
*   MIT License.  use at your own risk.
*/

//here are the libraries that we need
#include <Tympan_Library.h>  //include the Tympan Library
#include "BLE_nRF52.h"  //local version of the BLE library, which is specific to the nRF52840
#include "SerialManager.h"
#include "State.h"

//set the sample rate and block size
const float sample_rate_Hz = 44100.0f ; //24000 or 44100 (or 44117, or other frequencies in the table in AudioOutputI2S_F32)
const int audio_block_samples = 32;     //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);

//create audio library objects for handling the audio
Tympan                    myTympan(TympanRev::F,audio_settings);          //only TympanRev::F has the nRF52480
AudioInputI2S_F32         i2s_in(audio_settings);                         //Digital audio in *from* the Teensy Audio Board ADC.
AudioEffectGain_F32       gain1(audio_settings), gain2(audio_settings);   //Applies digital gain to audio data.  Left and right.
AudioOutputI2S_F32        i2s_out(audio_settings);                        //Digital audio out *to* the Teensy Audio Board DAC.

//Make all of the audio connections
AudioConnection_F32       patchCord1(i2s_in, 0, gain1, 0);   //connect the Left input
AudioConnection_F32       patchCord2(i2s_in, 1, gain2, 0);   //connect the Right input
AudioConnection_F32       patchCord5(gain1, 0, i2s_out, 0);     //connect the Left gain to the Left output
AudioConnection_F32       patchCord6(gain2, 0, i2s_out, 1);     //connect the Right gain to the Right output

// Create classes for controlling the system
#include      "SerialManager.h"
#include      "State.h"                            
BLE_nRF52     ble(&myTympan);                          //create bluetooth BLE
SerialManager serialManager;                 //create the serial manager for real-time control (via USB or App)
State         myState; //keeping one's state is useful for the App's GUI


// define the setup() function, the function that is called once when the device is booting
void setup() {
  //begin the serial comms (for debugging)
  //myTympan.beginBothSerial(); delay(1000);
  //Serial.begin(115200);  //USB Serial.  This begin() isn't really needed on Teensy. 
  (myTympan.BT_Serial)->begin(115200); //UART to BLE module.  For the nRF52840, we're having the nRF assume 115200.
  //Serial1.begin(115200);
  delay(1000);
  Serial.println("Tympan_Test_nRF52840: Starting setup()...");

  //allocate the dynamic memory for audio processing blocks
  AudioMemory_F32(10,audio_settings); 

  //Enable the Tympan to start the audio flowing!
  myTympan.enable(); // activate AIC

  //Choose the desired input
  myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC);     // use the on board microphones
  //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC);    // use the microphone jack - defaults to mic bias 2.5V
  //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF

  //Set the desired volume levels
  myTympan.volume_dB(myState.output_gain_dB);          // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.
  myTympan.setInputGain_dB(myState.input_gain_dB);     // set input volume, 0-47.5dB in 0.5dB setps

  //setup BLE
  //while (Serial1.available()) Serial1.read();
  while ((myTympan.BT_Serial)->available()) (myTympan.BT_Serial)->read(); //clear the incoming Serial1 (BT) buffer
  //ble.setupBLE(myTympan.getBTFirmwareRev());  //this uses the default firmware assumption. You can override!

  Serial.println("Setup complete.");
  serialManager.printHelp();
} //end setup()


// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {

  //look for in-coming serial messages (via USB or via Bluetooth)
  if (Serial.available()) serialManager.respondToByte((char)Serial.read());   //USB Serial

  //respond to BLE
  if (ble.available() > 0) {
    delay(2);
    int n = ble.available();
    char foo[128];
    int ind = 0;
    while (ble.available() && (ind < 128)) {
      char c = (char)ble.read();
      if ((c != (char)0x0A) && (c != (char)0x0D)) foo[ind++] = c;
    }
    if ((ind==1) && (foo[0] == ' ')) {
      //ignore
    } else {
      for (int i = 0; i < ind; i++) {
        char c = foo[i];
        Serial.print("Received from BLE: char = " + String(c) + ", which is HEX = " );Serial.println(c,HEX);
        //if ((c == 'j') || (c == 'J')) {
          serialManager.respondToByte(c); //for the Tympan simulation, service any messages received form the BLE module
        //}
      }
    }
  }

  //periodically print the CPU and Memory Usage
  if (myState.printCPUtoGUI) {
    myTympan.printCPUandMemory(millis(),3000); //print every 3000 msec
    serviceUpdateCPUtoGUI(millis(),3000);      //print every 3000 msec
  }

} //end loop();


// ///////////////// Servicing routines


//Test to see if enough time has passed to send up updated CPU value to the App
void serviceUpdateCPUtoGUI(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
  static unsigned long lastUpdate_millis = 0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
    //send the latest value to the GUI!
    serialManager.updateCpuDisplayUsage();
    
    lastUpdate_millis = curTime_millis;
  } // end if
} //end serviceUpdateCPUtoGUI();



// ///////////////// functions used to respond to the commands

//change the gain from the App
void changeGain(float change_in_gain_dB) {
  myState.digital_gain_dB = myState.digital_gain_dB + change_in_gain_dB;
  gain1.setGain_dB(myState.digital_gain_dB);  //set the gain of the Left-channel gain processor
  gain2.setGain_dB(myState.digital_gain_dB);  //set the gain of the Right-channel gain processor
}

//Print gain levels 
void printGainLevels(void) {
  Serial.print("Analog Input Gain (dB) = "); 
  Serial.println(myState.input_gain_dB); //print text to Serial port for debugging
  Serial.print("Digital Gain (dB) = "); 
  Serial.println(myState.digital_gain_dB); //print text to Serial port for debugging
}

void printBleName(void) {
  String name;
  int ret_val = ble.getBleName(name);
  Serial.println("printBleName: ret_val = " + String(ret_val) + ", name = " + name);
}

void setBleName(const String &name) {
  int ret_val = ble.setBleName(name);
  Serial.println("setBleName: ret_val = " + String(ret_val) + " for name = " + name);
}

