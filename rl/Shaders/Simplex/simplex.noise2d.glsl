float Noise2D(float x, float y) {
    float stretchOffset = (x + y) * STRETCH_2D;
    float xs = x + stretchOffset;
    float ys = y + stretchOffset;

    int xsb = int(floor(xs));
    int ysb = int(floor(ys));

    float squishOffset = float(xsb + ysb) * SQUISH_2D;
    float xb = float(xsb) + squishOffset;
    float yb = float(ysb) + squishOffset;

    float xins = xs - float(xsb);
    float yins = ys - float(ysb);
    float inSum = xins + yins;

    float dx0 = x - xb;
    float dy0 = y - yb;

    float value = 0.0;

    float dx1 = dx0 - 1.0 - SQUISH_2D;
    float dy1 = dy0 - SQUISH_2D;
    float attn1 = 2.0 - dot(vec2(dx1, dy1), vec2(dx1, dy1));
    value += max(0.0, attn1) * max(0.0, attn1) * Extrapolate2d(xsb + 1, ysb, dx1, dy1);

    float dx2 = dx0 - SQUISH_2D;
    float dy2 = dy0 - 1.0 - SQUISH_2D;
    float attn2 = 2.0 - dot(vec2(dx2, dy2), vec2(dx2, dy2));
    value += max(0.0, attn2) * max(0.0, attn2) * Extrapolate2d(xsb, ysb + 1, dx2, dy2);

    float zins = mix(1.0, 2.0, step(1.0, inSum)) - inSum;
    bool inLower = inSum <= 1.0;
    bool useCorner = inLower ? (zins > xins || zins > yins) : (zins < xins || zins < yins);
    bool xGreater = xins > yins;

    int xsv_ext = inLower ?
    (useCorner ? (xGreater ? xsb + 1 : xsb - 1) : xsb + 1) :
    (useCorner ? (xGreater ? xsb + 2 : xsb) : xsb);
    int ysv_ext = inLower ?
    (useCorner ? (xGreater ? ysb - 1 : ysb + 1) : ysb + 1) :
    (useCorner ? (xGreater ? ysb : ysb + 2) : ysb);

    float dx_ext = inLower ?
    (useCorner ? (xGreater ? dx0 - 1.0 : dx0 + 1.0) : dx0 - 1.0 - 2.0 * SQUISH_2D) :
    (useCorner ? (xGreater ? dx0 - 2.0 - 2.0 * SQUISH_2D : dx0 - 2.0 * SQUISH_2D) : dx0);
    float dy_ext = inLower ?
    (useCorner ? (xGreater ? dy0 + 1.0 : dy0 - 1.0) : dy0 - 1.0 - 2.0 * SQUISH_2D) :
    (useCorner ? (xGreater ? dy0 - 2.0 * SQUISH_2D : dy0 - 2.0 - 2.0 * SQUISH_2D) : dy0);

    // Adjust origin for upper region
    if (!inLower) {
        xsb += 1;
        ysb += 1;
        dx0 = dx0 - 1.0 - 2.0 * SQUISH_2D;
        dy0 = dy0 - 1.0 - 2.0 * SQUISH_2D;
    }

    // Contribution (0,0) or (1,1)
    float attn0 = 2.0 - dot(vec2(dx0, dy0), vec2(dx0, dy0));
    value += max(0.0, attn0) * max(0.0, attn0) * Extrapolate2d(xsb, ysb, dx0, dy0);

    // Extra Vertex
    float attn_ext = 2.0 - dot(vec2(dx_ext, dy_ext), vec2(dx_ext, dy_ext));
    value += max(0.0, attn_ext) * max(0.0, attn_ext) * Extrapolate2d(xsv_ext, ysv_ext, dx_ext, dy_ext);

    return value / NORM_2D;
}
