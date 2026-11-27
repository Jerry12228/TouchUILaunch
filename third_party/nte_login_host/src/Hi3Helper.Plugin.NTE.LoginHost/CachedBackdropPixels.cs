namespace Nte.LoginHost;

// Three separable sliding box passes approximate a soft Gaussian, in O(pixels)
// time independent of radius. The final noise is baked into this same bitmap.
internal static class CachedBackdropPixels
{
    internal static void BlurAndGrain(byte[] pixels, int width, int height, int radius)
    {
        if (width < 1 || height < 1 || pixels.Length != checked(width * height * 4) || radius < 1 || radius > 64)
            throw new ArgumentException("Invalid backdrop dimensions.");
        byte[] temporary = new byte[pixels.Length];
        for (int pass = 0; pass < 3; pass++)
        { Box(pixels, temporary, width, height, radius, horizontal: true); Box(temporary, pixels, width, height, radius, horizontal: false); }
        Array.Clear(temporary);
        uint random = 0xA51C71Cu;
        for (int offset = 0; offset < pixels.Length; offset += 4)
        {
            random ^= random << 13; random ^= random >> 17; random ^= random << 5;
            int grain = (int)(random % 5) - 2;
            for (int channel = 0; channel < 3; channel++) pixels[offset + channel] = (byte)Math.Clamp(pixels[offset + channel] + grain, 0, 255);
            pixels[offset + 3] = 255;
        }
    }

    private static void Box(byte[] source, byte[] destination, int width, int height, int radius, bool horizontal)
    {
        int lines = horizontal ? height : width, length = horizontal ? width : height;
        int increment = horizontal ? 4 : width * 4, divisor = 2 * radius + 1;
        for (int line = 0; line < lines; line++)
        {
            int start = horizontal ? line * width * 4 : line * 4;
            for (int channel = 0; channel < 3; channel++)
            {
                int sum = 0;
                for (int sample = -radius; sample <= radius; sample++) sum += source[start + Math.Clamp(sample, 0, length - 1) * increment + channel];
                for (int position = 0; position < length; position++)
                {
                    destination[start + position * increment + channel] = (byte)((sum + divisor / 2) / divisor);
                    sum += source[start + Math.Min(length - 1, position + radius + 1) * increment + channel]
                         - source[start + Math.Max(0, position - radius) * increment + channel];
                }
            }
        }
    }
}
