#include <SPI.h>
#include <Wire.h>
#include <string.h>
#include <EEPROM.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1331.h>
#include <MPU6050.h>

#define USAR_LOGO 1
#if USAR_LOGO
#include "logo_array.h"
#endif

#define OLED_CS   10
#define OLED_DC   9
#define OLED_RES  8

#define TRIG_BASE  6
#define ECHO_BASE  7
#define TRIG_ALT   4
#define ECHO_ALT   5

#define EJE_PENDIENTE   'X'
#define SIGNO_PENDIENTE 1

const float g            = 9.81;
const float LSB_POR_G    = 16384.0;
const float RAD_A_GRADOS = 57.2958;
const int   N_MEDIA      = 10;
const int   N_ACEL       = 20;
const float ALPHA        = 0.2;
const float masa         = 0.5;

#define MODO_NORMAL  0
#define MODO_TEOREMA 1
#define MODO_RAW     2

struct CalData {
  uint32_t magic;
  int16_t  ax, ay, az;
  int16_t  gx, gy, gz;
};

struct Metodo2 {
  float b, h, H, theta, a, F;
  bool  valido;
};

const uint32_t MAGIC = 0xCA11B006;
CalData cal;

int   modo = MODO_NORMAL;

int   calN = 0;
float calRaw[2], calReal[2];
float calK = 1.0, calB = 0.0;

float angFiltrado = 0;
bool  filtroIniciado = false;

char cmdBuf[16];
byte cmdLen = 0;

Adafruit_SSD1331 display = Adafruit_SSD1331(&SPI, OLED_CS, OLED_DC, OLED_RES);
MPU6050 mpu;

#define BLACK   0x0000
#define WHITE   0xFFFF
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define YELLOW  0xFFE0
#define MAGENTA 0xF81F

void aplicarOffsets() {
  mpu.setXAccelOffset(cal.ax);
  mpu.setYAccelOffset(cal.ay);
  mpu.setZAccelOffset(cal.az);
  mpu.setXGyroOffset(cal.gx);
  mpu.setYGyroOffset(cal.gy);
  mpu.setZGyroOffset(cal.gz);
}

void calibrarMPU() {
  Serial.println(F("Recalibrando MPU. Deja la rampa en su punto mas bajo (30 grados) y quieta 5s..."));
  mpu.setXAccelOffset(0);
  mpu.setYAccelOffset(0);
  mpu.setZAccelOffset(0);
  mpu.setXGyroOffset(0);
  mpu.setYGyroOffset(0);
  mpu.setZGyroOffset(0);
  delay(1000);

  display.fillScreen(BLACK);
  display.setTextColor(YELLOW);
  display.setTextSize(1);
  display.setCursor(0, 28);
  display.print(F("CALIBRANDO MPU"));

  long sAx = 0, sAy = 0, sAz = 0, sGx = 0, sGy = 0, sGz = 0;
  const int N = 2000;
  for (int i = 0; i < N; i++) {
    int16_t ax, ay, az, gx, gy, gz;
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    sAx += ax; sAy += ay; sAz += az;
    sGx += gx; sGy += gy; sGz += gz;
    delay(1);
  }

  const float th0     = 30.0 / RAD_A_GRADOS;
  const float axIdeal = 16384.0 * sin(th0);
  const float azIdeal = 16384.0 * cos(th0);

  cal.ax = -((sAx / N) - axIdeal) / 8;
  cal.ay = -((sAy / N) - 0)       / 8;
  cal.az = -((sAz / N) - azIdeal) / 8;
  cal.gx = -((sGx / N) - 0) / 4;
  cal.gy = -((sGy / N) - 0) / 4;
  cal.gz = -((sGz / N) - 0) / 4;
  aplicarOffsets();

  Serial.print(F("Offsets: ax=")); Serial.print(cal.ax);
  Serial.print(F(" ay=")); Serial.print(cal.ay);
  Serial.print(F(" az=")); Serial.println(cal.az);

  cal.magic = MAGIC;
  EEPROM.put(0, cal);

  display.fillScreen(BLACK);
  display.setTextColor(GREEN);
  display.setCursor(0, 28);
  display.print(F("CALIBRADO OK"));
  delay(1500);
  Serial.println(F("MPU calibrado y guardado en EEPROM."));
}

void printGrados(float valor, int decimales) {
  display.print(valor, decimales);
  display.write(247);
}

float valorEje(char eje, int16_t ax, int16_t ay, int16_t az) {
  if (eje == 'X') return ax;
  if (eje == 'Y') return ay;
  return az;
}

float anguloDeLectura(int16_t ax, int16_t ay, int16_t az) {
  float p    = SIGNO_PENDIENTE * valorEje(EJE_PENDIENTE, ax, ay, az);
  float t2   = (float)ax * ax + (float)ay * ay + (float)az * az;
  float perp = sqrt(max(t2 - p * p, 0.0));
  return atan2(p, perp) * RAD_A_GRADOS;
}

float anguloCrudo() {
  float sumP = 0, sumT2 = 0;
  for (int i = 0; i < N_ACEL; i++) {
    int16_t ax, ay, az, gx, gy, gz;
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    sumP  += SIGNO_PENDIENTE * valorEje(EJE_PENDIENTE, ax, ay, az);
    sumT2 += (float)ax * ax + (float)ay * ay + (float)az * az;
    delay(2);
  }
  float p    = sumP / N_ACEL;
  float t2   = sumT2 / N_ACEL;
  float perp = sqrt(max(t2 - p * p, 0.0));
  return atan2(p, perp) * RAD_A_GRADOS;
}

float leerAngulo() {
  float angCal = calK * anguloCrudo() + calB;
  if (!filtroIniciado) {
    angFiltrado = angCal;
    filtroIniciado = true;
  } else {
    angFiltrado += ALPHA * (angCal - angFiltrado);
  }
  return angFiltrado;
}

void registrarPunto(float real) {
  if (calN >= 2) {
    Serial.println(F("Ya hay 2 puntos. Envia K para reiniciar"));
    return;
  }
  calRaw[calN]  = anguloCrudo();
  calReal[calN] = real;
  calN++;

  if (calN == 1) {
    calK = 1.0;
    calB = calReal[0] - calRaw[0];
  } else {
    float dr = calRaw[1] - calRaw[0];
    if (fabs(dr) < 1.0) {
      calN = 1;
      Serial.println(F("Puntos muy cercanos. Usa otra inclinacion"));
      return;
    }
    calK = (calReal[1] - calReal[0]) / dr;
    calB = calReal[0] - calK * calRaw[0];
  }
  filtroIniciado = false;

  Serial.print(F("Punto ")); Serial.print(calN);
  Serial.print(F(": medido=")); Serial.print(calRaw[calN - 1], 2);
  Serial.print(F(" real=")); Serial.println(calReal[calN - 1], 1);
}

void resetCalibracion() {
  calN = 0;
  calK = 1.0;
  calB = 0.0;
  filtroIniciado = false;
  Serial.println(F("Calibracion angular reiniciada"));
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
  float v[N_MEDIA];
  int ok = 0;
  for (int i = 0; i < N_MEDIA; i++) {
    long t = leerDist(trig, echo);
    if (t > 0) {
      float cm = t * 0.0343 / 2.0;
      if (cm >= 2.0 && cm <= 400.0) v[ok++] = cm;
    }
    delay(30);
  }
  if (ok == 0) return -1;

  for (int i = 0; i < ok - 1; i++)
    for (int j = 0; j < ok - 1 - i; j++)
      if (v[j] > v[j + 1]) { float t = v[j]; v[j] = v[j + 1]; v[j + 1] = t; }

  if (ok % 2) return v[ok / 2];
  return (v[ok / 2 - 1] + v[ok / 2]) / 2.0;
}

Metodo2 calcularMetodo2() {
  Metodo2 r = {0, 0, 0, 0, 0, 0, false};

  r.b = leerDistMedia(TRIG_BASE, ECHO_BASE);
  r.h = leerDistMedia(TRIG_ALT, ECHO_ALT);
  if (r.b <= 0 || r.h < 0) return r;

  r.H      = sqrt(r.b * r.b + r.h * r.h);
  r.theta  = atan2(r.h, r.b) * RAD_A_GRADOS;
  r.a      = g * r.h / r.H;
  r.F      = masa * r.a;
  r.valido = true;
  return r;
}

void mostrarRaw() {
  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  float thetaMPU = anguloDeLectura(ax, ay, az);

  Metodo2 m2 = calcularMetodo2();

  display.fillRect(0, 0, 96, 64, BLACK);
  display.setTextSize(1);

  display.setTextColor(MAGENTA);
  display.setCursor(0, 0);
  display.print(F("RAW SENSORES"));

  display.setTextColor(CYAN);
  display.setCursor(0, 8);
  display.print(F("ax:")); display.print(ax);
  display.setCursor(0, 16);
  display.print(F("ay:")); display.print(ay);
  display.setCursor(0, 24);
  display.print(F("az:")); display.print(az);

  display.setTextColor(GREEN);
  display.setCursor(0, 32);
  display.print(F("ang MPU:"));
  printGrados(thetaMPU, 1);

  display.setTextColor(YELLOW);
  display.setCursor(0, 40);
  display.print(F("b:")); display.print(m2.b, 1);
  display.print(F(" h:")); display.print(m2.h, 1);
  display.setCursor(0, 48);
  display.print(F("H:")); display.print(m2.H, 1);

  display.setTextColor(GREEN);
  display.setCursor(0, 56);
  display.print(F("ang US:"));
  if (m2.valido) printGrados(m2.theta, 1);
  else display.print(F("INVALIDO"));

  Serial.print(F("RAW ax=")); Serial.print(ax);
  Serial.print(F(" ay=")); Serial.print(ay);
  Serial.print(F(" az=")); Serial.print(az);
  Serial.print(F(" angMPU=")); Serial.print(thetaMPU, 1);
  Serial.print(F(" b=")); Serial.print(m2.b, 1);
  Serial.print(F(" h=")); Serial.print(m2.h, 1);
  Serial.print(F(" H=")); Serial.print(m2.H, 1);
  Serial.print(F(" valido=")); Serial.print(m2.valido ? 1 : 0);
  Serial.print(F(" angUS=")); Serial.println(m2.theta, 1);
}

void procesarComando(const char *s) {
  int n = strlen(s);
  if (n == 0) return;

  if (n == 1 && (s[0] == 'T' || s[0] == 't')) {
    modo = MODO_TEOREMA;
    Serial.println(F("Modo: TEOREMA"));
  } else if (n == 1 && (s[0] == 'N' || s[0] == 'n')) {
    modo = MODO_NORMAL;
    Serial.println(F("Modo: NORMAL"));
  } else if (n == 1 && (s[0] == 'R' || s[0] == 'r')) {
    modo = MODO_RAW;
    Serial.println(F("Modo: RAW"));
  } else if (n == 1 && (s[0] == 'K' || s[0] == 'k')) {
    resetCalibracion();
  } else if (n == 1 && (s[0] == 'C' || s[0] == 'c')) {
    calibrarMPU();
  } else if (s[0] == 'P' || s[0] == 'p') {
    float real = atof(s + 1);
    if (real > 0 && real < 90) {
      registrarPunto(real);
    } else {
      Serial.println(F("Usa P<grados>, ej. P30"));
    }
  } else {
    Serial.println(F("Comando no valido"));
  }
}

void leerComandos() {
  while (Serial.available()) {
    char c = Serial.read();
    if (cmdLen == 0 && strchr("TtNnRrKkCc", c) != NULL) {
      cmdBuf[0] = c;
      cmdBuf[1] = '\0';
      procesarComando(cmdBuf);
      continue;
    }
    if (c == '\n' || c == '\r') {
      if (cmdLen > 0) {
        cmdBuf[cmdLen] = '\0';
        procesarComando(cmdBuf);
        cmdLen = 0;
      }
    } else if (cmdLen < sizeof(cmdBuf) - 1) {
      cmdBuf[cmdLen++] = c;
    }
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println(F("Comandos: N normal, T teorema, R raw, K borrar cal angular, C recalibrar MPU"));
  Serial.println(F("P<grados> = punto medido"));

  pinMode(TRIG_BASE, OUTPUT);
  pinMode(ECHO_BASE, INPUT);
  pinMode(TRIG_ALT, OUTPUT);
  pinMode(ECHO_ALT, INPUT);

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
    display.setCursor(0, 28);
    display.print(F("MPU6050 FAIL!"));
    while (true);
  }
  mpu.setFullScaleGyroRange(MPU6050_GYRO_FS_500);

  EEPROM.get(0, cal);
  if (cal.magic == MAGIC) {
    aplicarOffsets();
    Serial.println(F("Calibracion MPU cargada de EEPROM"));
    display.setTextColor(GREEN);
    display.setCursor(0, 28);
    display.print(F("CALIBRADO"));
    delay(1000);
  } else {
    Serial.println(F("SIN calibracion MPU. Envia C para calibrar."));
    display.setTextColor(RED);
    display.setCursor(0, 28);
    display.print(F("ENVIAR C"));
    delay(2000);
  }
  display.fillScreen(BLACK);
}

void loop() {
  leerComandos();

  if (modo == MODO_RAW) {
    mostrarRaw();
    delay(100);
    return;
  }

  float thetaMPU = leerAngulo();

  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  float a_MPU = SIGNO_PENDIENTE * valorEje(EJE_PENDIENTE, ax, ay, az) / LSB_POR_G * g;
  float F_MPU = masa * a_MPU;

  Metodo2 m2 = calcularMetodo2();

  display.fillRect(0, 0, 96, 64, BLACK);
  display.setTextSize(1);

  if (modo == MODO_TEOREMA) {
    display.setTextColor(MAGENTA);
    display.setCursor(0, 0);
    display.print(F("PITAGORAS"));

    display.setTextColor(CYAN);
    display.setCursor(0, 8);
    display.print(F("b:")); display.print(m2.b, 1);
    display.print(F(" h:")); display.print(m2.h, 1);

    display.setCursor(0, 16);
    display.print(F("H:")); display.print(m2.H, 1);

    display.setTextColor(MAGENTA);
    display.setCursor(0, 32);
    display.print(F("RECONSTRUCCION A"));

    display.setTextColor(YELLOW);
    display.setCursor(0, 40);
    display.print(F("a=g*h/H"));

    display.setTextColor(WHITE);
    display.setCursor(0, 48);
    display.print(F("a:"));
    if (m2.valido) display.print(m2.a, 3);
    else display.print(F("---"));
    display.print(F(" m/s2"));

    display.setTextColor(GREEN);
    display.setCursor(0, 56);
    display.print(F("ang:"));
    if (m2.valido) printGrados(m2.theta, 1);
    else display.print(F("INVALIDO"));
  } else {
    display.setTextColor(CYAN);
    display.setCursor(0, 0);
    display.print(F("F=m*a (MPU)"));

    display.setCursor(0, 8);
    display.print(F("m:")); display.print(masa, 3);
    display.print(F("kg "));
    printGrados(thetaMPU, 1);

    display.setCursor(0, 16);
    display.print(F("a:")); display.print(a_MPU, 3);
    display.print(F(" m/s2"));

    display.setCursor(0, 24);
    display.print(F("F:")); display.print(F_MPU, 3);
    display.print(F(" N"));

    display.setTextColor(YELLOW);
    display.setCursor(0, 32);
    display.print(F("Cosenos+F=m*a"));

    display.setCursor(0, 40);
    display.print(F("m:")); display.print(masa, 3);
    display.print(F("kg "));
    if (m2.valido) printGrados(m2.theta, 1);
    else display.print(F("---"));

    display.setCursor(0, 48);
    display.print(F("a:"));
    if (m2.valido) display.print(m2.a, 3);
    else display.print(F("---"));
    display.print(F(" m/s2"));

    display.setCursor(0, 56);
    display.print(F("F:"));
    if (m2.valido) display.print(m2.F, 3);
    else display.print(F("---"));
    display.print(F(" N"));
  }

  delay(100);
}