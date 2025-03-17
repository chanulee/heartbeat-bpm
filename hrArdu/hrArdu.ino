#include <Wire.h>
#include <MAX30105.h>

MAX30105 particleSensor;

const int sampleSize = 100;         // Number of samples for running average
uint16_t irBuffer[sampleSize];        // Circular buffer for IR values
int sampleCounter = 0;
float runningAverage = 0.0;

unsigned long lastBeatTime = 0;       // Timestamp of the last detected beat

// For BPM smoothing: we'll average BPM values over a set period
const unsigned long updateInterval = 3000;  // Update interval in milliseconds (5 seconds)
unsigned long lastBPMUpdateTime = 0;
float bpmSum = 0;
int bpmCount = 0;
int stableBPM = 0;
int prevStableBPM = 0;
const float alpha = 0.2;  // Smoothing factor for exponential smoothing (0 < alpha < 1)

bool beatDetected = false;

void setup() {
  Serial.begin(115200);
  delay(1000); // Allow time for Serial Monitor to connect
  
  // Initialize the sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30102 not found. Check wiring/power.");
    while(1); // Halt if sensor is not found
  } else {
    Serial.println("MAX30102 initialized successfully.");
  }
  
  // Configure sensor settings (adjust as needed)
  particleSensor.setup();  // Default configuration; adjust LED brightness, sample rate, etc.
}

void loop() {
  // Read the IR value from the sensor
  uint16_t irValue = particleSensor.getIR();

  // Only proceed if a finger is placed (adjust threshold as needed)
  if (irValue < 40000) {
    delay(20);
    return;
  }
  
  // Add current reading to the circular buffer for running average calculation
  irBuffer[sampleCounter % sampleSize] = irValue;
  sampleCounter++;

  // Once enough samples have been collected, compute the running average
  if (sampleCounter >= sampleSize) {
    runningAverage = 0;
    for (int i = 0; i < sampleSize; i++) {
      runningAverage += irBuffer[i];
    }
    runningAverage /= sampleSize;

    // Check for a beat: if the current IR value exceeds the running average by 2%
    if (irValue > runningAverage * 1.02) {
      unsigned long currentTime = millis();
      // Use a debounce delay of 600ms to avoid multiple detections per beat
      if (!beatDetected || (currentTime - lastBeatTime) > 600) {
        // Calculate the beat interval if not the first beat
        if (lastBeatTime != 0) {
          unsigned long beatInterval = currentTime - lastBeatTime;
          // Only consider intervals above 600ms as valid beats
          if (beatInterval >= 600) {
            int beatBPM = 60000 / beatInterval; // Convert beat interval (ms) to BPM
            // Only add plausible BPM values (e.g., between 30 and 200 BPM)
            if (beatBPM >= 30 && beatBPM <= 200) {
              bpmSum += beatBPM;
              bpmCount++;
            }
          }
        }
        lastBeatTime = currentTime;
        beatDetected = true;
      }
    } else {
      beatDetected = false;
    }
    
    // Every updateInterval (5 seconds), calculate and print a stable (averaged) BPM reading
    if ((millis() - lastBPMUpdateTime) > updateInterval && bpmCount > 0) {
      int newBPM = (int)(bpmSum / bpmCount);
      
      // Apply exponential smoothing: if there's no previous value, initialize it with newBPM
      if (prevStableBPM == 0) {
        stableBPM = newBPM;
      } else {
        stableBPM = (int)(alpha * newBPM + (1.0 - alpha) * prevStableBPM);
      }
      prevStableBPM = stableBPM;
      
      // Reset the BPM sum and count for the next interval
      bpmSum = 0;
      bpmCount = 0;
      lastBPMUpdateTime = millis();
      
      // Print only the stable BPM number (for Max/MSP parsing)
      Serial.println(stableBPM);
    }
  }
  
  // Delay to control sampling rate (adjust as needed)
  delay(20);
}
