#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Pin definitions
#define SUHU_PIN 32
#define KECEPATAN_PIN 33
#define LED_PIN 25

// OLED configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  Wire.begin(21, 22);
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(115200);
  
  // Inisialisasi OLED yang lebih baik menggunakan library
  if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Starting...");
  display.display();
  delay(1000);
}

void loop() {
  // Baca sensor dengan filter noise sederhana
  float suhuRaw = 0;
  float kecepatanRaw = 0;
  
  // Rata-rata dari 5 pembacaan untuk mengurangi noise
  for (int i = 0; i < 5; i++) {
    suhuRaw += analogRead(SUHU_PIN);
    kecepatanRaw += analogRead(KECEPATAN_PIN);
    delay(10);
  }
  suhuRaw /= 5;
  kecepatanRaw /= 5;
  
  // Konversi ke nilai yang sebenarnya
  float suhu = 20.0 + (suhuRaw / 4095.0) * 18.0;
  int kecepatan = map(kecepatanRaw, 0, 4095, 800, 1500);

  // Debugging nilai input
  Serial.print("Suhu: "); Serial.println(suhu);
  Serial.print("Kecepatan: "); Serial.println(kecepatan);

  // Hitung fuzzy
  float output = hitungFuzzy(suhu, kecepatan);

  Serial.print("Output Fuzzy: "); Serial.println(output);

  // Update display dengan method yang benar
  updateDisplay(suhu, kecepatan, output);

  // Kontrol pemanas
  if (output >= 75.0) {
    digitalWrite(LED_PIN, HIGH);
  } else {
    digitalWrite(LED_PIN, LOW);
  }

  delay(500);
}

// Fungsi untuk menghitung fuzzy dengan implementasi yang lebih baik
float hitungFuzzy(float suhu, float kecepatan) {
  // Hitung membership suhu - dengan batas yang lebih jelas
  float suhuRendah = fuzzify(suhu, 18.0, 20.0, 24.0, 26.0);
  float suhuSedang = fuzzify(suhu, 20.0, 24.0, 26.0, 28.0);
  float suhuTinggi = fuzzify(suhu, 24.0, 28.0, 38.0, 40.0);

  // Hitung membership kecepatan - dengan batas yang lebih jelas
  float kecepatanLambat = fuzzify(kecepatan, 700.0, 800.0, 900.0, 950.0);
  float kecepatanSedang = fuzzify(kecepatan, 850.0, 950.0, 1100.0, 1200.0);
  float kecepatanCepat = fuzzify(kecepatan, 1100.0, 1200.0, 1500.0, 1600.0);

  // Rules dengan bobot output yang sudah ditentukan
  float outputs[7] = {80.0, 75.0, 65.0, 60.0, 50.0, 45.0, 40.0};
  float rules[7];
  
  rules[0] = min(suhuRendah, kecepatanLambat);     // Rule 1: Jika suhu rendah dan kecepatan lambat, output = 80
  rules[1] = min(suhuRendah, kecepatanSedang);     // Rule 2: Jika suhu rendah dan kecepatan sedang, output = 75 
  rules[2] = min(suhuSedang, kecepatanLambat);     // Rule 3: Jika suhu sedang dan kecepatan lambat, output = 65
  rules[3] = min(suhuSedang, kecepatanSedang);     // Rule 4: Jika suhu sedang dan kecepatan sedang, output = 60
  rules[4] = min(suhuTinggi, kecepatanLambat);     // Rule 5: Jika suhu tinggi dan kecepatan lambat, output = 50
  rules[5] = min(suhuTinggi, kecepatanSedang);     // Rule 6: Jika suhu tinggi dan kecepatan sedang, output = 45
  rules[6] = min(suhuTinggi, kecepatanCepat);      // Rule 7: Jika suhu tinggi dan kecepatan cepat, output = 40

  // Defuzzifikasi dengan metode weighted average
  float numerator = 0.0;
  float denominator = 0.0;
  
  for (int i = 0; i < 7; i++) {
    numerator += rules[i] * outputs[i];
    denominator += rules[i];
  }

  // Cek pembagian dengan nol
  if (denominator < 0.001) {
    return 0.0;  // Nilai default jika tidak ada rule yang aktif
  }
  
  return numerator / denominator;
}

// Fungsi helper untuk fuzzifikasi yang lebih jelas dan konsisten
float fuzzify(float value, float a, float b, float c, float d) {
  if (value <= a || value >= d) {
    return 0.0;  // Di luar rentang
  } else if (value >= b && value <= c) {
    return 1.0;  // Nilai maksimum
  } else if (value > a && value < b) {
    return (value - a) / (b - a);  // Naik
  } else {
    return (d - value) / (d - c);  // Turun
  }
}

// Update display dengan menggunakan library yang benar
void updateDisplay(float suhu, int kecepatan, float output) {
  display.clearDisplay();
  display.setCursor(0, 0);

  // Tampilkan suhu
  display.print("Suhu: ");
  display.print(suhu, 1);
  display.println(" C");

  // Tampilkan kecepatan air
  display.print("Kec Air: ");
  display.print(kecepatan);
  display.println(" ML");

  // Tampilkan hasil fuzzy
  display.print("Pemanas: ");
  display.print(output, 1);
  display.println("%");

  // Tampilkan status pemanas
  if (output >= 75.0) {
    display.println("Pemanas ON");
  } else {
    display.println("Pemanas OFF");
  }

  // Refresh display
  display.display();
}