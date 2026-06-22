from PIL import Image, ImageDraw

img = Image.new('RGBA', (64, 64), (0, 0, 0, 0))
draw = ImageDraw.Draw(img)
draw.ellipse([12, 18, 52, 58], fill=(220, 30, 30), outline=(180, 20, 20))
draw.ellipse([26, 14, 38, 24], fill=(0, 0, 0, 0))
draw.line([(32, 16), (34, 8)], fill=(100, 60, 20), width=2)
draw.polygon([(34, 10), (44, 6), (38, 14)], fill=(40, 180, 40))
img.save('apple.ico', format='ICO', sizes=[(64, 64), (48, 48), (32, 32), (16, 16)])
