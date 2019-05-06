
/*
   Chip Audette, OpenAudio, Apr 2019

   MIT License.  Use at your own risk.
*/

#ifndef _AudioSDWriter_h
#define _AudioSDWriter_h

#include "SDWriter.h"
#include "AudioSettings_F32.h"
#include "AudioStream_F32.h"

//variables to control printing of warnings and timings and whatnot
#define PRINT_FULL_SD_TIMING 0    //set to 1 to print timing information of *every* write operation.  Great for logging to file.  Bad for real-time human reading.

//AudioSDWriter: A class to write data from audio blocks as part of the 
//   Teensy/Tympan audio processing paradigm.  The AudioSDWriter class is 
//   just a virtual Base class.  Use AudioSDWriter_F32 further down.
class AudioSDWriter {
  public:
    AudioSDWriter(void) {};
    enum class STATE { UNPREPARED = -1, STOPPED, RECORDING };
    STATE getState(void) {
      return current_SD_state;
    };
    enum class WriteDataType { INT16, FLOAT32 };
    virtual int setNumWriteChannels(int n) {
      return numWriteChannels = max(1, min(n, 2));  //can be 1 or 2
    }
    virtual int getNumWriteChannels(void) {
      return numWriteChannels;
    }

    //virtual void prepareSDforRecording(void) = 0;
    //virtual int startRecording(void) = 0;
    //virtual int startRecording(char *) = 0;
    //virtual void stopRecording(void) = 0;

  protected:
    STATE current_SD_state = STATE::UNPREPARED;
    WriteDataType writeDataType = WriteDataType::INT16;
    int recording_count = 0;
    int numWriteChannels = 2;
};

//AudioSDWriter_F32: A class to write data from audio blocks as part
//   of the Teensy/Tympan audio processing paradigm.  For this class, the
//   audio is given as float32 and written as int16
class AudioSDWriter_F32 : public AudioSDWriter, public AudioStream_F32 {
  //GUI: inputs:2, outputs:0 //this line used for automatic generation of GUI node
  public:
    AudioSDWriter_F32(void) :
      AudioSDWriter(),
      AudioStream_F32(2, inputQueueArray)
    { 
      setup();
    }
    AudioSDWriter_F32(const AudioSettings_F32 &settings) :
      AudioSDWriter(),
      AudioStream_F32(2, inputQueueArray)
    { 
      setup(); 
      setSampleRate_Hz(settings.sample_rate_Hz);
    }
    AudioSDWriter_F32(const AudioSettings_F32 &settings, Print* _serial_ptr) :
      AudioSDWriter(),
      AudioStream_F32(2, inputQueueArray)
    { 
      setup(_serial_ptr);
      setSampleRate_Hz(settings.sample_rate_Hz);
    }
    AudioSDWriter_F32(const AudioSettings_F32 &settings, Print* _serial_ptr, const int _writeSizeBytes) :
      AudioSDWriter(),
      AudioStream_F32(2, inputQueueArray)
    { 
      setup(_serial_ptr, _writeSizeBytes); 
      setSampleRate_Hz(settings.sample_rate_Hz); 
    }
    ~AudioSDWriter_F32(void) {
      stopRecording();
      //delete buffSDWriterF32;
      delete buffSDWriter;
    }

    void setup(void) {
      setWriteDataType(WriteDataType::INT16, &Serial, DEFAULT_SDWRITE_BYTES);
      //setupQueues();
    }
    void setup(Print *_serial_ptr) {
      setSerial(_serial_ptr);
      setWriteDataType(WriteDataType::INT16, _serial_ptr, DEFAULT_SDWRITE_BYTES);
      //setupQueues();
    }
    void setup(Print *_serial_ptr, const int _writeSizeBytes) {
      setSerial(_serial_ptr);
      setWriteDataType(WriteDataType::INT16, _serial_ptr, _writeSizeBytes);
      //setupQueues();
    }
    //void setupQueues(void) { queueL.end();  queueR.end(); } //disable the queues by defeault

    void setSerial(Print *_serial_ptr) {  serial_ptr = _serial_ptr;  }
    void setWriteDataType(WriteDataType type) {
      Print *serial_ptr = &Serial1;
      int write_nbytes = DEFAULT_SDWRITE_BYTES;

      //get info from previous objects
      //if (buffSDWriterF32) {
      //  serial_ptr = buffSDWriterF32->getSerial();
      //  write_nbytes = buffSDWriterF32->getWriteSizeBytes();
      //} else
      if (buffSDWriter) {
        serial_ptr = buffSDWriter->getSerial();
        write_nbytes = buffSDWriter->getWriteSizeBytes();
      }

      //make the full method call
      setWriteDataType(type, serial_ptr, write_nbytes);
    }
    void setWriteDataType(WriteDataType type, Print* serial_ptr, const int writeSizeBytes) {
      stopRecording();
      //switch (type) {
      //  case (WriteDataType::INT16):
          writeDataType = type;
          //if (buffSDWriterF32) delete buffSDWriterF32;
          if (!buffSDWriter) buffSDWriter = new BufferedSDWriter(serial_ptr, writeSizeBytes);
      //    break;
      //  case (WriteDataType::FLOAT32):
      //    writeDataType = type;
      //    if (buffSDWriterI16) delete buffSDWriterI16;
      //    if (!buffSDWriterF32) buffSDWriterF32 = new BufferedSDWriter_F32(serial_ptr, writeSizeBytes);
      //    break;
      //}
    }
    void setWriteSizeBytes(const int n) {  //512Bytes is most efficient for SD
      if (buffSDWriter) {
        buffSDWriter->setWriteSizeBytes(n);
      //} else if (buffSDWriterF32) {
      //  buffSDWriterF32->setWriteSizeBytes(n);
      }
    }
    int getWriteSizeBytes(void) {  //512Bytes is most efficient for SD
      if (buffSDWriter) {
        return buffSDWriter->getWriteSizeBytes();
      //} else if (buffSDWriterF32) {
      //  return buffSDWriterF32->getWriteSizeBytes();
      } else {
        return 0;
      }
    }
    virtual int setNumWriteChannels(int n) {
      if (buffSDWriter) {
        return buffSDWriter->setNChanWAV(n);
      //} else if (buffSDWriterF32) {
      //  return buffSDWriterF32->setNChanWAV(n);
      } else {
        return n;
      }
    }
    virtual float setSampleRate_Hz(float fs_Hz) {
      if (buffSDWriter) {
        return buffSDWriter->setSampleRateWAV(fs_Hz);
      //} else if (buffSDWriterF32) {
      //  return buffSDWriterF32->setSampleRateWAV(fs_Hz);
      } else {
        return fs_Hz;
      }
    }
    
    void prepareSDforRecording(void) {
      if (current_SD_state == STATE::UNPREPARED) {
        if (buffSDWriter) {
          buffSDWriter->init(); //part of SDWriter, which is the base for BufferedSDWriter_I16
          if (PRINT_FULL_SD_TIMING) buffSDWriter->enablePrintElapsedWriteTime(); //for debugging.  make sure time is less than (audio_block_samples/sample_rate_Hz * 1e6) = 2900 usec for 128 samples at 44.1 kHz
        //} else if (buffSDWriterF32) {
        //  buffSDWriterF32->init();
        //  if (PRINT_FULL_SD_TIMING) buffSDWriterF32->enablePrintElapsedWriteTime(); //for debugging.  make sure time is less than (audio_block_samples/sample_rate_Hz * 1e6) = 2900 usec for 128 samples at 44.1 kHz
        }
        current_SD_state = STATE::STOPPED;
      }
    }

    int startRecording(void) {
      int return_val = 0;

      //check to see if the SD has been initialized
      if (current_SD_state == STATE::UNPREPARED) prepareSDforRecording();

      //check to see if SD is ready
      if (current_SD_state == STATE::STOPPED) {
        recording_count++;
        if (recording_count < 1000) {
          //make file name
          char fname[] = "AUDIOxxx.WAV";
          int hundreds = recording_count / 100;
          fname[5] = hundreds + '0';  //stupid way to convert the number to a character
          int tens = (recording_count - (hundreds*100)) / 10;  //truncates
          fname[6] = tens + '0';  //stupid way to convert the number to a character
          int ones = recording_count - (tens * 10) - (hundreds*100);
          fname[7] = ones + '0';  //stupid way to convert the number to a character

          //open the file
          return_val = startRecording(fname);
        } else {
          if (serial_ptr) {
            serial_ptr->println("AudioSDWriter: start: Cannot do more than 999 files.");
          }
        }
      } else {
        if (serial_ptr) {
          serial_ptr->println("AudioSDWriter: start: not in correct state to start.");
        }
        return_val = -1;
      }
      return return_val;
    }
    
    int startRecording(char* fname) {
      int return_val = 0;
      if (current_SD_state == STATE::STOPPED) {
        //try to open the file on the SD card
        if (openAsWAV(fname)) { //returns TRUE if the file opened successfully
          if (serial_ptr) {
            serial_ptr->print("AudioSDWriter: Opened ");
            serial_ptr->println(fname);
          }
          
          //start the queues.  Then, in the serviceSD, the fact that the queues
          //are getting full will begin the writing
          //queueL.clear(); queueR.clear(); //clear out any previous data
          //queueL.begin(); queueR.begin();
          buffSDWriter->resetBuffer();
          current_SD_state = STATE::RECORDING;
          setStartTimeMillis();
          
        } else {
          if (serial_ptr) {
            serial_ptr->print("AudioSDWriter: start: Failed to open ");
            serial_ptr->println(fname);
          }
          return_val = -1;
        }
      } else {
        if (serial_ptr) {
          serial_ptr->println("AudioSDWriter: start: not in correct state to start.");
        }
        return_val = -1;
      }
      return return_val;
    }

    void stopRecording(void) {
      if (current_SD_state == STATE::RECORDING) {
        //if (serial_ptr) {
        //  serial_ptr->println("stopRecording: Closing SD File...");
        //}

        //close the file
        close();
        current_SD_state = STATE::STOPPED;

        //stop and clear the queues so that they stop accumulating data
        //queueL.end();  queueR.end();   //stop accumulating new blocks of audio
        //queueL.clear();queueR.clear(); //release any audio blocks that were accumulated.

        //clear the buffer
        if (buffSDWriter) {
          buffSDWriter->resetBuffer();
        //} else if (buffSDWriterF32) {
        //  buffSDWriterF32->resetBuffer();
        }
      }
    }

    //update is called by the Audio processing ISR.  This update function should
    //only service the recording queues so as to buffer the audio data.
    //The acutal SD writing should occur in the loop() as invoked by a service routine
    void update(void) {
      audio_block_f32_t *audio_blocks[4];

      //get the audio
      for (int Ichan=0; Ichan < numWriteChannels; Ichan++) audio_blocks[Ichan] = receiveReadOnly_f32(Ichan);

      //copy the audio to the bug write buffer
      if (current_SD_state == STATE::RECORDING) {
        if (buffSDWriter) {
          buffSDWriter->copyToWriteBuffer(audio_blocks,numWriteChannels);
        //} else if (buffSDWriterF32) {
        //  buffSDWriterF32->copyToWriteBuffer(audio_blocks,numWriteChannels);
        }
      }

      //release the audio blocks
      for (int Ichan=0; Ichan < numWriteChannels; Ichan++) {
        if (audio_blocks[Ichan]) AudioStream_F32::release(audio_blocks[Ichan]);
      }  
         
    }

    bool isFileOpen(void) {
      if (buffSDWriter) {
        return buffSDWriter->isFileOpen();
      //} else if (buffSDWriterF32) {
      //  return buffSDWriterF32->isFileOpen();
      } else {
        return false;
      }
    }

    //this is what pulls data from the queues and sends to SD for writing.
    //should be invoked from loop(), not from an ISR
    int serviceSD(void) {
      if (buffSDWriter) {
        return buffSDWriter->writeBufferedData();
      //} else if (buffSDWriterF32) {
      //  return buffSDWriterF32->writeBufferedData();
      } else {
        return false;
      }
      
      //if (numWriteChannels == 1) {
      //  return serviceSD_oneChan();
      //} else if (numWriteChannels == 2) {
      //  return serviceSD_twoChan();
      //} else {
      //  return 0;
      //}
    }
    
//    int serviceSD_oneChan(void) {
//      int return_val = 0;
//      //is the SD subsystem ready to write?
//      if (isFileOpen()) {
//        //if audio data is ready, write it to SD
//        if (queueL.available()) {
//          audio_block_f32_t *foo = queueL.getAudioBlock();
//          writeOneChannel(foo->data, foo->length);
//          queueL.freeBuffer();
//          return_val = 1;
//
//          //clear out right channel even if we're not processing it
//          if (queueR.available()) {
//            foo = queueR.getAudioBlock(); //do I actually need to get it?  Or can I just freeBuffer() it?
//            queueR.freeBuffer();
//          }
//        }
//      }
//      return return_val;
//    }
//    int serviceSD_twoChan(void) {
//      int return_val = 0;
//      //is the SD subsystem ready to write?
//      if (isFileOpen()) {
//        if (queueL.available() && queueR.available()) {
//          audio_block_f32_t *left = queueL.getAudioBlock(), *right = queueR.getAudioBlock();
//          interleaveAndWrite(
//            left->data,    //float32 array for left audio channel
//            right->data,   //float32 array for right audio channel
//            left->length); //number of samples in each channel
//          queueL.freeBuffer(); queueR.freeBuffer();  //free up these blocks now that they are written
//          return_val = 1;
//        }
//      }
//      return return_val;
//    }

    unsigned long getNBlocksWritten(void) {
      if (buffSDWriter) {
        return buffSDWriter->getNBlocksWritten();
      //} else if (buffSDWriterF32) {
      //  return buffSDWriterF32->getNBlocksWritten();
      } else {
        return 0;
      }
    }
    
    void resetNBlocksWritten(void) {
      if (buffSDWriter) {
        buffSDWriter->resetNBlocksWritten();
      //} else if (buffSDWriterF32) {
      //  buffSDWriterF32->resetNBlocksWritten();
      }
    }
//
//
//    bool getQueueOverrun(void) {
//      return (queueL.getOverrun() || queueR.getOverrun());
//    }
//    void clearQueueOverrun(void) {
//      queueL.clearOverrun();
//      queueR.clearOverrun();
//    }
  unsigned long getStartTimeMillis(void) { return t_start_millis; };
  unsigned long setStartTimeMillis(void) { return t_start_millis = millis(); };

  protected:
    audio_block_f32_t *inputQueueArray[2]; //two input channels
    //AudioRecordQueue_F32 queueL, queueR;
    BufferedSDWriter *buffSDWriter = 0;
    //BufferedSDWriter_F32 *buffSDWriterF32 = 0;
    Print *serial_ptr = &Serial;
    unsigned long t_start_millis = 0;

    bool openAsWAV(char *fname) {
      if (buffSDWriter) {
        return buffSDWriter->openAsWAV(fname);
      //} else if (buffSDWriterF32) {
      //  Serial.println("AudioSDWriter: cannot open F32 as WAV...opening as RAW...");
      //  if (strlen(fname) > 3) {//change the filename exention to RAW
      //    fname[strlen(fname)-3] = 'R';fname[strlen(fname)-2] = 'A';fname[strlen(fname)-1] = 'W';
      //  }
      //  return buffSDWriterF32->open(fname);
      } else {
        return false;
      }
    }
    bool open(char *fname) {
      if (buffSDWriter) {
        //Serial.println("AudioSDWriter: opening BuffSDWritterI16...");
        return buffSDWriter->open(fname);
      //} else if (buffSDWriterF32) {
      //  //Serial.println("AudioSDWriter: opening BuffSDWritterF32...");
      //  return buffSDWriterF32->open(fname);
      } else {
        return false;
      }
    }
    
//    int interleaveAndWrite(float32_t *left, float32_t *right, const int &length) {
//      if (buffSDWriterI16) {
//        //Serial.println("AudioSDWriter: interleave via I16...");
//        return buffSDWriterI16->interleaveAndWrite(left, right, length);
//      } else if (buffSDWriterF32) {
//        //Serial.println("AudioSDWriter: interleave via F32...");
//        return buffSDWriterF32->interleaveAndWrite(left, right, length);
//      } else {
//        return 0;
//      }
//    }
//    
//    int writeOneChannel(float32_t *left, const int &length) {
//      if (buffSDWriterI16) {
//        return buffSDWriterI16->writeOneChannel(left, length);
//      } else if (buffSDWriterF32) {
//        return buffSDWriterF32->writeOneChannel(left, length);
//      } else {
//        return 0;
//      }
//    }

    int close(void) {
      if (buffSDWriter) {
        return buffSDWriter->close();
      //} else if (buffSDWriterF32) {
      //  return buffSDWriterF32->close();
      } else {
        return 0;
      }
    }
};

#endif
