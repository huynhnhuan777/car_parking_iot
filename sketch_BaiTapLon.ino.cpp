#include <Arduino.h>
#line 1 "C:\\Users\\Nhuan\\OneDrive - ut.edu.vn\\Documents\\Arduino\\sketch_BaiTapLon\\sketch_BaiTapLon.ino"
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

int haveSlotReal = TOTAL_SLOT; 
int haveSlot     = TOTAL_SLOT; 

// Biến điều phối logic
int pendingIn  = 0;   // Xe vào cổng nhưng chưa đỗ vào slot
int pendingOut = 0;   // Xe đã rời slot nhưng chưa đi qua cổng ra

// ================= LCD PAGE =================
bool lcdPage = 0; 
unsigned long lastPageMillis = 0;

// ================= SETUP =================
#line 34 "C:\\Users\\Nhuan\\OneDrive - ut.edu.vn\\Documents\\Arduino\\sketch_BaiTapLon\\sketch_BaiTapLon.ino"
void setup();
#line 66 "C:\\Users\\Nhuan\\OneDrive - ut.edu.vn\\Documents\\Arduino\\sketch_BaiTapLon\\sketch_BaiTapLon.ino"
void loop();
#line 80 "C:\\Users\\Nhuan\\OneDrive - ut.edu.vn\\Documents\\Arduino\\sketch_BaiTapLon\\sketch_BaiTapLon.ino"
void readSlot();
#line 86 "C:\\Users\\Nhuan\\OneDrive - ut.edu.vn\\Documents\\Arduino\\sketch_BaiTapLon\\sketch_BaiTapLon.ino"
void handleSlotEvents();
#line 102 "C:\\Users\\Nhuan\\OneDrive - ut.edu.vn\\Documents\\Arduino\\sketch_BaiTapLon\\sketch_BaiTapLon.ino"
void calculateHaveSlotReal();
#line 110 "C:\\Users\\Nhuan\\OneDrive - ut.edu.vn\\Documents\\Arduino\\sketch_BaiTapLon\\sketch_BaiTapLon.ino"
void calculateHaveSlotDisplay();
#line 120 "C:\\Users\\Nhuan\\OneDrive - ut.edu.vn\\Documents\\Arduino\\sketch_BaiTapLon\\sketch_BaiTapLon.ino"
void gateControl();
#line 152 "C:\\Users\\Nhuan\\OneDrive - ut.edu.vn\\Documents\\Arduino\\sketch_BaiTapLon\\sketch_BaiTapLon.ino"
void lcdPageAutoSwitch();
#line 160 "C:\\Users\\Nhuan\\OneDrive - ut.edu.vn\\Documents\\Arduino\\sketch_BaiTapLon\\sketch_BaiTapLon.ino"
void lcdUpdate();
#line 177 "C:\\Users\\Nhuan\\OneDrive - ut.edu.vn\\Documents\\Arduino\\sketch_BaiTapLon\\sketch_BaiTapLon.ino"
void printSlot(int col, int row, int index);
#line 183 "C:\\Users\\Nhuan\\OneDrive - ut.edu.vn\\Documents\\Arduino\\sketch_BaiTapLon\\sketch_BaiTapLon.ino"
void sendSerialData();
#line 34 "C:\\Users\\Nhuan\\OneDrive - ut.edu.vn\\Documents\\Arduino\\sketch_BaiTapLon\\sketch_BaiTapLon.ino"
void setup() {
  Serial.begin(9600);
  Wire.begin();

  pinMode(IR_ENTER, INPUT);
  pinMode(IR_BACK, INPUT);

  gateServo.attach(SERVO_PIN);
  gateServo.write(90); 

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("Parking System");
  delay(1000);

  if (!mcp.begin_I2C(0x20)) {
    lcd.clear();
    lcd.print("MCP23017 FAIL");
    while (1);
  }

  for (int i = 0; i <= 7; i++) mcp.pinMode(i, INPUT_PULLUP);
  for (int i = 8; i <= 10; i++) mcp.pinMode(i, INPUT_PULLUP);

  readSlot();
  calculateHaveSlotReal();
  memcpy(slotLast, slotState, sizeof(slotState));
  lcd.clear();
}

// ================= LOOP =================
void loop() {
  readSlot();
  handleSlotEvents();       
  calculateHaveSlotReal();
  calculateHaveSlotDisplay();

  gateControl();
  lcdPageAutoSwitch();
  lcdUpdate();
  sendSerialData();

  delay(100);
}

void readSlot() {
  for (int i = 0; i < 8; i++) slotState[i] = (mcp.digitalRead(i) == LOW);
  for (int i = 0; i < 3; i++) slotState[i + 8] = (mcp.digitalRead(i + 8) == LOW);
}

// ================= XỬ LÝ SỰ KIỆN TẠI SLOT =================
void handleSlotEvents() {
  for (int i = 0; i < TOTAL_SLOT; i++) {
    // 1. XE ĐỖ VÀO: Trống -> Có xe
    if (slotLast[i] == false && slotState[i] == true) {
      if (pendingIn > 0) pendingIn--; 
    }
    
    // 2. XE RỜI SLOT: Có xe -> Trống
    if (slotLast[i] == true && slotState[i] == false) {
      // Khi xe rời slot, ta tăng pendingOut để "giữ" không cho LCD tăng số slot trống liền
      pendingOut++; 
    }
  }
  memcpy(slotLast, slotState, sizeof(slotState));
}

void calculateHaveSlotReal() {
  int count = 0;
  for (int i = 0; i < TOTAL_SLOT; i++)
    if (!slotState[i]) count++;
  haveSlotReal = count;
}

// ================= TÍNH TOÁN HIỂN THỊ (QUAN TRỌNG) =================
void calculateHaveSlotDisplay() {
  // haveSlot = (Số slot trống thực tế) - (Số xe đang vào) - (Số xe đã rời slot nhưng chưa ra cổng)
  // Việc trừ pendingOut ở đây giúp triệt tiêu cái +1 khi haveSlotReal tăng lúc xe rời chỗ.
  haveSlot = haveSlotReal - pendingIn - pendingOut;

  if (haveSlot < 0) haveSlot = 0;
  if (haveSlot > TOTAL_SLOT) haveSlot = TOTAL_SLOT;
}

// ================= ĐIỀU KHIỂN CỔNG =================
void gateControl() {
  static int enterLast = HIGH;
  static int backLast  = HIGH;

  int enterNow = digitalRead(IR_ENTER);
  int backNow  = digitalRead(IR_BACK);

  // CẢM BIẾN VÀO (IR_ENTER)
  if (enterLast == HIGH && enterNow == LOW) {
    if (haveSlot > 0) {
      pendingIn++; 
      gateServo.write(180); 
    }
  }

  // CẢM BIẾN RA (IR_BACK) - ĐÂY LÀ NƠI QUYẾT ĐỊNH TĂNG SLOT
  if (backLast == HIGH && backNow == LOW) {
    gateServo.write(180); 
    // Khi xe đi qua cảm biến này, ta mới giải phóng pendingOut
    // Lúc này LCD mới chính thức tăng thêm 1 slot trống
    if (pendingOut > 0) pendingOut--; 
  }

  // ĐÓNG CỔNG
  if (enterNow == HIGH && backNow == HIGH) {
    gateServo.write(90);
  }

  enterLast = enterNow;
  backLast  = backNow;
}

void lcdPageAutoSwitch() {
  if (millis() - lastPageMillis >= 2500) {
    lcdPage = !lcdPage;
    lastPageMillis = millis();
    lcd.clear();
  }
}

void lcdUpdate() {
  lcd.setCursor(0,0);
  lcd.print("Have Slot: ");
  lcd.print(haveSlot);
  lcd.print("   ");

  if (!lcdPage) {
    printSlot(0,1,0); printSlot(10,1,1);
    printSlot(0,2,2); printSlot(10,2,3);
    printSlot(0,3,4); printSlot(10,3,5);
  } else {
    printSlot(0,1,6); printSlot(10,1,7);
    printSlot(0,2,8); printSlot(10,2,9);
    printSlot(0,3,10);
  }
}

void printSlot(int col, int row, int index) {
  lcd.setCursor(col, row);
  lcd.print("S"); lcd.print(index + 1); lcd.print(":");
  lcd.print(slotState[index] ? "Full " : "Empty");
}

void sendSerialData() {
  for (int i = 0; i < TOTAL_SLOT; i++) {
    Serial.print(slotState[i] ? 1 : 0);
    Serial.print(",");
  }
  Serial.println(haveSlot); 
}
