
#ifndef _SerialManager_h
#define _SerialManager_h

#include <Tympan_Library.h>
#include "State.h"

//classes from the main sketch that might be used here
extern Tympan myTympan;                  //created in the main *.ino file
extern State myState;                    //created in the main *.ino file
extern AudioSettings_F32 audio_settings; //created in the main *.ino file  
extern BLE_nRF52 ble;


//functions in the main sketch that I want to call from here
extern void changeGain(float);
extern void printGainLevels(void);
extern void incrementHighpassFilters(float);
extern void printHighpassCutoff(void);

//
// The purpose of this class is to be a central place to handle all of the interactions
// to and from the SerialMonitor or TympanRemote App.  Therefore, this class does things like:
//
//    * Define what buttons and whatnot are in the TympanRemote App GUI
//    * Define what commands that your system will respond to, whether from the SerialMonitor or from the GUI
//    * Send updates to the GUI based on the changing state of the system
//
// You could achieve all of these goals without creating a dedicated class.  But, it 
// is good practice to try to encapsulate some of these functions so that, when the
// rest of your code calls functions related to the serial communciation (USB or App),
// you have a better idea of where to look for the code and what other code it relates to.
//

void setButtonState(String btnId, bool newState) {
  String msg = String("STATE=BTN:" + btnId + ":1");
  Serial.println("serialManager: setButtonState: sending = " + msg);
  ble.sendMessage(msg);
}
void setButtonText(String btnId, String text) {
  String msg = String("TEXT=BTN:" + btnId + ":" + text);
  Serial.println("serialManager: setButtonText: sending = " + msg);
  ble.sendMessage(msg);
}


//now, define the Serial Manager class
class SerialManager  {  // see Tympan_Library for SerialManagerBase for more functions!
  public:
    SerialManager(void)  {};
      
    void printHelp(void);
    void createTympanRemoteLayout(void); 
    void printTympanRemoteLayout(void); 
    bool respondToByte(char c);
    bool processCharacter(char c);  //this is called automatically by SerialManagerBase.respondToByte(char c)

    //method for updating the GUI on the App
    void setFullGUIState(bool activeButtonsOnly = false);
    void updateGainDisplay(void);
    void updateFilterDisplay(void);
    void updateCpuDisplayOnOff(void);
    void updateCpuDisplayUsage(void);

    //factors by which to raise or lower the parameters when receiving commands from TympanRemote App
    float gainIncrement_dB = 3.0f;            //raise or lower by x dB
    float freqIncrement = powf(2.0,1.0/3.0);  //raise or lower by 1/3rd octave
  private:

    TympanRemoteFormatter myGUI;  //Creates the GUI-writing class for interacting with TympanRemote App
   
};

void SerialManager::printHelp(void) {  
  Serial.println("SerialManager Help: Available Commands:");
  Serial.println(" h: Print this help");
  Serial.println(" k/K: AUDIO: Incr/Decrease Digital Gain");
  Serial.println(" t/T: AUDIO: Incr/Decrease Cutoff of Highpass Filter");
  Serial.println(" c/C: SYSTEM: Enable/Disable printing of CPU and Memory usage");
  Serial.println(" v:   BLE: Get firmware info from BLE module");
  Serial.println(" n:   BLE: Get BLE name of the BLE module");
  Serial.println(" N:   BLE: Set BLE name of the BLE module to TYMP-TYMP");
  Serial.println(" G:   BLE: Get BLE status of Connected");
  Serial.println(" g:   BLE: Get BLE status of Advertising");
  Serial.println(" f/F: BLE: Enable/Disable Advertising");
  Serial.println(" m:   BLE: Get BLE status of LedMode");
  Serial.println(" b/B: BLE: Set LedMode: b=1, B=0");
  Serial.println(" J:   Send JSON for the GUI for the Tympan Remote App");
  Serial.println();
}

bool SerialManager::respondToByte(char c) {
  //Serial.println("SerialManager: respondToByte " + String(c));
  return processCharacter(c);
}

//switch yard to determine the desired action
bool SerialManager::processCharacter(char c) {  //this is called by SerialManagerBase.respondToByte(char c)
  bool ret_val = true;

  switch (c) {
    case 'h':
      printHelp(); 
      break;
    case 'J': case 'j':           //The TympanRemote app sends a 'J' to the Tympan when it connects
      printTympanRemoteLayout();  //in resonse, the Tympan sends the definition of the GUI that we'd like
      break;
    case 'v':
      {
        String version;
        ble.version(version);
        Serial.println("serialManager: BLE module firmware: " + version);
      }
      break;
    case 'n':
      {
        String name = String("");
        int err_code = ble.getBleName(name);
        Serial.println("serialManager: retrieving BLE module name.  name = " + name);
      }
      break;
    case 'N':
      {
        String name = String("TympTymp");
        Serial.println("serialManager: setting BLE module name: " + name);
        int err_code = ble.setBleName(name);
      }
      break;
    case 'g':
      Serial.println("serialManager: BLE: isAdvertising = " + String(ble.isAdvertising()));
      break;
    case 'f':
      Serial.println("serialManager: BLE: enable Advertising...");
      ble.enableAdvertising(true);
      break;
    case 'F':
      Serial.println("serialManager: BLE: disable Advertising...");
      ble.enableAdvertising(false);
      break;
    case 'G':
      Serial.println("serialManager: BLE: isConnected = " + String(ble.isConnected()));
      break;
    case 'm':
      Serial.println("serialManager: BLE: getLedMode = " + String(ble.getLedMode()));
      break;
    case 'b':
      Serial.println("serialManager: BLE: setLedMode to 1...");
      ble.setLedMode(1);
      break;
    case 'B':
      Serial.println("serialManager: BLE: setLedMode to 0...");
      ble.setLedMode(0);
      break;     
    case 'k':
      changeGain(gainIncrement_dB);   //raise
      printGainLevels();      //print to USB Serial (ie, to the SerialMonitor)
      updateGainDisplay();    //update the App
      break;
    case 'K':
      changeGain(-gainIncrement_dB);  //lower
      printGainLevels();     //print to USB Serial (ie, to the SerialMonitor)
      updateGainDisplay();   //update the App
      break;
    case 't':
      incrementHighpassFilters(freqIncrement);     //raise
      printHighpassCutoff();  //print to USB Serial (ie, to the SerialMonitor)
      updateFilterDisplay();  //update the App
      break;
    case 'T':
      incrementHighpassFilters(1.0/freqIncrement); //lower
      printHighpassCutoff();  //print to USB Serial (ie, to the SerialMonitor)
      updateFilterDisplay();  //update the App
      break;
    case 'c':
      Serial.println("Starting CPU reporting...");
      myState.printCPUtoGUI = true;
      updateCpuDisplayOnOff();
      break;
    case 'C':
      Serial.println("Stopping CPU reporting...");
      myState.printCPUtoGUI = false;
      updateCpuDisplayOnOff();
      break;   
    default:
      ret_val = false;
  }
  return ret_val;
}



// //////////////////////////////////  Methods for defining and transmitting the GUI to the App

//define the GUI for the App
void SerialManager::createTympanRemoteLayout(void) {
  
  // Create some temporary variables
  TR_Page *page_h;  //dummy handle for a page
  TR_Card *card_h;  //dummy handle for a card

  //Add first page to GUI  (the indentation doesn't matter; it is only to help us see it better)
  page_h = myGUI.addPage("Treble Boost Demo");
      
      //Add a card under the first page
      card_h = page_h->addCard("Highpass Gain (dB)");
          //Add a "-" digital gain button with the Label("-"); Command("K"); Internal ID ("minusButton"); and width (4)
          card_h->addButton("-","K","",           4);  //displayed string, command, button ID, button width (out of 12)

          //Add an indicator that's a button with no command:  Label (value of the digital gain); Command (""); Internal ID ("gain indicator"); width (4).
          card_h->addButton("","","gainIndicator",4);  //displayed string (blank for now), command (blank), button ID, button width (out of 12)
  
          //Add a "+" digital gain button with the Label("+"); Command("K"); Internal ID ("minusButton"); and width (4)
          card_h->addButton("+","k","",           4);   //displayed string, command, button ID, button width (out of 12)

      //Add another card to this page
      card_h = page_h->addCard("Highpass Cutoff (Hz)");
          card_h->addButton("-","T","",        4);  //displayed string, command, button ID, button width (out of 12)
          card_h->addButton("", "", "cutoffHz",4);  //displayed string (blank for now), command (blank), button ID, button width (out of 12)
          card_h->addButton("+","t","",        4);  //displayed string, command, button ID, button width (out of 12)


  //Add a second page to the GUI
  page_h = myGUI.addPage("Globals");

    //Add an example card that just displays a value...no interactive elements
    card_h = page_h->addCard(String("Analog Input Gain (dB)"));
      card_h->addButton("",      "", "inpGain",   12); //label, command, id, width (out of 12)...THIS ISFULL WIDTH!

    //Add an example card where one of the buttons will indicate "on" or "off"
//    card_h = page_h->addCard(String("CPU Usage (%)"));
//      card_h->addButton("Start", "c", "cpuStart", 4);  //label, command, id, width...we'll light this one up if we're showing CPU usage
//      card_h->addButton(""     , "",  "cpuValue", 4);  //label, command, id, width...this one will display the actual CPU value
//      card_h->addButton("Stop",  "C", "",         4);  //label, command, id, width...this one will turn off the CPU display
        
  //add some pre-defined pages to the GUI (pages that are built-into the App)
  myGUI.addPredefinedPage("serialMonitor");
  //myGUI.addPredefinedPage("serialPlotter");
}


// Print the layout for the Tympan Remote app, in a JSON-ish string
void SerialManager::printTympanRemoteLayout(void) {
    Serial.println("SerialManager: printTympanRemoteLayout: sending JSON...");
    if (myGUI.get_nPages() < 1) createTympanRemoteLayout();  //create the GUI, if it hasn't already been created
    String s = myGUI.asString();
    Serial.println(s);
    ble.sendMessage(s); //ble is held by SerialManagerBase
    setFullGUIState();
}

// //////////////////////////////////  Methods for updating the display on the GUI

void SerialManager::setFullGUIState(bool activeButtonsOnly) {
  updateGainDisplay();
  updateFilterDisplay();
  setButtonText("inpGain",String(myState.input_gain_dB,1));
  updateCpuDisplayOnOff();
}

void SerialManager::updateGainDisplay(void) {
  setButtonText("gainIndicator", String(myState.digital_gain_dB,1));
}

void SerialManager::updateFilterDisplay(void) {
  setButtonText("cutoffHz", String(myState.cutoff_Hz,0));
}

void SerialManager::updateCpuDisplayOnOff(void) {
  //setButtonState("cpuStart",myState.printCPUtoGUI);  //illuminate the button if we will be sending the CPU value
}

void SerialManager::updateCpuDisplayUsage(void) {
   setButtonText("cpuValue",String(audio_settings.processorUsage(),1)); //one decimal places
}

#endif
