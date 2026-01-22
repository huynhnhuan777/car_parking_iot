#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_MCP23X17.h>
#include <Servo.h>

// ================= HARDWARE =================
LiquidCrystal_I2C lcd(0x27, 20, 4);
Adafruit_MCP23X17 mcp;
Servo gateServo;

// ================= PIN =================
#define SERVO_PIN 3
#define IR_ENTER  2
#define IR_BACK   4

// ================= SLOT =================
#define TOTAL_SLOT 11

bool slotState[TOTAL_SLOT];
bool slotLast[TOTAL_SLOT];

// slot thực tế & hiển thị
int haveSlotReal = TOTAL_SLOT;
int haveSlot     = TOTAL_SLOT;

// trừ hao
int pendingIn  = 0;   // xe vào cổng chưa đỗ
int pendingOut = 0;   // xe ra cổng chưa rời slot

// ================= LCD PAGE =================
bool lcdPage = 0; // 0: slot 1–6 | 1: slot 7–11
unsigned long lastPageMillis = 0;

// ================= SETUP =================
void setup() {
  Serial.begin(9600);
  Wire.begin();

  pinMode(IR_ENTER, INPUT);
  pinMode(IR_BACK, INPUT);

  gateServo.attach(SERVO_PIN);
  gateServo.write(90); // đóng cổng

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("Parking System");
  lcd.setCursor(0,1);
  lcd.print("Initializing...");
  delay(1000);

  // MCP23017
  if (!mcp.begin_I2C(0x20)) {
    lcd.clear();
    lcd.print("MCP23017 FAIL");
    while (1);
  }

  // ===== MCP PIN MODE (FIX ĐÚNG) =====
  // GPA0–7 : Slot 1–8
  for (int i = 0; i <= 7; i++)
    mcp.pinMode(i, INPUT_PULLUP);

  // GPB0–2 : Slot 9–11
  for (int i = 8; i <= 10; i++)
    mcp.pinMode(i, INPUT_PULLUP);

  readSlot();
  calculateHaveSlotReal();
  memcpy(slotLast, slotState, sizeof(slotState));

  lcd.clear();
}

// ================= LOOP =================
void loop() {
  readSlot();
  handleSlotChange();        // xử lý trừ hao theo slot
  calculateHaveSlotReal();
  calculateHaveSlotDisplay();

  gateControl();
  lcdPageAutoSwitch();
  lcdUpdate();

  delay(100);
}

// ================= READ SLOT (FIX) =================
void readSlot() {
  // Slot 1–8 : GPA0–7
  for (int i = 0; i < 8; i++) {
    slotState[i] = (mcp.digitalRead(i) == LOW);
  }

  // Slot 9–11 : GPB0–2
  for (int i = 0; i < 3; i++) {
    slotState[i + 8] = (mcp.digitalRead(i + 8) == LOW);
  }
}

// ================= SLOT CHANGE HANDLER =================
void handleSlotChange() {
  for (int i = 0; i < TOTAL_SLOT; i++) {

    // Trống → Có xe (xe đã đỗ)
    if (slotLast[i] == false && slotState[i] == true) {
      if (pendingIn > 0) pendingIn--;
    }

    // Có xe → Trống (xe rời chỗ)
    if (slotLast[i] == true && slotState[i] == false) {
      if (pendingOut > 0) pendingOut--;
    }
  }

  memcpy(slotLast, slotState, sizeof(slotState));
}

// ================= COUNT REAL SLOT =================
void calculateHaveSlotReal() {
  int count = 0;
  for (int i = 0; i < TOTAL_SLOT; i++)
    if (!slotState[i]) count++;

  haveSlotReal = count;
}

// ================= COUNT DISPLAY SLOT =================
void calculateHaveSlotDisplay() {
  haveSlot = haveSlotReal - pendingIn + pendingOut;

  if (haveSlot < 0) haveSlot = 0;
  if (haveSlot > TOTAL_SLOT) haveSlot = TOTAL_SLOT;
}

// ================= GATE CONTROL =================
void gateControl() {
  static int enterLast = HIGH;
  static int backLast  = HIGH;

  int enterNow = digitalRead(IR_ENTER);
  int backNow  = digitalRead(IR_BACK);

  // ===== XE VÀO =====
  if (enterLast == HIGH && enterNow == LOW) {
    if (haveSlot > 0) {
      pendingIn++;              // trừ hao trước
      gateServo.write(180);
    } else {
      lcd.setCursor(0,0);
      lcd.print("Parking Full !!! ");
    }
  }

  // ===== XE RA =====
  if (backLast == HIGH && backNow == LOW) {
    pendingOut++;               // cộng hao trước
    gateServo.write(180);
  }

  // ===== ĐÓNG CỔNG =====
  if (enterNow == HIGH && backNow == HIGH) {
    gateServo.write(90);
  }

  enterLast = enterNow;
  backLast  = backNow;
}

// ================= LCD PAGE SWITCH =================
void lcdPageAutoSwitch() {
  if (millis() - lastPageMillis >= 2000) {
    lcdPage = !lcdPage;
    lastPageMillis = millis();
    lcd.clear();
  }
}

// ================= LCD UPDATE =================
void lcdUpdate() {
  lcd.setCursor(0,0);
  lcd.print("Have Slot: ");
  lcd.print(haveSlot);
  lcd.print("   ");

  if (!lcdPage) {
    printSlot(0,1,0);
    printSlot(10,1,1);
    printSlot(0,2,2);
    printSlot(10,2,3);
    printSlot(0,3,4);
    printSlot(10,3,5);
  } else {
    printSlot(0,1,6);
    printSlot(10,1,7);
    printSlot(0,2,8);
    printSlot(10,2,9);
    printSlot(0,3,10);
  }
}

// ================= PRINT SLOT =================
void printSlot(int col, int row, int index) {
  lcd.setCursor(col, row);
  lcd.print("S");
  lcd.print(index + 1);
  lcd.print(":");
  lcd.print(slotState[index] ? "Fill " : "Empty");
}
