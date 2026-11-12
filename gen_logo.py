from PIL import Image, ImageOps

img = Image.open("logo.jpeg").convert("L").resize((96, 64), Image.LANCZOS)
img = ImageOps.autocontrast(img)

UMBRAL = 100
img = img.point(lambda p: 255 if p > UMBRAL else 0)

width = 96
height = 64
row_bytes = width // 8

data = []
for y in range(height):
    for x_byte in range(row_bytes):
        byte = 0
        for bit in range(8):
            x = x_byte * 8 + bit
            if img.getpixel((x, y)) == 255:
                byte |= (1 << (7 - bit))
        data.append(byte)

with open("logo_array.h", "w") as f:
    f.write("const uint8_t logo[] PROGMEM = {\n")
    for i, b in enumerate(data):
        f.write(f"0x{b:02X},")
        if (i + 1) % 12 == 0:
            f.write("\n")
    f.write("};\n")

print(f"logo_array.h generado: {len(data)} bytes")