from PIL import Image
import argparse
import time
import serial

SCREEN_W = 320
SCREEN_H = 240

AUTO_ROTATE_VERTICAL = True   # True = поворачивать вертикальные картинки
FIT_MODE = "contain"          # "contain" = с полями, "fill" = заполнить экран с возможной обрезкой


def prepare_image(path, screen_w, screen_h, auto_rotate=True, fit_mode="contain"):
    image = Image.open(path).convert("RGB")
    orig_w, orig_h = image.size

    # Опциональный поворот: только если картинка изначально вертикальная
    if auto_rotate and orig_h > orig_w:
        image = image.rotate(90, expand=True)

    if fit_mode == "contain":
        # Вписать целиком, сохранив пропорции, с чёрными полями
        image.thumbnail((screen_w, screen_h))
        canvas = Image.new("RGB", (screen_w, screen_h), (0, 0, 0))
        x0 = (screen_w - image.width) // 2
        y0 = (screen_h - image.height) // 2
        canvas.paste(image, (x0, y0))
        image = canvas

    elif fit_mode == "fill":
        # Заполнить весь экран, сохранив пропорции, лишнее обрезать
        img_ratio = image.width / image.height
        screen_ratio = screen_w / screen_h

        if img_ratio > screen_ratio:
            # картинка слишком широкая -> подгоняем по высоте
            new_h = screen_h
            new_w = int(image.width * screen_h / image.height)
        else:
            # картинка слишком высокая -> подгоняем по ширине
            new_w = screen_w
            new_h = int(image.height * screen_w / image.width)

        image = image.resize((new_w, new_h))

        left = (new_w - screen_w) // 2
        top = (new_h - screen_h) // 2
        right = left + screen_w
        bottom = top + screen_h

        image = image.crop((left, top, right, bottom))

    else:
        raise ValueError("fit_mode must be 'contain' or 'fill'")

    return image


def main():
    parser = argparse.ArgumentParser(description="Вывод картинки на дисплей ILI9341")
    parser.add_argument("image", help="путь к картинке")
    parser.add_argument("--port", default="COM15", help="COM-порт устройства (по умолчанию COM15)")
    args = parser.parse_args()

    image = prepare_image(
        args.image,
        SCREEN_W,
        SCREEN_H,
        auto_rotate=AUTO_ROTATE_VERTICAL,
        fit_mode=FIT_MODE
    )

    width, height = image.size

    ser = serial.Serial(args.port, 115200, timeout=1)
    time.sleep(2)

    if ser.is_open:
        print(f"Port {ser.name} opened")

    try:
        for y in range(height):
            for x in range(width):
                r, g, b = image.getpixel((x, y))
                color888 = (r << 16) | (g << 8) | b
                ser.write(f"disp_px {x} {y} 0x{color888:06X}\n".encode("ascii"))

    finally:
        ser.close()
        print("Port closed")


if __name__ == "__main__":
    main()