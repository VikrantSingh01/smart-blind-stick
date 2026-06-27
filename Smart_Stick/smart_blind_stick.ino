/* ==========================================================================
   SMART BLIND STICK  -  Obstacle, Voice & Puddle Alert
   Science Exhibition  |  Theme: Future Technology & Robotics
   Board: Arduino UNO R3
   ==========================================================================

   HELLO, INVENTOR!  Read this file top-to-bottom like a story. Every section
   has plain-English notes that explain WHAT the code does and WHY.

   THE BIG IDEA
   ------------
   The stick has an ultrasonic sensor on the front. It works like a bat:
   it sends out a sound "ping", waits for the echo to bounce back, and times
   how long that takes. Sound travels at a known speed (about 343 metres every
   second), so from that time we can work out the DISTANCE to the object.
   Then we warn the user with beeps, a buzzing handle, a voice, and an LED.

   HOW AN ARDUINO PROGRAM RUNS
   ---------------------------
   Every Arduino sketch has two special functions:
     setup()  -> runs ONCE when the board powers on (we get everything ready)
     loop()   -> runs OVER and OVER, forever, many times every second
   Think of setup() as "getting ready" and loop() as "doing the job".

   PIN MAP (which wire goes where) - every module: VCC->5V, GND->GND
     HC-SR04 ultrasonic:  Trig = D9 ,  Echo = D10
     Passive buzzer:      D4
     Vibration motor:     D5
     Warning LED:         D6  (through a 220 ohm resistor)
     ISD1820 voice:       P-L = D7
     Rain / water sensor: AO  = A0
   ========================================================================== */


/* ==========================================================================
   PART 1 - FEATURE SWITCHES
   --------------------------------------------------------------------------
   "#define" makes a NICKNAME for a value. Before the program is built, the
   computer swaps every nickname for its value. We use them as ON/OFF switches:
   1 means ON, 0 means OFF.
   TIP: start with VOICE and WATER OFF. Get the core working first, then turn
   them ON one at a time once those modules are wired.
   ========================================================================== */
#define USE_PASSIVE_BUZZER 1   // 1 = passive buzzer (can change pitch). 0 = active buzzer (one fixed beep).
#define USE_VIBRATION      1   // the motor that makes the handle buzz
#define USE_LED            1   // the warning light
#define USE_VOICE          0   // the talking module (ISD1820). Set to 1 AFTER you wire it.
#define USE_WATER          0   // the puddle sensor. Set to 1 AFTER you wire it.


/* ==========================================================================
   PART 2 - PINS (giving each wire a name)
   --------------------------------------------------------------------------
   The Arduino has many metal pins. Each has a number (like 9) or an analog
   name (like A0). Instead of remembering numbers, we name each pin ONCE here.
   "const int" means "a whole number that never changes". If you ever move a
   wire, you only edit it in this one place.
   ========================================================================== */
const int PIN_TRIG  = 9;    // we SEND the ping out from this pin
const int PIN_ECHO  = 10;   // we LISTEN for the echo coming back on this pin
const int PIN_BUZZ  = 4;    // buzzer
const int PIN_VIBE  = 5;    // vibration motor
const int PIN_LED   = 6;    // warning LED
const int PIN_VOICE = 7;    // ISD1820 "P-L": a quick HIGH pulse here plays the whole message once
const int PIN_WATER = A0;   // water sensor (an ANALOG pin: it reads a RANGE of values, not just on/off)


/* ==========================================================================
   PART 3 - SETTINGS YOU CAN TUNE (the "science" lives here!)
   --------------------------------------------------------------------------
   These numbers decide how the stick behaves. Changing them changes the
   experiment - perfect for your data table. Units:
     distance = centimetres (cm)   |   time = milliseconds (ms; 1000 ms = 1 s)
     pitch    = Hertz (Hz; higher number = higher-pitched sound)
   ========================================================================== */
const int RANGE_MAX    = 150;  // start warning when an object is closer than 150 cm
const int RANGE_DANGER = 20;   // full alarm when closer than 20 cm
const int GAP_FAR  = 600;      // far away  -> 600 ms between beeps (slow beeping)
const int GAP_NEAR = 120;      // very near -> 120 ms between beeps (fast beeping)
const int FREQ_FAR  = 1500;    // far away  -> 1500 Hz tone (lower pitch)
const int FREQ_NEAR = 3000;    // very near -> 3000 Hz tone (higher pitch)
const int SAMPLE_MS = 60;      // measure the distance every 60 ms (about 16 times a second)
const unsigned long VOICE_COOLDOWN = 4000;  // wait 4000 ms (4 s) before the voice may speak again


/* ==========================================================================
   PART 4 - MEMORY (variables that change while the stick runs)
   --------------------------------------------------------------------------
   A "variable" is a labelled box that holds a value the program can change.
   "unsigned long" holds a very big, never-negative number - perfect for time,
   because millis() (explained later) counts up into the billions.
   "bool" holds only true or false. "int" holds a whole number.
   ========================================================================== */
unsigned long lastSample = 0;  // the moment we last measured distance
unsigned long lastBeep   = 0;  // the moment we last flipped the beep on/off
unsigned long lastVoice  = 0;  // the moment the voice last spoke
bool buzzOn = false;           // is the beep currently sounding?
bool dangerOn = false;         // are we already in the solid "danger" alarm? (stops us restarting it every loop)
int  smoothDist = RANGE_MAX;   // our steady, best guess of the distance right now
int  waterDry   = 600;         // what "dry" reads like (the stick LEARNS this at startup)


/* ==========================================================================
   METHOD: pingOnce()
   --------------------------------------------------------------------------
   WHAT IT DOES : fires ONE ultrasonic ping and returns the distance in cm.
   HOW IT WORKS :
     1. A tiny 10-microsecond HIGH pulse on Trig tells the sensor "ping now!".
     2. pulseIn() measures how many MICROSECONDS the Echo pin stays HIGH -
        that is exactly how long the sound took to fly out AND bounce back.
     3. Distance = speed-of-sound x time / 2.
        Sound travels 0.0343 cm per microsecond. We divide by 2 because the
        sound made a round trip (there AND back again).
   RETURNS : distance in centimetres. If no echo returns in time, the path is
             clear, so we return a big number (RANGE_MAX + 50).
   (A "microsecond" is one millionth of a second - sound is fast!)
   ========================================================================== */
int pingOnce() {
  digitalWrite(PIN_TRIG, LOW);  delayMicroseconds(2);   // settle the pin so the ping starts clean
  digitalWrite(PIN_TRIG, HIGH); delayMicroseconds(10);  // the 10 us "send now" pulse
  digitalWrite(PIN_TRIG, LOW);
  long us = pulseIn(PIN_ECHO, HIGH, 12000UL); // time the echo (give up after ~2 m so we never freeze)
  if (us == 0) return RANGE_MAX + 50;          // 0 means "no echo heard" = clear path ahead
  return (int)(us * 0.0343 / 2.0);             // turn the time into a distance in cm
}


/* ==========================================================================
   METHOD: median3()
   --------------------------------------------------------------------------
   WHY IT EXISTS : a single ping can be wrong (sound can glance off at an angle
   and give a junk number). So we ping THREE times and keep the MIDDLE value.
   Keeping the middle throws away a freak high or low reading. This is called a
   "median filter", and it makes the stick much steadier and more trustworthy.
   THE TRICK : add all three, then subtract the biggest and the smallest. What
   is left MUST be the middle one.
   WHY THE SMALL WAITS : the HC-SR04 datasheet asks us to leave a short gap
   between pings so the LAST ping's sound has faded away. Without the gap, a
   leftover echo can sneak into the next ping and give a wrong "ghost" reading.
   A few milliseconds is plenty, and three pings + waits still fit inside one
   60 ms measurement slot.
   ========================================================================== */
int median3() {
  int a = pingOnce(); delay(5);   // let the first ping's echo die away...
  int b = pingOnce(); delay(5);   // ...and the second's too, before the third
  int c = pingOnce();
  return a + b + c - max(a, max(b, c)) - min(a, min(b, c)); // (sum) - (biggest) - (smallest) = middle
}


/* ==========================================================================
   METHOD: setBuzzer(on, freq)
   --------------------------------------------------------------------------
   Turns the buzzer ON (at a chosen pitch) or OFF. It takes two inputs:
     on   = true/false  (should it sound?)
     freq = the pitch in Hz  (only a PASSIVE buzzer can use this)
   A PASSIVE buzzer can play different pitches with tone(); an ACTIVE buzzer
   only makes one fixed beep, so for it we just switch the pin HIGH/LOW.
   The "#if USE_PASSIVE_BUZZER" lets the SAME program work for either buzzer.
   ========================================================================== */
void setBuzzer(bool on, int freq) {
#if USE_PASSIVE_BUZZER
  if (on) tone(PIN_BUZZ, freq); else noTone(PIN_BUZZ);  // tone() makes the pitch; noTone() stops it
#else
  digitalWrite(PIN_BUZZ, on ? HIGH : LOW);             // active buzzer: simply on or off
#endif
}


/* ==========================================================================
   METHOD: setOutputs(on)
   --------------------------------------------------------------------------
   Makes the LED and the vibration motor FOLLOW the beep: when the stick beeps,
   the light lights up and the handle buzzes; when the beep stops, they stop
   too. One little helper keeps the main code tidy and easy to read.
   ========================================================================== */
void setOutputs(bool on) {
#if USE_LED
  digitalWrite(PIN_LED, on ? HIGH : LOW);
#endif
#if USE_VIBRATION
  digitalWrite(PIN_VIBE, on ? HIGH : LOW);
#endif
}


/* ==========================================================================
   METHOD: level(d)
   --------------------------------------------------------------------------
   Turns a distance into a simple "danger number" for the laptop graph:
     0 = clear (far away)   1 = warning (getting close)   2 = danger (very close)
   This is the line the Serial Plotter draws, so judges can SEE the alert rise.
   ========================================================================== */
int level(int d) {
  if (d <= RANGE_DANGER) return 2;   // very close
  if (d <  RANGE_MAX)    return 1;   // in the warning zone
  return 0;                          // all clear
}


/* ==========================================================================
   setup()  -  runs ONCE at power-on
   --------------------------------------------------------------------------
   Here we tell each pin whether it is an OUTPUT (we control it, like the
   buzzer) or an INPUT (we read it, like the Echo pin). Then we do a quick
   self-test so we KNOW every part works before the exhibition starts.
   ========================================================================== */
void setup() {
  pinMode(PIN_TRIG, OUTPUT);   // we send pings out  -> OUTPUT
  pinMode(PIN_ECHO, INPUT);    // we listen for echo -> INPUT
  pinMode(PIN_BUZZ, OUTPUT);
#if USE_VIBRATION
  pinMode(PIN_VIBE, OUTPUT);
#endif
#if USE_LED
  pinMode(PIN_LED, OUTPUT);
#endif
#if USE_VOICE
  pinMode(PIN_VOICE, OUTPUT); digitalWrite(PIN_VOICE, LOW);  // start with the voice silent
#endif
  Serial.begin(9600);          // open a "chat line" to the laptop at speed 9600

  // --- Boot self-test: one chirp + flash proves the buzzer, LED and motor work ---
  setBuzzer(true, 2500); setOutputs(true); delay(250);   // everything ON for a quarter second...
  setBuzzer(false, 0);   setOutputs(false);              // ...then everything OFF

#if USE_WATER
  // Learn what "dry" looks like: average 20 quick readings of the water sensor.
  long s = 0;
  for (int i = 0; i < 20; i++) { s += analogRead(PIN_WATER); delay(5); }
  waterDry = s / 20;           // store the dry baseline so later we can notice "wetter than dry"
#endif

  Serial.println("distance_cm,alert_level");  // column titles for the laptop graph / spreadsheet
}


/* ==========================================================================
   loop()  -  runs FOREVER, many times per second
   --------------------------------------------------------------------------
   BIG IDEA - "non-blocking":
   We never use delay() to pause in here. delay() would make the stick STOP and
   stare at the wall, missing anything that moves. Instead we check the clock
   (millis() = the number of ms since power-on) and only act when enough time
   has passed. That way the stick keeps sensing NON-STOP. This is the secret
   that makes it feel fast and responsive.

   The loop does three jobs each time:
     (A) measure the distance      (B) check for a puddle      (C) give the alert
   ========================================================================== */
void loop() {
  unsigned long now = millis();   // "now" = how many milliseconds since the stick turned on

  // ---- (A) MEASURE the distance every SAMPLE_MS, then smooth it ----
  if (now - lastSample >= SAMPLE_MS) {       // has it been 60 ms since the last measurement?
    lastSample = now;                         // remember this moment for next time
    int raw = median3();                      // get a clean distance (the middle of 3 pings)
    // Smoothing: keep 70% of the old value + 30% of the new one. This stops the
    // number from jumping around - it eases gently toward each new reading.
    smoothDist = (smoothDist * 7 + raw * 3) / 10;
    // Send the numbers to the laptop (Serial Plotter draws them as a live graph):
    Serial.print(smoothDist); Serial.print(","); Serial.println(level(smoothDist));
  }

  // ---- (B) PUDDLE CHECK: if the floor is wet, give a special warble ----
#if USE_WATER
  if (analogRead(PIN_WATER) < (waterDry - 150)) {  // water makes the reading DROP below "dry"
    static unsigned long wt = 0;     // "static" = this box keeps its value between loops
    static bool ws = false;
    if (now - wt >= 90) { wt = now; ws = !ws; setBuzzer(ws, 2200); setOutputs(ws); } // fast warble
    return;   // skip the obstacle code this round - the puddle warning takes priority
  }
#endif

  // ---- (C) OBSTACLE ALERT: choose what to do based on the distance ----
  if (smoothDist <= RANGE_DANGER) {
    // VERY CLOSE: solid tone + steady buzz + LED on, and SPEAK the warning.
    // Start the alarm only on the FIRST loop we enter danger. Re-calling tone()
    // every loop (thousands of times a second) makes the buzzer click/stutter
    // instead of holding one clean note, so we "latch" it on with dangerOn.
    if (!dangerOn) { setBuzzer(true, FREQ_NEAR); setOutputs(true); dangerOn = true; }
#if USE_VOICE
    if (now - lastVoice >= VOICE_COOLDOWN) {   // only speak if 4 s have passed, so it never stutters
      lastVoice = now;
      digitalWrite(PIN_VOICE, HIGH); delay(60); digitalWrite(PIN_VOICE, LOW); // quick pulse = play once
    }
#endif
  }
  else if (smoothDist < RANGE_MAX) {
    // IN WARNING RANGE: beep on/off. The closer the object, the SHORTER the gap
    // (faster beeps) and the HIGHER the pitch. map() does the maths: it stretches
    // the distance (20..150 cm) onto the beep gap and onto the pitch.
    dangerOn = false;                            // left the danger zone: allow the alarm to re-arm later
    int gap  = map(smoothDist, RANGE_DANGER, RANGE_MAX, GAP_NEAR, GAP_FAR);
    int freq = map(smoothDist, RANGE_DANGER, RANGE_MAX, FREQ_NEAR, FREQ_FAR);
    if (now - lastBeep >= (unsigned long)gap) {  // is it time to flip the beep?
      lastBeep = now;
      buzzOn = !buzzOn;                          // "!" means flip: on->off, off->on
      setBuzzer(buzzOn, freq); setOutputs(buzzOn);
    }
  }
  else {
    // ALL CLEAR: nothing is close, so keep everything silent and off.
    setBuzzer(false, 0); setOutputs(false); buzzOn = false; dangerOn = false;
  }
}


/* ==========================================================================
   QUESTIONS?
   --------------------------------------------------------------------------
   Reach out to Vikrant Singh for any queries:
     https://github.com/VikrantSingh01
   ========================================================================== */
