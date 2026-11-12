Vectorímetro digital para plano inclinado
Aparato didáctico que muestra cómo una fuerza sobre un plano inclinado se descompone en sus componentes. Calcula la aceleración y la fuerza por dos métodos independientes y permite compararlos.
Descripción
El sistema calcula la aceleración de un bloque sobre una rampa y la fuerza con la segunda ley de Newton (F = m·a), usando dos caminos:
Método 1 (MPU6050): mide el ángulo θ del plano a partir de la gravedad. Con él, a = g·sen(θ).
Método 2 (ultrasónicos): mide una distancia base fija (b) y la altura variable (h) del extremo de la rampa. Con la hipotenusa H = √(b² + h²) se obtiene θ = atan2(h, b) y a = g·h/H.
En ambos casos, F = m·a con la masa fija del equipo.
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
Abre el Monitor Serial a 9600 baudios y selecciona "Nueva línea".
Coloca la rampa en su punto más bajo (aprox. 30°) y déjala quieta.
Envía C para calibrar el MPU6050. El resultado se guarda en la EEPROM.
Envía K para borrar la calibración angular anterior.
Mide tres ángulos con un transportador y envía P<grados> para cada uno (por ejemplo P30, P40, P45). Dos puntos bastan para la calibración angular.
Envía N para la pantalla principal o R para ver las lecturas brutas.
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
Logo (opcional)
Para mostrar un logo al encender, genera logo_array.h a partir de una imagen de 96×64 píxeles con un script de conversión a monocromo, y colócalo en la misma carpeta que vectorimetro.ino. Para desactivarlo, cambia USAR_LOGO a 0.
Limitaciones
Con la rampa quieta, el MPU6050 mide la gravedad. La "aceleración" del método 1 es g·sen(θ), no un movimiento real.
La masa es fija en el código (masa = 0.5 kg). Cámbiala en el código si el bloque pesa otra cosa.
Si una lectura ultrasónica no es válida (fuera de 2 a 400 cm o sin eco), la pantalla muestra --- o INVALIDO.
Licencia
Proyecto académico. Ajusta esta sección según tu institución o el uso que le des.