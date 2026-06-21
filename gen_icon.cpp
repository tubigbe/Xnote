#include <windows.h>
#include <cstdio>
#include <cstdlib>

#pragma pack(push, 1)
struct IcoHeader {
    WORD reserved;
    WORD type;
    WORD count;
};

struct IcoDirEntry {
    BYTE width;
    BYTE height;
    BYTE colors;
    BYTE reserved;
    WORD planes;
    WORD bitCount;
    DWORD imgSize;
    DWORD offset;
};

struct BmpHeader {
    DWORD biSize;
    LONG biWidth;
    LONG biHeight;
    WORD biPlanes;
    WORD biBitCount;
    DWORD biCompression;
    DWORD biSizeImage;
    LONG biXPelsPerMeter;
    LONG biYPelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
};
#pragma pack(pop)

int main() {
    const int size = 64;
    const int pixels = size * size;

    // Allocate pixel data (BGRA, bottom-up)
    unsigned char* pixels_data = (unsigned char*)calloc(pixels * 4, 1);
    unsigned char* and_mask = (unsigned char*)calloc(((size + 31) / 32 * 4) * size, 1);

    // Draw apple on pixel_data (BGRA format, bottom-up)
    auto setPixel = [&](int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255) {
        if (x < 0 || x >= size || y < 0 || y >= size) return;
        int idx = (y * size + x) * 4;
        pixels_data[idx + 0] = b;
        pixels_data[idx + 1] = g;
        pixels_data[idx + 2] = r;
        pixels_data[idx + 3] = a;
    };

    // Fill background transparent
    for (int i = 0; i < pixels; i++) {
        pixels_data[i * 4 + 3] = 0; // fully transparent
    }

    // Apple body (red) - ellipse centered at (32, 38), rx=20, ry=20
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            double dx = (x - 32) / 20.0;
            double dy = (y - 38) / 20.0;
            if (dx * dx + dy * dy <= 1.0) {
                // Add some shading
                double light = 1.0 - 0.3 * dy;
                int r = (int)(220 * light);
                int g = (int)(30 * light);
                int b = (int)(30 * light);
                if (r > 255) r = 255;
                setPixel(x, y, r, g, b, 255);
            }
        }
    }

    // Indent at top (transparent ellipse)
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            double dx = (x - 32) / 6.0;
            double dy = (y - 18) / 5.0;
            if (dx * dx + dy * dy <= 1.0) {
                setPixel(x, y, 0, 0, 0, 0);
            }
        }
    }

    // Stem (brown line)
    for (int i = 0; i < 10; i++) {
        int x = 32 + i / 5;
        int y = 16 - i;
        setPixel(x, y, 100, 60, 20, 255);
        setPixel(x + 1, y, 100, 60, 20, 255);
    }

    // Leaf (green triangle)
    auto inTriangle = [](int x, int y, int x1, int y1, int x2, int y2, int x3, int y3) {
        int d1 = (x - x2) * (y1 - y2) - (x1 - x2) * (y - y2);
        int d2 = (x - x3) * (y2 - y3) - (x2 - x3) * (y - y3);
        int d3 = (x - x1) * (y3 - y1) - (x3 - x1) * (y - y1);
        bool has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
        bool has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);
        return !(has_neg && has_pos);
    };

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            if (inTriangle(x, y, 34, 10, 44, 6, 38, 14)) {
                setPixel(x, y, 40, 180, 40, 255);
            }
        }
    }

    // Write ICO file
    FILE* f = fopen("apple.ico", "wb");
    if (!f) {
        printf("Failed to open apple.ico for writing\n");
        free(pixels_data);
        free(and_mask);
        return 1;
    }

    // Flip pixel data vertically (ICO expects bottom-up)
    unsigned char* flipped = (unsigned char*)malloc(pixels * 4);
    for (int y = 0; y < size; y++) {
        memcpy(flipped + y * size * 4, pixels_data + (size - 1 - y) * size * 4, size * 4);
    }

    IcoHeader header = {0, 1, 1};
    IcoDirEntry entry = {};
    entry.width = size;
    entry.height = size;
    entry.colors = 0;
    entry.reserved = 0;
    entry.planes = 1;
    entry.bitCount = 32;
    entry.offset = 6 + 16; // header + 1 dir entry

    DWORD xorSize = sizeof(BmpHeader) + pixels * 4;
    DWORD andRowBytes = (size + 31) / 32 * 4;
    DWORD andSize = andRowBytes * size;
    entry.imgSize = xorSize + andSize;

    BmpHeader bmp = {};
    bmp.biSize = sizeof(BmpHeader);
    bmp.biWidth = size;
    bmp.biHeight = size * 2; // height for XOR+AND combined
    bmp.biPlanes = 1;
    bmp.biBitCount = 32;
    bmp.biCompression = BI_RGB;
    bmp.biSizeImage = pixels * 4;

    fwrite(&header, sizeof(header), 1, f);
    fwrite(&entry, sizeof(entry), 1, f);
    fwrite(&bmp, sizeof(bmp), 1, f);
    fwrite(flipped, pixels * 4, 1, f);
    // AND mask (all 0 = opaque where alpha is set)
    fwrite(and_mask, andSize, 1, f);

    fclose(f);
    free(pixels_data);
    free(flipped);
    free(and_mask);

    printf("Created apple.ico successfully\n");
    return 0;
}
