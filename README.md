Vectorímetro: Segunda Ley de Newton en plano inclinado (Arduino UNO)
Compara dos métodos para obtener la aceleración de un bloque sobre un plano inclinado y la fuerza con F = m·a.
MPU6050: ángulo de inclinación θ del plano y a = g·sen(θ).
Ultrasónicos: distancia base fija y altura variable, hipotenusa H = √(b² + h²), ángulo θ = atan2(h, b) y a = g·h/H. Luego F = m·a.
Hardware
Arduino UNO
Pantalla OLED SSD1331 (SPI): CS=10, DC=9, RES=8, SCK=13, MOSI=11
MPU6050 (I2C): SDA=A4, SCL=A5
Ultrasónico de la base (fija): TRIG=6, ECHO=7
Ultrasónico de la altura (variable): TRIG=4, ECHO=5
Librerías
Adafruit GFX Library
Adafruit SSD1331 OLED Driver Library
MPU6050 (Electronic Cats)
Uso
Abre el Monitor Serial a 9600 baudios con "Nueva línea".
Pon la rampa en su punto más bajo (30°, aproximado) y déjala quieta.
Envía C para calibrar el MPU6050. Se guarda en la EEPROM.
Envía K para borrar la calibración angular anterior.
Mide tres ángulos con transportador y envía P<grados> para cada uno (por ejemplo P30, P40, P45). Dos puntos bastan para la calibración angular.
Envía N para ver la pantalla principal o R para ver las lecturas brutas.
La calibración angular (P) se pierde al reiniciar el Arduino. La del MPU (C) se conserva en la EEPROM.
Comandos serial
Comando
Acción
C
Calibra el MPU6050 con la rampa en su punto más bajo (30°)
K
Borra la calibración angular
P<grados>
Registra un punto medido con transportador (ej. P30)
N
Pantalla principal (método 1 y método 2)
T
Menú Pitágoras y reconstrucción de a
R
Lecturas brutas de los sensores
Notas
Con la rampa quieta, el MPU6050 mide la gravedad. La "aceleración" del método 1 es g·sen(θ), no un movimiento real.
La masa es fija en el código (masa = 0.5 kg). Cámbiala en el código si tu bloque pesa otra cosa.
Si una lectura ultrasónica no es válida (fuera de 2 a 400 cm o sin eco), la pantalla muestra --- o INVALIDO.