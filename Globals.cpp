#include "Globals.h"

TwoWire I2C_RTC = TwoWire(1);
Adafruit_SH1106G display = Adafruit_SH1106G(128, 64, OLED_MOSI, OLED_CLK, OLED_DC, OLED_RST, OLED_CS);
RoboEyes<Adafruit_SH1106G> roboEyes(display);

Adafruit_MPU6050 mpu;
RTC_DS3231 rtc;
DFRobotDFPlayerMini myDFPlayer;

OneButton btnL(BTN_L_PIN, true);
OneButton btnR(BTN_R_PIN, true);
ESP32Encoder encoder;

// System variable initialization
AppMode currentMode = MODE_EYES; 
volatile bool isUpdateRequired = true;
DateTime currentDateTime;
int currentMusicTrack = 1;
bool isMusicPlaying = false;
bool isBotTired = false;
bool isActionInProgress = false;

bool isAsleep = false;

int displayBrightness = 150;
volatile bool isBrightnessChanged = false; // Initialize brightness flag
bool isTimeSetting = false;
bool wasMusicPlayingBeforeAlarm = false; // Initialize alarm music state

// --- Music player variable initialization ---
MusicSelection musicSelection = SEL_PLAY; // Default selection: play button
bool isVolumeAdjusting = false;
int currentVolume = 15; // Default volume

// --- Pomodoro timer defaults ---
PomoState pomoState = POMO_SETUP_WORK;
int pomoWorkTime = 25;   // Default 25 min
int pomoShortBreak = 5;  // Default 5 min
int pomoLongBreak = 15;  // Default 15 min
long pomoTimerSeconds = 0;
int pomoRoundCounter = 0;
int pomoTotalDuration = 0;

// --- Answer Book variables ---
bool isAnswerRevealed = false;
const char* currentAnswer = "";

// Slot machine animation variables
bool isAnswerSpinning = false;
unsigned long answerSpinStartTime = 0;
float answerSpinSpeed = 0.0;
float answerSpinOffset = 0.0;
int answerReelIndices[5] = {0, 1, 2, 3, 4};

// Book icon bitmap (from Lopaka design tool)
const unsigned char image_book_bits[] PROGMEM = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0xc0,0x1f,0x00,0xc0,0x3f,0x00,0x00,0x00,0x3c,0xe0,0x01,0x78,0xc0,0x01,0x00,
  0x00,0x06,0x00,0x0e,0x07,0x00,0x06,0x00,0x00,0x01,0x00,0xf0,0x00,0x00,0x08,0x00,
  0xc0,0x00,0x00,0x60,0x00,0x00,0x30,0x00,0xb8,0x00,0x00,0x60,0x00,0x00,0xf0,0x00,
  0xb8,0x00,0x00,0x60,0x00,0x00,0xf0,0x01,0xb8,0x00,0x00,0x60,0x00,0x00,0xf0,0x01,
  0xb8,0x00,0x00,0x60,0x00,0x00,0xd0,0x01,0xb8,0x70,0xc0,0x61,0xf0,0x7f,0xd0,0x01,
  0xb8,0x00,0x00,0x66,0x04,0x00,0xd1,0x01,0xb8,0x00,0x00,0x60,0x00,0x00,0xd0,0x01,
  0xb8,0x00,0x00,0x60,0x00,0x00,0xd0,0x01,0xb8,0x00,0x00,0x60,0x00,0x00,0xd0,0x01,
  0xb8,0x00,0x00,0x60,0x00,0x00,0xd0,0x01,0xb8,0x00,0x7f,0x60,0x00,0x03,0xd0,0x01,
  0xb8,0x18,0x00,0x23,0x18,0xc0,0xd1,0x01,0xb8,0x00,0x00,0x24,0x02,0x00,0xd0,0x01,
  0xb8,0x00,0x00,0x20,0x00,0x00,0xd0,0x01,0xb8,0x00,0x00,0x60,0x00,0x00,0xd0,0x01,
  0xb8,0x00,0x00,0x60,0x00,0x00,0xd0,0x01,0xb8,0x00,0x00,0x60,0x00,0x00,0xd0,0x01,
  0xb8,0xf0,0xe0,0x60,0xe0,0x27,0xd0,0x01,0xb8,0x00,0x00,0x66,0x0c,0x00,0xd1,0x01,
  0xb8,0x00,0x00,0x60,0x00,0x00,0xd0,0x01,0xb8,0x00,0x00,0x60,0x00,0x00,0xd0,0x01,
  0xb8,0x00,0x00,0x60,0x00,0x00,0xd0,0x01,0xb8,0x00,0x00,0x60,0x00,0x00,0xd0,0x01,
  0xb8,0x80,0xff,0x63,0xfc,0x1f,0xd0,0x01,0xb8,0xf8,0x00,0x6e,0x07,0xf0,0xd1,0x01,
  0xb8,0x8f,0xff,0xf0,0xf0,0x07,0xde,0x01,0xb8,0xfc,0xff,0x67,0xfe,0xff,0xd1,0x01,
  0xb8,0xff,0xff,0x0f,0xff,0xff,0xdf,0x01,0xf8,0x1f,0x00,0xf8,0x01,0x80,0xff,0x01,
  0xf8,0x00,0x00,0xf0,0x00,0x00,0xf0,0x01,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x01,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

// Music track names (SD card mapping: index+3 = file number)
const char* const composerNames[] PROGMEM = {
    "Mozart",    // 03.mp3
    "Beethoven", // 04.mp3
    "Chopin",    // 05.mp3
    "Richard",   // 06.mp3
    "Bach",      // 07.mp3
    "Vivaldi"    // 08.mp3
};
const int totalTracks = sizeof(composerNames) / sizeof(composerNames[0]);

// Answer pool (~250 entries, stored in PROGMEM to save RAM)
const char* const answers_pool[] PROGMEM = {
    // --- Actionable advice ---
    "Go for it", "Wait for it", "Too early", "It is decisive", "Unlikely",
    "Believe in it", "Trust yourself", "Follow your heart", "Ignore the doubt", "Count on it",
    "Let it go", "Keep going", "Take a break", "Try harder", "Look closer",
    "Move on", "Start now", "Be patient", "Don't worry", "Stay calm",
    "Act now", "Think twice", "Take the risk", "Play it safe", "Seek advice",
    "Listen carefully", "Speak up", "Forgive", "Forget it", "Hold on",
    "Let go", "Change direction", "Stay put", "Explore", "Simplify",
    "Work harder", "Rest now", "Enjoy it", "Be bold", "Be kind",
    // --- Yes/No/Maybe ---
    "Smile", "Breath deep", "Why not?", "Definitely", "Of course",
    "Never", "Always", "Sometimes", "Once more", "Enough",
    "Big yes", "Small no", "Do it", "Don't do it", "Absolutely",
    // --- Time-based ---
    "It's time", "Not yet", "Soon", "Later", "Tomorrow",
    "Today", "Right now", "In a year", "Forget that", "Remember this",
    "Yes, please", "No, thanks", "Maybe later", "Right away", "Eventually",
    "Within weeks", "Within months", "Years away", "Any moment", "Never ever",
    // --- Decision helpers ---
    "Choose this", "Choose that", "Both", "Neither", "All in",
    "Step back", "Walk away", "Stop", "Look up", "Look down",
    "Inside you", "Ask a friend", "Ask family", "Trust luck", "Hard work pays",
    "Dream big", "Wake up", "Sleep on it", "It will pass", "Good things coming",
    "Be ready", "Watch out", "Safe choice", "Wild guess", "Sure thing",
    "No way", "Possible", "Impossible", "Worth it", "Waste of time",
    // --- Self-care ---
    "Drink warm water", "Take some rest", "Wear a jacket", "Eat more!",
    "Treat yourself", "Take a sick day", "Go shopping!", "Take a deep breath",
    "You deserve a break", "Drink some coffee", "Have some tea", "Stop working!",
    "Time for a vacation", "Listen to music", "Close your laptop", "Protect your eyes",
    // --- Life wisdom ---
    "Keep secret", "Tell everyone", "Help others", "Help yourself",
    "Be honest", "Stay true", "Fake it", "Make it",
    "Break it", "Fix it", "Leave it", "Take it", "Give it",
    "Save it", "Spend it", "Invest", "Wait and see", "Take charge",
    "Let others lead", "Cooperate", "Compete", "Win", "Learn",
    "Grow", "Shrink", "Expand", "Focus", "Distract",
    "Clear your mind", "Follow logic", "Follow instinct", "Be practical", "Be creative",
    "Serious", "Have fun", "Relax", "Hurry", "Slow down",
    // --- Direction ---
    "Speed up", "Turn left", "Turn right", "Go straight", "Go back",
    "New start", "End it", "Continue", "Pause", "Resume",
    "Accept it", "Reject it", "Negotiate", "Ask nicely", "Just take it", "Wait",
    "See the signs", "Ignore signs", "Trust fate", "Make fate", "Destiny",
    // --- Fate & certainty ---
    "Luck", "For sure", "I doubt it", "Who knows?", "Only you know", "Ask nature",
    // --- NEW: Motivation ---
    "You got this", "Keep pushing", "Almost there", "One more try", "Stay strong",
    "Rise up", "Bounce back", "No regrets", "Own it", "Shine bright",
    "Stay hungry", "Think bigger", "Dig deeper", "Stand tall", "Press on",
    // --- NEW: Quirky & fun ---
    "Ask your cat", "Flip a coin", "Check the moon", "Roll a dice", "Read a book",
    "Dance first", "Sing it out", "Nap on it", "Eat snacks", "Touch grass",
    "Hug someone", "Count to ten", "Close your eyes", "Feel the wind", "Just vibes",
    // --- NEW: Introspection ---
    "Look within", "Know yourself", "Find balance", "Seek peace", "Breathe out",
    "Let it flow", "Stay grounded", "Open your mind", "Free yourself", "Be still",
    "Trust the path", "Embrace change", "Accept chaos", "Find joy", "Stay curious",
    // --- NEW: Bold moves ---
    "Take the leap", "Build something", "Ship it now", "Break the mold", "Go big",
    "Say yes", "Say no", "Walk faster", "Run for it", "Jump in",
    "Make waves", "Be fearless", "Start fresh", "Level up", "Power through"
};

const int answers_count = sizeof(answers_pool) / sizeof(answers_pool[0]);