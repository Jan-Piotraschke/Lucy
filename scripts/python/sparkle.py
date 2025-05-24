from PIL import Image, ImageDraw, ImageFilter

# Create a transparent image
size = (32, 32)
image = Image.new("RGBA", size, (0, 0, 0, 0))
draw = ImageDraw.Draw(image)

# Define golden color (RGB: 255, 215, 0)
gold = (255, 215, 0, 255)
soft_gold = (255, 215, 0, 180)

# Center point
center = (size[0] // 2, size[1] // 2)

# Draw a sparkle using lines (like a + and x cross)
line_length = 10
line_width = 2
draw.line((center[0] - line_length, center[1], center[0] + line_length, center[1]), fill=gold, width=line_width)
draw.line((center[0], center[1] - line_length, center[0], center[1] + line_length), fill=gold, width=line_width)
draw.line((center[0] - line_length, center[1] - line_length, center[0] + line_length, center[1] + line_length), fill=soft_gold, width=1)
draw.line((center[0] - line_length, center[1] + line_length, center[0] + line_length, center[1] - line_length), fill=soft_gold, width=1)

# Optional: add a glow effect
glow = image.filter(ImageFilter.GaussianBlur(radius=2))
final_image = Image.alpha_composite(glow, image)

# Save the image
output_path = "sparkle.png"
final_image.save(output_path)
