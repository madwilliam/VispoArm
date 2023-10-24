
#include <stdio.h>
#include <stdlib.h>
#include <Servo.h>

/// SAME AS PYTHON
int PEAK_HEIGHT_FLX = 60;
int PEAK_HEIGHT_EXT = 20;
int MID = 50;
int WINDOW = 100;
int POOR_PREDICTION_THRESHOLD = 1000000;
int flx_aligned[100];
int ext_aligned[100];
int DC_COMPONENT = 55;
float alpha = 0.2;

/// THIS FILE ONLY
int count = 0;
const int analogInPin = A0;
const int D7 = 7;
Servo serv;
int EXT = 1;
int FLX = 0;
int RST = 3;
int prevpred = 1;
double *MAX_SIG;

int FLEX_FILTER[100];
int EXT_FILTER[100];
int RST_FILTER[100];
#include <EEPROM.h>

// type used to capture each data point in a sample
// !maximum space in nano is believed to be 1024B!
// in that case, the hard max is 5B per data point
#define USE_TYPE short

// N - number of samples represented in window average
// LO - first sample in window average
// HI - last sample in window average
int BASE = 0;
int LENGTH = 100;

int ADDR_FN  = 0;
int ADDR_FLO = ADDR_FN + sizeof(USE_TYPE);
int ADDR_FHI = ADDR_FLO + (LENGTH - 1) * sizeof(USE_TYPE);

int ADDR_EN = ADDR_FHI + sizeof(USE_TYPE);
int ADDR_ELO = ADDR_EN + sizeof(USE_TYPE);
int ADDR_EHI = ADDR_ELO + (LENGTH - 1) * sizeof(USE_TYPE);

int LINEUP = 50;

int addr; 
USE_TYPE E_val;
USE_TYPE N;

int emg;

int dt_ms = 30;

bool start_fresh = false;
bool align = true;


int ema(int exp, int actual, float alpha){
  // smooths the signal based on alpha value
  // 0.1 is large smoothing, 0.5 is less smoothing, 1 is basically none
  // float value 0 to 1

  return (int) ( (alpha * (float)(exp)) + ((1 - alpha) * (float)(actual)) );
}

void convolve_avg(int* A, int* B, double *AVG_VAL){
  // convolve signals A and B and return the average of the convolution
  // aligned signal is A, filter is B
  double large_conv = 0;
  double large_avg = 0;
  *AVG_VAL = 0; // zero out avg
  for(int i = 0; i < WINDOW-1; i++)
  {
    large_conv = 0;
    for(int j = 0; j < WINDOW-1; j++){ 
      large_conv += ((double)(A[i-j]) * (double)(B[j])); // compute current convolution
    }
    // CHECK IF CURRENT CONV IS THE MAX VAL, MAYBE ALSO MAKE ONE FOR AVG
    *AVG_VAL += large_conv;
    *AVG_VAL /= 2;

  }
}

void convolve_max(int* A, int* B, double *MAX_VAL){
  // convolve signals A and B and return the maximum of the convolution
  // aligned signal is A, filter is B

  double large_conv = 0;
  double prev_conv = 0;
  double large_max = 0;
  *MAX_VAL = 0; // zero out max
  for(int i = 0; i < WINDOW-1; i++)
  {
    large_conv = 0;
    for(int j = 0; j < WINDOW-1; j++){ 
      large_conv += ((double)(A[i-j]) * (double)(B[j])); // compute current convolution
    }
    // CHECK IF CURRENT CONV IS THE MAX VAL, MAYBE ALSO MAKE ONE FOR AVG
    if ((large_conv > prev_conv) && (large_conv > large_max)){
        large_max = large_conv;
    }
    prev_conv = large_conv;
  }
  *MAX_VAL = large_max;
}

int find_peaks(int *data, int max_height){
  // iterate through data file, comparing each pt 
  // until max peak obtained larger than peak_height 

  int MAX_INDEX = 0;
  int PREV = data[0]; //handle both negative and pos peak starts
  for(int i = 0; i < WINDOW-1; i++){
    if ((data[i] >= PREV) && (data[i] >= max_height) && (data[i] >= data[MAX_INDEX])){
      MAX_INDEX = i;
    }
  }
  return MAX_INDEX;
}

void align_signal(int *data){
  // align the signal to the mid point based on the maximum peak,
  // FLEX and EXT are aligned at the SAME peak

  int max_height;
  int peak_index, gap, endpoint;

  int neg_data[WINDOW];
  
  max_height = PEAK_HEIGHT_FLX;

  peak_index = find_peaks(data, max_height); // find maximum peak

  if (peak_index){
    if (peak_index > MID){
        gap = peak_index - MID;

        for(int i = 0; i < (WINDOW-1 - gap); i++){
          flx_aligned[i] = data[gap]; // save starting at gap up to window end
          ext_aligned[i] = data[gap]; // save starting at gap up to window end
          gap++;
        }
        for(int i = gap; i < WINDOW-1; i++){
          flx_aligned[i] = DC_COMPONENT; // make the rest of the window DC component flat
          ext_aligned[i] = DC_COMPONENT; // make the rest of the window DC component flat
        }

    }
    else if (peak_index <= MID){
        gap = MID - peak_index;

        for(int i = 0; i < gap; i++){
          flx_aligned[i] = DC_COMPONENT; // make the beginning of the window DC component flat
          ext_aligned[i] = DC_COMPONENT; // make the beginning of the window DC component flat
        }
        endpoint = (WINDOW-1-gap);
        for(int i = 0; i < endpoint; i++){
          flx_aligned[gap] = data[i]; // make from gap to window end the data 
          ext_aligned[gap] = data[i]; // make from gap to window end the data 
          gap++;
        }
    }
    else{
      for(int i = 0; i < WINDOW-1; i++){
        flx_aligned[i] = data[i]; // IF NO PEAKS ARE FOUND AT THE SPECIFIED HEIGHT, RETURN ORIGINAL WAVE
        ext_aligned[i] = data[i]; // IF NO PEAKS ARE FOUND AT THE SPECIFIED HEIGHT, RETURN ORIGINAL WAVE
      }
    }
  }
}

int match_filter_prediction(int *d){
  // take a WINDOW size int array
  // align the signal to match the filter alignment
  // convolve the signal with the flexion and extension filter
  // return a number corresponding to FLEXION or EXTENSION based on convolution

  double MAX_FLX;
  double MAX_EXT;
  double MAX_RST;

  double PRED;

  align_signal(d); // ALIGN AS YOU CONVOLVE

  // CONVOLVE AND RETURN MAX?
  convolve_max(flx_aligned, FLEX_FILTER, &MAX_FLX);
  convolve_max(ext_aligned, EXT_FILTER, &MAX_EXT);

  Serial.println("END CONVOLUTION");

  // if (MAX_FLX < 300000 || MAX_EXT < 300000){
  //   Serial.println("Low Similarity, No prediction");
  //   return -1;
  // }
  PRED = (abs(MAX_FLX - MAX_EXT) > 4000 ? MAX_FLX:MAX_EXT);

  //flexion = 0, extension = 1, sustain = 2, rest = 3

  // RETURN THE PREDICTION BASED ON MAX VALUE
  if (PRED == MAX_FLX){
    return FLX;
  }
  else if (PRED == MAX_EXT){
    return EXT;
  }
  else if (PRED == MAX_RST){
    return RST;
  }

  return -1;
}

void setup(){
  // initialize Serial port and analog pins
  // fill the filters using data saved to EEPROM

  Serial.begin(9600);
  pinMode(A0, INPUT);
  pinMode(D7, OUTPUT);
  serv.attach(9);
  Serial.println("SETUP DONE");
  Serial.println("CALIBRATING...");
  mem_fill_filter();
  Serial.println("FILTERS LOADED");  
}

void loop(){
  // MAIN LOOP that samples analog pin waiting for a spike in the signal
  // indicating potential movement. 'gap' variable determines threshold for detection
  // loop then records WINDOW size signal, and sends it to a prediction fn
  // 
  // 

  int sensorWindow[WINDOW];
  int gap = 3;
  int val = 0;
  int prev_i;

  int prediction = 0;
  int i;

  int ema_actual; 

  while(1){
    // may have an issue with missing part of wave before drop, maybe save a buffer of 5 at all times?
    val = analogRead(analogInPin);
    ema_actual = DC_COMPONENT;
    ema_actual = ema(val, ema_actual, alpha); // do ema as read in

    // if the beginning of the loop
    if(count == 0){
      prev_i = ema_actual;
    }

    // if a movement is detected
    if(((gap + ema_actual) < prev_i) || ((-gap + ema_actual) > prev_i)){
      Serial.println("MOVEMENT DETECTED...");
      *MAX_SIG = 0;
      // read in a 100 smaple window
      for(i = 0; i < WINDOW; i++) {
        val = analogRead(analogInPin);
        ema_actual = DC_COMPONENT;
        ema_actual = ema(val, ema_actual, alpha);

        sensorWindow[i] = ema_actual; // save the window into sensorWindow

        if(*MAX_SIG < ema_actual){
          *MAX_SIG = ema_actual;
        }
        delay(dt_ms); // delay to allow time for ADC
      }

      // send new window to the prediction model 
      Serial.println("PREDICTING...");
      prediction = match_filter_prediction(sensorWindow);

      // process the prediction and move the servo
      if ((prediction == FLX) && (prevpred == EXT)){
        Serial.println("PREDICTION : EXTENSION");
        prevpred = FLX;
        driveServo(EXT);
      }
      else if ((prediction == EXT) && (prevpred == FLX)){
        Serial.println("PREDICTION : FLEXION");
        prevpred = EXT;
        driveServo(FLX);
      }
      else{
        Serial.println("PREDICTION : REST");
      }

      // prevpred = prediction;
    }

    // otherwise continue to poll for movement
    else if (count%100==0){
      Serial.println("POLLING...");
      count = 0;
    }
    prev_i = ema_actual;
    count++;
    delay(dt_ms);
  }
}

void mem_fill_filter()
// get filters saved to EEPROM from memory 
// and save into global variables
{ 
  int buffer = DC_COMPONENT - 23;
  int i = 0;
  for(addr = ADDR_FN; addr <= ADDR_FHI; addr += sizeof(USE_TYPE))
  {
    EEPROM.get(addr, E_val);
    FLEX_FILTER[i] = E_val+buffer;
    i++;
  }

  i = 0;
  for(addr = ADDR_EN; addr <= ADDR_EHI; addr += sizeof(USE_TYPE))
  {
    EEPROM.get(addr, E_val);
    EXT_FILTER[i] = E_val+buffer;
    i++;
  }

}

void driveServo(int pred){

  int SERV_PIN = 9;   // PWM pin
  int SERV_MIN = 10;   // minimum servo angle FLEXION
  int SERV_MAX = 180; // maximum sevo angle EXTENSION

  // unsigned long dt_ms = 250; // 16 Hz
  unsigned long servo_dt_ms = 1500; // 16 Hz

  int actual;

  if (pred == EXT) {
    // actual = min(actual + 10, SERV_MAX);
    Serial.println("MOVE EXT");
    actual = SERV_MAX;
    serv.write(actual);
    delay(servo_dt_ms);

  }
  else if (pred == FLX) {
    // actual = max(actual - 10, SERV_MIN);
    Serial.println("MOVE FLX");
    actual = SERV_MIN;
    serv.write(actual);
    delay(servo_dt_ms);
  }
  else if (pred == RST){
    Serial.println("DO NOT MOVE");
    delay(servo_dt_ms);
  }

  prevpred = pred;

}

========================= END MATCH FILTER LOOP CODE ==========================

 
A.	 EEPROM manipulation script for storing Flexion/Extension filters and viewing/erasing stored data.
#include <EEPROM.h>

// type used to capture each data point in a sample
// !maximum space in nano is believed to be 1024B!
// in that case, the hard max is 5B per data point
#define USE_TYPE short

// N - number of samples represented in window average
// LO - first sample in window average
// HI - last sample in window average
int BASE = 0;
int LENGTH = 100;

int ADDR_FN  = 0;
int ADDR_FLO = ADDR_FN + sizeof(USE_TYPE);
int ADDR_FHI = ADDR_FLO + (LENGTH - 1) * sizeof(USE_TYPE);

int ADDR_EN = ADDR_FHI + sizeof(USE_TYPE);
int ADDR_ELO = ADDR_EN + sizeof(USE_TYPE);
int ADDR_EHI = ADDR_ELO + (LENGTH - 1) * sizeof(USE_TYPE);

int LINEUP = 50;
///////////////////////////////////////////////////////

int addr; 
USE_TYPE E_val;
USE_TYPE N;

const uint8_t in_pin = A0;
int emg;

int dt_ms = 30;

bool start_fresh = false;
bool align = true;
float alpha = 0.4;
int actual = 0; // CHANGING THIS AFFECTS THE DC OFFSET
int ema_actual = 0;
int ema_val = 0;

void setup()
{
  Serial.begin(9600);
  pinMode(in_pin, INPUT);

  if(start_fresh)
  {
    mem_wipe();
  }
}

int ema(int exp, int actual, float alpha){
  return (int) (alpha * (float)exp + (1 - alpha) * (float)actual);
}

void mem_wipe()
{
  for (int i = 0; i < EEPROM.length() ; i++)
  {
    EEPROM.update(i, -1);
  }

  EEPROM.put(ADDR_FN, (unsigned USE_TYPE) 0);
  EEPROM.put(ADDR_EN, (unsigned USE_TYPE) 0);

  Serial.println("Done wipe.");
}

void mem_return()
{
  for(addr = ADDR_FN; addr <= ADDR_FHI; addr += sizeof(USE_TYPE))
  {
    EEPROM.get(addr, E_val);
    Serial.print(E_val);
    Serial.print(",");
  }
  Serial.println();

  for(addr = ADDR_EN; addr <= ADDR_EHI; addr += sizeof(USE_TYPE))
  {
    EEPROM.get(addr, E_val);
    Serial.print(E_val);
    Serial.print(",");
  }
  Serial.println();
}

USE_TYPE* get_window_ext()
{
  USE_TYPE arr[LENGTH + 1];
  int i = 0;

  for(addr = ADDR_EN; addr <= ADDR_EHI; addr += sizeof(USE_TYPE))
  {
    EEPROM.get(addr, arr[i]);
  }

  return arr;
}

USE_TYPE* get_window_flex()
{
  USE_TYPE arr[LENGTH + 1];
  int i = 0;

  for(addr = ADDR_FN; addr <= ADDR_FHI; addr += sizeof(USE_TYPE))
  {
    EEPROM.get(addr, arr[i]);
  }

  return arr;
}

void mem_catch_noalign(char command)
{
  // Loop <LENGTH> times with delay <dt_ms> (nom. 100, 30ms)
  // Loop iterates over the address space, stepping by sizeof datapoints.
  // It reads the old value from mem, expands it to maintain an accurate
  // average, and folds a new sensor value into the average window.

  switch (command)
  {
    case 'f':
      EEPROM.get(ADDR_FN, N);

      for (addr = ADDR_FLO; addr <= ADDR_FHI; addr += sizeof(USE_TYPE))
      {
        EEPROM.get(addr, E_val);
        E_val *= N;

        emg = analogRead(in_pin);
        E_val += (short) emg;
        E_val /= N + 1;

        EEPROM.put(addr, E_val);
        delay(dt_ms);
      }

      N += 1;
      EEPROM.put(ADDR_FN, N);        
      break;

    case 'e':
      EEPROM.get(ADDR_EN, N);

      for (addr = ADDR_ELO; addr <= ADDR_EHI; addr += sizeof(USE_TYPE))
      {
        EEPROM.get(addr, E_val);
        E_val *= N;

        emg = analogRead(in_pin);
        E_val += (short) emg;
        E_val /= N + 1;

        EEPROM.put(addr, E_val);
        delay(dt_ms);
      }

      N += 1;
      EEPROM.put(ADDR_EN, N);        
      break;

    default:
      return;
      break;
  }
}

void mem_catch(char command)
{
  int emg_arr[LENGTH];
  int lim_emg;
  int lim_ind;
  int i;
  int offset;

  switch (command)
  {
    case 'f':

      lim_ind = -1;
      lim_emg = -1000;

      // Take data points, hold index of global maximum
      for (i = 0; i < LENGTH; i++)
      {
        ema_val = analogRead(in_pin);
        emg_arr[i] = ema(ema_val,actual,alpha);

        if (emg_arr[i] > lim_emg)
        {
          lim_emg = emg_arr[i];
          lim_ind = i;
        }

        delay(dt_ms);
      }

      // How far we need to shift
      // - Positive val means pulse is too far ahead, and must be right shifted
      // - Negative val means pulse is lagging behind, and must be left shifted
      offset = LINEUP - lim_ind;

      i = 0;
      for (addr = ADDR_FLO; addr <= ADDR_FHI; addr += sizeof(USE_TYPE))
      {
        // If the wave is shifted right (offset positive)
        // Extrapolate beginning values
        if (i < offset)
        {
          emg = emg_arr[0];
        }
        // Else if the wave is shifted left (offset negative)
        // Extrapolate end values
        else if (i > (LENGTH - 1 + offset))
        {
          emg = emg_arr[LENGTH - 1];
        }
        // Iterate through the inner values, with actual pulse data
        else
        {
          emg = emg_arr[i - offset];
        }
        i++;

        // Retrieve N and pulse window, fold in the new values
        EEPROM.get(ADDR_FN, N);
        EEPROM.get(addr, E_val);
        E_val *= N;

        E_val += (short) emg;
        E_val /= N + 1;

        EEPROM.put(addr, E_val);
      }

      N += 1;
      EEPROM.put(ADDR_FN, N); 

      Serial.println("Done F.");
      break;

    case 'e':

      lim_ind = -1;
      lim_emg = 1000;

      // Take data, hold index of global minimum
      for (i = 0; i < LENGTH; i++)
      {
        ema_val = analogRead(in_pin);
        emg_arr[i] = ema(ema_val,actual,alpha);

        if (emg_arr[i] < lim_emg)
        {
          lim_emg = emg_arr[i];
          lim_ind = i;
        }

        delay(dt_ms);
      }

      // How far we need to shift
      // - Positive val means pulse is too far ahead, and must be right shifted
      // - Negative val means pulse is lagging behind, and must be left shifted
      offset = LINEUP - lim_ind;

      i = 0;
      for (addr = ADDR_ELO; addr <= ADDR_EHI; addr += sizeof(USE_TYPE))
      {
        // If the wave is shifted right (offset positive)
        // Extrapolate beginning values
        if (i < offset)
        {
          emg = emg_arr[0];
        }
        // Else if the wave is shifted left (offset negative)
        // Extrapolate end values
        else if (i > (LENGTH - 1 + offset))
        {
          emg = emg_arr[LENGTH - 1];
        }
        // Iterate through the inner values, with actual pulse data
        else
        {
          emg = emg_arr[i - offset];
        }
        i++;

        // Retrieve N and pulse window, fold in the new values
        EEPROM.get(ADDR_EN, N);
        EEPROM.get(addr, E_val);
        E_val *= N;

        E_val += (short) emg;
        E_val /= N + 1;

        EEPROM.put(addr, E_val);
      }

      N += 1;
      EEPROM.put(ADDR_EN, N); 

      Serial.println("Done E.");
      break;

    default:
      return;
      break;
  }
}

void loop()
{
  // delay(dt_ms);
  // ema_val = analogRead(in_pin);
  // ema_actual = ema(ema_val,actual,alpha);
  // Serial.println(ema_actual);
  if( Serial.available() > 0 )
  {
    char in_char = Serial.read();
    switch (in_char)
    {
      // User operated ROM wipe
      case '0':
        mem_wipe();
        break;

      // Returns N followed by window; of flex, then extend (in csv format)
      case 'm':
        mem_return();
        break;

      // Catch new window and fold into memory
      default:
        if (align)
        {
          mem_catch(in_char);
        }
        else
        {
          mem_catch_noalign(in_char);
        }

        break;
    }
  }
}

========================== END EEPROM CODE ==========================
 
B.	  
#include "MotionClassifier.h"
#include <Servo.h>

Servo serv;

Eloquent::ML::Port::SVM mclassifier;

// These constants won't change. They're used to give names to the pins used:
const int analogInPin = A0;  // Analog input pin that the potentiometer is attached to
const int analogOutPin = 7;  // Analog output pin that the LED is attached to

int outputValue = 0;  // value output to the PWM (analog out)
int D7 = 7;

int prediction = 0;
int prevpred = 1;
int EXT = 1;
int FLX = 0;
int RST = 3;


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(A0, INPUT);
  pinMode(D7, OUTPUT);
  serv.attach(9);
}

void loop() {
  // testServo();
  WindowedClassifier();
}

float ema(int exp, float actual, float alpha){
  return alpha * exp + (1 - alpha) * actual;
}

void driveServo(int pred){

  int SERV_PIN = 9;   // PWM pin
  int SERV_MIN = 20;   // minimum servo angle FLEXION
  int SERV_MAX = 160; // maximum sevo angle EXTENSION

  unsigned long dt_ms = 30; // 33 Hz

  int actual;

  if (pred == EXT) {
    // actual = min(actual + 10, SERV_MAX);
    Serial.println("MOVE EXT");
    actual = SERV_MAX;
    serv.write(actual);
    delay(dt_ms);

  }
  else if (pred == FLX) {
    // actual = max(actual - 10, SERV_MIN);
    Serial.println("MOVE FLX");
    actual = SERV_MIN;
    serv.write(actual);
    delay(dt_ms);
    // Serial.println("MOVE EXT");
    // actual = SERV_MAX;
    // serv.write(actual);
    // delay(dt_ms);
  }
  else if (pred == RST){
    Serial.println("DO NOT MOVE");
    delay(dt_ms);
  }

  prevpred = pred;

}


void windowedClassifier() {

  float sensorWindow;

 // GET WINDOW TO CLASSIFY
  //consecutive
  float gap = 50;
  int backtrack = 10;
  int val = 300;
  int windAVG = 50; // 40
  int wind = 100;
  int window;
  int prev_i = 0;

  float actual = 300;
  float ema_actual = 300;

  window = windAVG;

  while(1){
    val = analogRead(analogInPin);
    ema_actual = ema(val, actual, 0.5);
    actual = ema_actual;
    // ema_actual = val;
    Serial.println(ema_actual);
    if(((gap + ema_actual) < prev_i) || ((-gap + ema_actual) > prev_i)){ // IF ACTION MIGHT BE HAPPENING
      for(int i = 0; i < window; i++) {
        // sensorWindow[i] = analogRead(analogInPin); // save signal, not AVG
        val = analogRead(analogInPin);
        ema_actual = ema(val, actual, 0.5);
        // ema_actual = val;
        // Serial.println("COLLECTING");
        Serial.println(ema_actual);
        sensorWindow += ema_actual; // running sum, AVG
        actual = ema_actual;
        delay(30);
      }
        // INPUT AMPL INTO CLASSIFER TO GET DIRECTION
      float pred_arg = (sensorWindow / window);
      
      prediction = mclassifier.predict(&pred_arg); // get avg

      // OUTPUT DIGITAL SIGNAL TO SERVO/ LINEAR ACTUATION

      // Serial.println(pred_arg);
      if (prediction == FLX){
        if(pred_arg > 600.){
          driveServo(prediction);
        }
        else{
          continue;
        }
      }
      else if( prediction == EXT){
        driveServo(prediction);
      }
      sensorWindow = 0;
      continue;
    }
    else{
      prev_i = val;
      continue;
    }

  }

}
void testServo() {

  // float sensorWindow[100];
  int sensorWindow;

 // GET WINDOW TO CLASSIFY
  //consecutive
  float gap = 10;
  int backtrack = 10;
  int val = 0;
  int windAVG = 40;
  int wind = 100;
  int window;
  int prev_i = 0;

  int actual = 0;

  window = windAVG;

  int predictionArray[6] = {FLX,EXT,FLX,EXT};
  for(int i=0;i<10;i++){
    Serial.println(predictionArray[i]);
    driveServo(predictionArray[i]);

  }
  // driveServo(prediction);
}

========================== END SVC CLASSIFIER CODE ==========================
Also required for SVC, is the following header file generated using micomlgen which contains the trained model kernels and decision function:
#pragma once
#include <stdarg.h>
namespace Eloquent {
    namespace ML {
        namespace Port {
            class SVM {
                public:
                    /**
                    * Predict class for features vector
                    */
                    int predict(float *x) {
                        float kernels[124] = { 0 };
                        float decisions[3] = { 0 };
                        int votes[3] = { 0 };
                        kernels[0] = compute_kernel(x,   234.107781740693 );
                        kernels[1] = compute_kernel(x,   209.395206789073 );
                        kernels[2] = compute_kernel(x,   228.187365375106 );
                        kernels[3] = compute_kernel(x,   194.31433261167 );
                        kernels[4] = compute_kernel(x,   310.104073453035 );
                        kernels[5] = compute_kernel(x,   344.782616081447 );
                        kernels[6] = compute_kernel(x,   310.809518121942 );
                        kernels[7] = compute_kernel(x,   298.477851047818 );
                        kernels[8] = compute_kernel(x,   336.802522174925 );
                        kernels[9] = compute_kernel(x,   306.540183280313 );
                        kernels[10] = compute_kernel(x,   318.837550339576 );
                        kernels[11] = compute_kernel(x,   304.024319177053 );
                        kernels[12] = compute_kernel(x,   311.524791714699 );
                        kernels[13] = compute_kernel(x,   303.937723130519 );
                        kernels[14] = compute_kernel(x,   337.718838196133 );
                        kernels[15] = compute_kernel(x,   340.449203020712 );
                        kernels[16] = compute_kernel(x,   330.829559014473 );
                        kernels[17] = compute_kernel(x,   314.672900858812 );
                        kernels[18] = compute_kernel(x,   319.384972556802 );
                        kernels[19] = compute_kernel(x,   318.376372367674 );
                        kernels[20] = compute_kernel(x,   296.577402904131 );
                        kernels[21] = compute_kernel(x,   312.326752243236 );
                        kernels[22] = compute_kernel(x,   306.173234655297 );
                        kernels[23] = compute_kernel(x,   322.705031840236 );
                        kernels[24] = compute_kernel(x,   309.786584525767 );
                        kernels[25] = compute_kernel(x,   301.310442297938 );
                        kernels[26] = compute_kernel(x,   314.177196995269 );
                        kernels[27] = compute_kernel(x,   318.363971165982 );
                        kernels[28] = compute_kernel(x,   319.72979324924 );
                        kernels[29] = compute_kernel(x,   307.048701263221 );
                        kernels[30] = compute_kernel(x,   312.722265915959 );
                        kernels[31] = compute_kernel(x,   305.037469525351 );
                        kernels[32] = compute_kernel(x,   314.548050770745 );
                        kernels[33] = compute_kernel(x,   317.419006332481 );
                        kernels[34] = compute_kernel(x,   318.6265483269 );
                        kernels[35] = compute_kernel(x,   319.113374447597 );
                        kernels[36] = compute_kernel(x,   297.489206230694 );
                        kernels[37] = compute_kernel(x,   285.015903223307 );
                        kernels[38] = compute_kernel(x,   327.798446901411 );
                        kernels[39] = compute_kernel(x,   331.464307681916 );
                        kernels[40] = compute_kernel(x,   361.606735994796 );
                        kernels[41] = compute_kernel(x,   461.27141265983 );
                        kernels[42] = compute_kernel(x,   342.394032461751 );
                        kernels[43] = compute_kernel(x,   342.969128494926 );
                        kernels[44] = compute_kernel(x,   338.958138146159 );
                        kernels[45] = compute_kernel(x,   329.683141003202 );
                        kernels[46] = compute_kernel(x,   319.852516343134 );
                        kernels[47] = compute_kernel(x,   316.446907343993 );
                        kernels[48] = compute_kernel(x,   302.375899468141 );
                        kernels[49] = compute_kernel(x,   357.471375250842 );
                        kernels[50] = compute_kernel(x,   308.265144759 );
                        kernels[51] = compute_kernel(x,   309.740862193824 );
                        kernels[52] = compute_kernel(x,   299.723175535072 );
                        kernels[53] = compute_kernel(x,   322.306149167867 );
                        kernels[54] = compute_kernel(x,   321.538469778975 );
                        kernels[55] = compute_kernel(x,   304.852648683665 );
                        kernels[56] = compute_kernel(x,   310.942875975449 );
                        kernels[57] = compute_kernel(x,   343.539168567414 );
                        kernels[58] = compute_kernel(x,   340.920593249077 );
                        kernels[59] = compute_kernel(x,   329.625130208038 );
                        kernels[60] = compute_kernel(x,   311.783420215847 );
                        kernels[61] = compute_kernel(x,   323.069296231788 );
                        kernels[62] = compute_kernel(x,   312.158412033869 );
                        kernels[63] = compute_kernel(x,   327.102223270688 );
                        kernels[64] = compute_kernel(x,   311.931403969216 );
                        kernels[65] = compute_kernel(x,   316.894922194045 );
                        kernels[66] = compute_kernel(x,   311.465302090792 );
                        kernels[67] = compute_kernel(x,   315.603824535524 );
                        kernels[68] = compute_kernel(x,   334.240557156776 );
                        kernels[69] = compute_kernel(x,   332.220374409011 );
                        kernels[70] = compute_kernel(x,   328.366275792239 );
                        kernels[71] = compute_kernel(x,   323.622987096684 );
                        kernels[72] = compute_kernel(x,   322.321761691657 );
                        kernels[73] = compute_kernel(x,   326.668511444142 );
                        kernels[74] = compute_kernel(x,   326.707955512412 );
                        kernels[75] = compute_kernel(x,   324.003870618603 );
                        kernels[76] = compute_kernel(x,   321.236582652607 );
                        kernels[77] = compute_kernel(x,   322.999372397718 );
                        kernels[78] = compute_kernel(x,   320.180090070704 );
                        kernels[79] = compute_kernel(x,   339.077541341288 );
                        kernels[80] = compute_kernel(x,   336.270495361798 );
                        kernels[81] = compute_kernel(x,   436.431526793743 );
                        kernels[82] = compute_kernel(x,   317.243527444689 );
                        kernels[83] = compute_kernel(x,   481.136117251161 );
                        kernels[84] = compute_kernel(x,   315.70777045867 );
                        kernels[85] = compute_kernel(x,   301.479042968826 );
                        kernels[86] = compute_kernel(x,   317.48854208915 );
                        kernels[87] = compute_kernel(x,   316.810016444048 );
                        kernels[88] = compute_kernel(x,   319.240601001177 );
                        kernels[89] = compute_kernel(x,   319.840595020828 );
                        kernels[90] = compute_kernel(x,   316.364386481504 );
                        kernels[91] = compute_kernel(x,   324.400278689848 );
                        kernels[92] = compute_kernel(x,   322.825237632863 );
                        kernels[93] = compute_kernel(x,   323.959131705976 );
                        kernels[94] = compute_kernel(x,   329.92530873484 );
                        kernels[95] = compute_kernel(x,   333.650592136663 );
                        kernels[96] = compute_kernel(x,   323.329392107386 );
                        kernels[97] = compute_kernel(x,   326.592806140683 );
                        kernels[98] = compute_kernel(x,   324.184868560568 );
                        kernels[99] = compute_kernel(x,   323.492291406345 );
                        kernels[100] = compute_kernel(x,   334.628887212667 );
                        kernels[101] = compute_kernel(x,   325.445889353463 );
                        kernels[102] = compute_kernel(x,   323.325968381561 );
                        kernels[103] = compute_kernel(x,   322.777615778224 );
                        kernels[104] = compute_kernel(x,   318.044311194458 );
                        kernels[105] = compute_kernel(x,   324.640877696595 );
                        kernels[106] = compute_kernel(x,   317.992817904879 );
                        kernels[107] = compute_kernel(x,   324.548178304788 );
                        kernels[108] = compute_kernel(x,   314.373782221697 );
                        kernels[109] = compute_kernel(x,   372.170989203965 );
                        kernels[110] = compute_kernel(x,   311.485975985895 );
                        kernels[111] = compute_kernel(x,   337.480921319829 );
                        kernels[112] = compute_kernel(x,   344.395507807593 );
                        kernels[113] = compute_kernel(x,   344.947593716001 );
                        kernels[114] = compute_kernel(x,   309.19158269763 );
                        kernels[115] = compute_kernel(x,   320.079912383088 );
                        kernels[116] = compute_kernel(x,   324.206569622938 );
                        kernels[117] = compute_kernel(x,   319.478810950246 );
                        kernels[118] = compute_kernel(x,   341.621535088088 );
                        kernels[119] = compute_kernel(x,   317.194212370343 );
                        kernels[120] = compute_kernel(x,   318.793354336997 );
                        kernels[121] = compute_kernel(x,   310.349598863602 );
                        kernels[122] = compute_kernel(x,   319.855592259515 );
                        kernels[123] = compute_kernel(x,   323.404828819629 );
                        decisions[0] = 0.182901415023
                        + kernels[0] * 0.506094038262
                        + kernels[1] * 0.726847325072
                        + kernels[2] * 0.439148339329
                        + kernels[3] * 0.742449447832
                        + kernels[5]
                        + kernels[8]
                        + kernels[9]
                        + kernels[10]
                        + kernels[11]
                        + kernels[12] * 0.581163913352
                        + kernels[13] * 0.066328750914
                        + kernels[14]
                        + kernels[15]
                        + kernels[16]
                        + kernels[17]
                        + kernels[18]
                        + kernels[19]
                        + kernels[20] * 0.281621617903
                        + kernels[21]
                        + kernels[22]
                        + kernels[23]
                        + kernels[24] * 0.165377295408
                        + kernels[26]
                        + kernels[27]
                        + kernels[28]
                        + kernels[29]
                        + kernels[30]
                        + kernels[31]
                        + kernels[32]
                        + kernels[33]
                        + kernels[34]
                        + kernels[35]
                        + kernels[36] * 0.880524807473
                        + kernels[37] * 0.673866889617
                        + kernels[40] * -0.794186122061
                        - kernels[41]
                        + kernels[42] * -0.179236846339
                        - kernels[43]
                        - kernels[44]
                        - kernels[46]
                        - kernels[47]
                        - kernels[48]
                        + kernels[49] * -0.449320579982
                        - kernels[50]
                        - kernels[51]
                        - kernels[52]
                        - kernels[53]
                        - kernels[54]
                        - kernels[55]
                        - kernels[56]
                        - kernels[57]
                        - kernels[60]
                        - kernels[61]
                        - kernels[62]
                        - kernels[64]
                        - kernels[65]
                        - kernels[66]
                        - kernels[67]
                        + kernels[68] * -0.808837609759
                        + kernels[71] * -0.172080760708
                        - kernels[72]
                        - kernels[76]
                        - kernels[77]
                        - kernels[78]
                        + kernels[79] * -0.659760506313
                        - kernels[80]
                        ;
                        decisions[1] = -0.003195798296
                        + kernels[0] * 0.620973109333
                        + kernels[1] * 0.892303822786
                        + kernels[2] * 0.539869686696
                        + kernels[3] * 0.911376122356
                        + kernels[4]
                        + kernels[5]
                        + kernels[6]
                        + kernels[7]
                        + kernels[8]
                        + kernels[10]
                        + kernels[12]
                        + kernels[14]
                        + kernels[15]
                        + kernels[16]
                        + kernels[17]
                        + kernels[18]
                        + kernels[19]
                        + kernels[21]
                        + kernels[23]
                        + kernels[24] * 0.596393563799
                        + kernels[25] * 0.033061656163
                        + kernels[26]
                        + kernels[27]
                        + kernels[28]
                        + kernels[30]
                        + kernels[32]
                        + kernels[33]
                        + kernels[34]
                        + kernels[35]
                        + kernels[36] * 0.414341437909
                        + kernels[37] * 0.815874639529
                        + kernels[81] * -0.99658550696
                        - kernels[82]
                        + kernels[83] * -0.996929863393
                        - kernels[84]
                        - kernels[85]
                        - kernels[86]
                        - kernels[87]
                        - kernels[88]
                        - kernels[89]
                        - kernels[90]
                        + kernels[94] * -0.815427233321
                        - kernels[95]
                        - kernels[100]
                        - kernels[104]
                        - kernels[106]
                        - kernels[108]
                        + kernels[109] * -0.996666676387
                        - kernels[110]
                        - kernels[111]
                        - kernels[112]
                        - kernels[113]
                        - kernels[114]
                        + kernels[115] * -0.166867381855
                        - kernels[117]
                        + kernels[118] * -0.851717376654
                        - kernels[119]
                        - kernels[120]
                        - kernels[121]
                        - kernels[122]
                        ;
                        decisions[2] = 0.149224023965
                        + kernels[38]
                        + kernels[39]
                        + kernels[40]
                        + kernels[41] * 0.872054982675
                        + kernels[42]
                        + kernels[43]
                        + kernels[45]
                        + kernels[46]
                        + kernels[47]
                        + kernels[48] * 0.325935891762
                        + kernels[49] * 0.166486712911
                        + kernels[51]
                        + kernels[52]
                        + kernels[53]
                        + kernels[54]
                        + kernels[56]
                        + kernels[57]
                        + kernels[58]
                        + kernels[59]
                        + kernels[60]
                        + kernels[61]
                        + kernels[62]
                        + kernels[63]
                        + kernels[64]
                        + kernels[65]
                        + kernels[66]
                        + kernels[67]
                        + kernels[69] * 0.69438848288
                        + kernels[70]
                        + kernels[71]
                        + kernels[72]
                        + kernels[73]
                        + kernels[74]
                        + kernels[75]
                        + kernels[76]
                        + kernels[77]
                        + kernels[78]
                        + kernels[79] * 0.533468149429
                        - kernels[81]
                        - kernels[82]
                        - kernels[83]
                        - kernels[84]
                        - kernels[85]
                        - kernels[86]
                        - kernels[87]
                        - kernels[90]
                        - kernels[91]
                        - kernels[92]
                        - kernels[93]
                        - kernels[94]
                        - kernels[95]
                        - kernels[96]
                        - kernels[97]
                        - kernels[98]
                        - kernels[99]
                        - kernels[100]
                        - kernels[101]
                        - kernels[102]
                        + kernels[103] * -0.864784120542
                        - kernels[105]
                        + kernels[106] * -0.727550099115
                        - kernels[107]
                        - kernels[108]
                        - kernels[109]
                        - kernels[110]
                        - kernels[111]
                        - kernels[112]
                        - kernels[113]
                        - kernels[114]
                        - kernels[116]
                        - kernels[118]
                        - kernels[119]
                        - kernels[121]
                        - kernels[123]
                        ;
                        votes[decisions[0] > 0 ? 0 : 1] += 1;
                        votes[decisions[1] > 0 ? 0 : 2] += 1;
                        votes[decisions[2] > 0 ? 1 : 2] += 1;
                        int val = votes[0];
                        int idx = 0;

                        for (int i = 1; i < 3; i++) {
                            if (votes[i] > val) {
                                val = votes[i];
                                idx = i;
                            }
                        }

                        return idx;
                    }

                protected:
                    /**
                    * Compute kernel between feature vector and support vector.
                    * Kernel type: rbf
                    */
                    float compute_kernel(float *x, ...) {
                        va_list w;
                        va_start(w, 1);
                        float kernel = 0.0;

                        for (uint16_t i = 0; i < 1; i++) {
                            kernel += pow(x[i] - va_arg(w, double), 2);
                        }

                        return exp(-0.01 * kernel);
                    }
                };
            }
        }
    }

========================== END SVC MODEL HEADER ==========================
 
