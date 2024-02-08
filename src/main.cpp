// Include the necessary libraries
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <si5351.h>

// Create a Si5351 object
Si5351 si5351;

// Create an SSD1306 display object
#define SCREEN_WIDTH 128 // OLED display width in pixels
#define SCREEN_HEIGHT 64 // OLED display height in pixels
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);

// Set the starting frequency
uint64_t freq = 60000000ULL;
bool transmitBeep = true; // Flag to control continuous beeping

// Define a simple melody (you can customize this)
const int melodyNotes[] = {262, 294, 330, 349, 392, 440, 494, 523}; // Example: C4 to C5

void setup() {
  // Initialize the I2C communication protocol
  Wire.begin();

  // Initialize the OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;) {}
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  // Initialize the SI5351
  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);
  si5351.set_freq(freq, SI5351_CLK0);

  // Start serial communication
  Serial.begin(9600);
}

// Play the melody using the Melody library
void transmitMelody() {
  const int noteDuration = 300; // Adjust this value for shorter or longer notes

  for (int i = 0; i < sizeof(melodyNotes) / sizeof(melodyNotes[0]); i++) {
    si5351.output_enable(SI5351_CLK0, 1); // Enable output
    tone(8, melodyNotes[i]); // Play the note
    delay(noteDuration); // Note duration (adjust as needed)
    noTone(8);
    si5351.output_enable(SI5351_CLK0, 0); // Disable output
    delay(50); // Short pause between notes (adjust as needed)
  }
}


void loop() {
  // Display the frequency and "Frequency" on the same line
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Frequency: ");
  display.print(String(freq / 1000000ULL) + " MHz");

  // Display transmission status
  display.setCursor(0, 20);
  if (transmitBeep) {
    display.println("Playing Melody");
      transmitMelody();
  } else {
    display.println("Not Transmitting");
  }

  display.display();

  // Check for input from the serial monitor
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    if (input == "start") {
      transmitBeep = true;
    } else if (input == "stop") {
      transmitBeep = false;
    } else {
      freq = input.toInt() * 1000000ULL; // Convert input to frequency in Hz
      si5351.set_freq(freq, SI5351_CLK0);
    }
  }

  // Transmit continuous beeping if enabled
  if (transmitBeep) {
    si5351.output_enable(SI5351_CLK0, 1);
    delay(500); // Beep duration (adjust as needed)
    si5351.output_enable(SI5351_CLK0, 0);
    delay(500); // Silence duration (adjust as needed)
  } else {
    si5351.output_enable(SI5351_CLK0, 0);
  }
}