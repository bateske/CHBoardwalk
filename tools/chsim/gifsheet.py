"""Tile frames of a GIF into one image, to look through an animation at a glance.

    python gifsheet.py in.gif out.png [first] [step] [count] [cols]
"""
import sys

from PIL import Image


def main():
    src, dst = sys.argv[1], sys.argv[2]
    first = int(sys.argv[3]) if len(sys.argv) > 3 else 0
    step = int(sys.argv[4]) if len(sys.argv) > 4 else 1
    count = int(sys.argv[5]) if len(sys.argv) > 5 else 16
    cols = int(sys.argv[6]) if len(sys.argv) > 6 else 8
    im = Image.open(src)
    frames = []
    for i in range(first, min(first + step * count, im.n_frames), step):
        im.seek(i)
        frames.append(im.convert("RGB"))
    w, h = frames[0].size
    rows = (len(frames) + cols - 1) // cols
    sheet = Image.new("RGB", (cols * (w + 4) + 4, rows * (h + 4) + 4), (40, 40, 40))
    for k, f in enumerate(frames):
        sheet.paste(f, (4 + (k % cols) * (w + 4), 4 + (k // cols) * (h + 4)))
    sheet.save(dst)
    print(f"{dst}: frames {first}..{first + step * (len(frames) - 1)} of {im.n_frames}")


if __name__ == "__main__":
    main()
