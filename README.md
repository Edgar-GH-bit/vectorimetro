# Vectorimetro: Segunda Ley de Newton en plano inclinado (Arduino UNO)

Compara dos metodos para obtener la aceleracion de un bloque en un plano inclinado y la fuerza con F = m * a:

1. **MPU6050**: angulo de inclinacion en grados y a = g * sen(theta).
2. **Ultrasonico**: Teorema de Pitagoras (base = sqrt(L^2 - h^2)), reconstruccion de a = g * h / L y F = m * a.

## Hardware
- Arduino UNO
- Pantalla OLED SSD1331 (SPI): CS=10, DC=9, RES=8, SCK=13, MOSI=11
- MPU6050 (I2C): SDA=A4, SCL=A5
- 2 sensores ultrasonicos: TRIG1=4, ECHO1=5, TRIG2=6, ECHO2=7

## Librerias
- Adafruit GFX Library
- Adafruit SSD1331 OLED Driver Library
- MPU6050 (Jeff Rowberg / i2cdevlib)

## Uso
1. Abre el monitor serial a 9600 baudios con "Nueva linea".
2. Mide el angulo del plano con un transportador y dejalo quieto.
3. Envia `A` + grados (ej: `A25`) para calibrar.

## Comandos serial
| Comando | Accion |
|---------|--------|
| `A25`   | Calibra con 25 grados (plano quieto) |
| `N`     | Pantalla principal |
| `T`     | Menu Pitagoras y reconstruccion de a |
| `R`     | Lecturas brutas de los sensores |
| `0.75`  | Cambia la masa (kg) |
