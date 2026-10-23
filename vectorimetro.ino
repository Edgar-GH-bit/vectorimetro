#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1331.h>
#include <MPU6050.h>

// 1 = muestra el logo (requiere logo_array.h con "const uint8_t logo[] PROGMEM")
// 0 = no usa el logo (para descartar que el error venga de logo_array.h)
#define USAR_LOGO 1

#if USAR_LOGO
#include "logo_array.h"
#endif

#define OLED_CS   10
#define OLED_DC   9
#define OLED_RES  8

#define TRIG1  4
#define ECHO1  5
#define TRIG2  6
#define ECHO2  7

const float L_PLANO = 30.0;       
const float g = 9.81;
const int N_MEDIA = 10;
const int NUM_MUESTRAS_CAL = 200;
const float LSB_POR_G = 16384.0;   
const float RAD_A_GRADOS = 57.2958;


#define MODO_NORMAL  0
#define MODO_TEOREMA 1
#define MODO_RAW     2

float masa = 0.5;
int modo = MODO_NORMAL;
int16_t offsetAX = 0, offsetAY = 0, offsetAZ = 0;

Adafruit_SSD1331 display = Adafruit_SSD1331(&SPI, OLED_CS, OLED_DC, OLED_RES);
MPU6050 mpu;

#define BLACK   0x0000
#define WHITE   0xFFFF
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define YELLOW  0xFFE0
#define MAGENTA 0xF81F


void printGrados(float valor, int decimales) {
  display.print(valor, decimales);
  display.write(247);
}

void calibrarMPU() {
  Serial.println(F("Calibrando MPU6050... NO MOVER (plano en horizontal)"));
  display.fillScreen(BLACK);
  display.setTextColor(YELLOW);
  display.setTextSize(1);
  display.setCursor(0, 28);
  display.print(F("CALIBRANDO..."));

  long sumX = 0, sumY = 0, sumZ = 0;
  for (int i = 0; i < NUM_MUESTRAS_CAL; i++) {
    int16_t ax, ay, az, gx, gy, gz;
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    sumX += ax;
    sumY += ay;
    sumZ += az;
    delay(5);
  }

  offsetAX = sumX / NUM_MUESTRAS_CAL;
  offsetAY = sumY / NUM_MUESTRAS_CAL;

  offsetAZ = (sumZ / NUM_MUESTRAS_CAL) - (int16_t)LSB_POR_G;

  Serial.print(F("Offsets: AX=")); Serial.print(offsetAX);
  Serial.print(F(" AY=")); Serial.print(offsetAY);
  Serial.print(F(" AZ=")); Serial.println(offsetAZ);

  display.fillRect(0, 24, 96, 16, BLACK);
  display.setTextColor(GREEN);
  display.setCursor(0, 28);
  display.print(F("OK!"));
  delay(1000);
}

long leerDist(int trig, int echo) {
  digitalWrite(trig, LOW);
  delayMicroseconds(5);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  return pulseIn(echo, HIGH, 30000);   
}

float leerDistMedia(int trig, int echo) {
  long suma = 0;
  for (int i = 0; i < N_MEDIA; i++) {
    suma += leerDist(trig, echo);
    delay(30);
  }
  return (suma / (float)N_MEDIA) * 0.034 / 2.0;
}


void mostrarRaw() {
  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  
  float thetaMPU = atan2((float)(ax - offsetAX), (float)(az - offsetAZ)) * RAD_A_GRADOS;

 
  long t1 = leerDist(TRIG1, ECHO1);
  delay(30);
  long t2 = leerDist(TRIG2, ECHO2);
  float d1 = t1 * 0.034 / 2.0;
  float d2 = t2 * 0.034 / 2.0;

  
  float hRaw = d2;
  if (hRaw > L_PLANO) hRaw = L_PLANO;
  float thetaUS = asin(hRaw / L_PLANO) * RAD_A_GRADOS;

  display.fillRect(0, 0, 96, 64, BLACK);
  display.setTextSize(1);

  display.setTextColor(MAGENTA);
  display.setCursor(0, 0);
  display.print(F("RAW SENSORES"));

  display.setTextColor(CYAN);
  display.setCursor(0, 8);
  display.print(F("ax:"));
  display.print(ax);
  display.setCursor(0, 16);
  display.print(F("ay:"));
  display.print(ay);
  display.setCursor(0, 24);
  display.print(F("az:"));
  display.print(az);

  display.setTextColor(GREEN);
  display.setCursor(0, 32);
  display.print(F("ang MPU:"));
  printGrados(thetaMPU, 1);

  display.setTextColor(YELLOW);
  display.setCursor(0, 40);
  display.print(F("t1:"));
  display.print(t1);
  display.print(F(" d:"));
  display.print(d1, 1);

  display.setCursor(0, 48);
  display.print(F("t2:"));
  display.print(t2);
  display.print(F(" d:"));
  display.print(d2, 1);

  display.setTextColor(GREEN);
  display.setCursor(0, 56);
  display.print(F("ang US:"));
  printGrados(thetaUS, 1);

  Serial.print(F("RAW ax=")); Serial.print(ax);
  Serial.print(F(" ay=")); Serial.print(ay);
  Serial.print(F(" az=")); Serial.print(az);
  Serial.print(F(" angMPU(grados)=")); Serial.print(thetaMPU, 1);
  Serial.print(F(" t1=")); Serial.print(t1);
  Serial.print(F(" t2=")); Serial.print(t2);
  Serial.print(F(" angUS(grados)=")); Serial.println(thetaUS, 1);
}

void setup() {
  Serial.begin(9600);
  Serial.println(F("Comandos: N=normal, T=teorema, R=raw, numero=masa(kg)"));

  pinMode(TRIG1, OUTPUT);
  pinMode(ECHO1, INPUT);
  pinMode(TRIG2, OUTPUT);
  pinMode(ECHO2, INPUT);

  display.begin();
  display.setRotation(0);
  display.fillScreen(BLACK);

#if USAR_LOGO
  display.drawBitmap(0, 0, logo, 96, 64, WHITE);
#else
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 28);
  display.print(F("PLANO INCLINADO"));
#endif
  delay(2000);
  display.fillScreen(BLACK);

  Wire.begin();
  Wire.setClock(400000);
  mpu.initialize();
  if (!mpu.testConnection()) {
    display.setTextColor(RED);
    display.setTextSize(1);
    display.setCursor(0, 28);
    display.print(F("MPU6050 FAIL!"));
    while (true);
  }
  mpu.setFullScaleGyroRange(MPU6050_GYRO_FS_500);
  calibrarMPU();
}

void loop() {
  
  if (Serial.available()) {
    char buf[16];
    int n = Serial.readBytesUntil('\n', buf, sizeof(buf) - 1);
    while (n > 0 && (buf[n - 1] == '\r' || buf[n - 1] == ' ')) n--;
    buf[n] = '\0';

    if (n == 1 && (buf[0] == 'T' || buf[0] == 't')) {
      modo = MODO_TEOREMA;
    } else if (n == 1 && (buf[0] == 'N' || buf[0] == 'n')) {
      modo = MODO_NORMAL;
    } else if (n == 1 && (buf[0] == 'R' || buf[0] == 'r')) {
      modo = MODO_RAW;
    } else if (n > 0) {
      float nuevaMasa = atof(buf);
      if (nuevaMasa > 0 && nuevaMasa < 1000) {
        masa = nuevaMasa;
        Serial.print(F("Masa: "));
        Serial.print(masa, 3);
        Serial.println(F(" kg"));
      }
    }
  }

  
  if (modo == MODO_RAW) {
    mostrarRaw();
    delay(100);
    return;
  }


  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  ax -= offsetAX;
  az -= offsetAZ;

  float a_MPU    = ax / LSB_POR_G * g;
  float F_MPU    = masa * a_MPU;
  float thetaMPU = atan2((float)ax, (float)az) * RAD_A_GRADOS;   // GRADOS

  // ---------- METODO 2: Pitagoras -> reconstruccion de a -> F = m * a ----------
  float h = leerDistMedia(TRIG2, ECHO2);
  if (h > L_PLANO) h = L_PLANO;   // evita sqrt de negativo

  // Teorema de Pitagoras
  float base = sqrt(L_PLANO * L_PLANO - h * h);

  // Reconstruccion de la aceleracion
  float a_US = g * (h / L_PLANO);

  // Inclinacion en GRADOS
  float thetaUS = asin(h / L_PLANO) * RAD_A_GRADOS;

  // Segunda ley de Newton
  float F_US = masa * a_US;

  // ---------- Pantalla ----------
  display.fillRect(0, 0, 96, 64, BLACK);
  display.setTextSize(1);

  if (modo == MODO_TEOREMA) {
    // ===== MENU: Teorema de Pitagoras y reconstruccion de a =====
    display.setTextColor(MAGENTA);
    display.setCursor(0, 0);
    display.print(F("PITAGORAS"));

    display.setTextColor(CYAN);
    display.setCursor(0, 8);
    display.print(F("L:"));
    display.print(L_PLANO, 1);
    display.print(F(" h:"));
    display.print(h, 1);

    display.setCursor(0, 16);
    display.print(F("b=sqrt(L2-h2)"));

    display.setTextColor(WHITE);
    display.setCursor(0, 24);
    display.print(F("b:"));
    display.print(base, 2);

    display.setTextColor(MAGENTA);
    display.setCursor(0, 32);
    display.print(F("RECONSTRUCCION A"));

    display.setTextColor(YELLOW);
    display.setCursor(0, 40);
    display.print(F("a=g*h/L"));

    display.setCursor(0, 48);
    display.print(F("a:"));
    display.print(a_US, 3);
    display.print(F(" m/s2"));

    display.setTextColor(GREEN);
    display.setCursor(0, 56);
    display.print(F("ang:"));
    printGrados(thetaUS, 1);
  } else {
    // ===== PANTALLA PRINCIPAL =====
    // --- Metodo 1: F = m*a ---
    display.setTextColor(CYAN);
    display.setCursor(0, 0);
    display.print(F("F=m*a (MPU)"));

    display.setCursor(0, 8);
    display.print(F("m:"));
    display.print(masa, 3);
    display.print(F("kg "));
    printGrados(thetaMPU, 1);

    display.setCursor(0, 16);
    display.print(F("a:"));
    display.print(a_MPU, 3);
    display.print(F(" m/s2"));

    display.setCursor(0, 24);
    display.print(F("F:"));
    display.print(F_MPU, 3);
    display.print(F(" N"));

    // --- Metodo 2: Pitagoras + F=m*a ---
    display.setTextColor(YELLOW);
    display.setCursor(0, 32);
    display.print(F("Pitagoras+F=m*a"));

    display.setCursor(0, 40);
    display.print(F("m:"));
    display.print(masa, 3);
    display.print(F("kg "));
    printGrados(thetaUS, 1);

    display.setCursor(0, 48);
    display.print(F("a:"));
    display.print(a_US, 3);
    display.print(F(" m/s2"));

    display.setCursor(0, 56);
    display.print(F("F:"));
    display.print(F_US, 3);
    display.print(F(" N"));
  }

  delay(100);
}