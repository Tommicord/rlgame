float Noise2D(float x, float y) {
    // Place input coordinates onto grid
    float stretchOffset = (x + y) * STRETCH_2D;
    float xs = x + stretchOffset;
    float ys = y + stretchOffset;

    // Floor to get grid coordinates of rhombus (stretched square) super-cell origin
    int xsb = int(floor(xs));
    int ysb = int(floor(ys));

    // Skew out to get actual coordinates of rhombus origin
    float squishOffset = float(xsb + ysb) * SQUISH_2D;
    float xb = float(xsb) + squishOffset;
    float yb = float(ysb) + squishOffset;

    // Compute grid coordinates relative to rhombus origin
    float xins = xs - float(xsb);
    float yins = ys - float(ysb);

    // Sum those together to get a value that determines which region we're in
    float inSum = xins + yins;

    // Positions relative to origin point
    float dx0 = x - xb;
    float dy0 = y - yb;

    // We'll be defining these inside the next block and using them afterwards
    float dx_ext, dy_ext;
    int xsv_ext, ysv_ext;

    float value = 0.0;

    // Contribution (1,0)
    float dx1 = dx0 - 1.0 - SQUISH_2D;
    float dy1 = dy0 - 0.0 - SQUISH_2D;
    float attn1 = 2.0 - dx1 * dx1 - dy1 * dy1;
    if (attn1 > 0.0) {
        attn1 *= attn1;
        value += attn1 * attn1 * Extrapolate2d(xsb + 1, ysb + 0, dx1, dy1);
    }

    // Contribution (0,1)
    float dx2 = dx0 - 0.0 - SQUISH_2D;
    float dy2 = dy0 - 1.0 - SQUISH_2D;
    float attn2 = 2.0 - dx2 * dx2 - dy2 * dy2;
    if (attn2 > 0.0) {
        attn2 *= attn2;
        value += attn2 * attn2 * Extrapolate2d(xsb + 0, ysb + 1, dx2, dy2);
    }

    if (inSum <= 1.0) {
        // We're inside the triangle (2-Simplex) at (0,0)
        float zins = 1.0 - inSum;
        if (zins > xins || zins > yins) {
            // (0,0) is one of the closest two triangular vertices
            if (xins > yins) {
                xsv_ext = xsb + 1;
                ysv_ext = ysb - 1;
                dx_ext = dx0 - 1.0;
                dy_ext = dy0 + 1.0;
            } else {
                xsv_ext = xsb - 1;
                ysv_ext = ysb + 1;
                dx_ext = dx0 + 1.0;
                dy_ext = dy0 - 1.0;
            }
        } else {
            // (1,0) and (0,1) are the closest two vertices
            xsv_ext = xsb + 1;
            ysv_ext = ysb + 1;
            dx_ext = dx0 - 1.0 - 2.0 * SQUISH_2D;
            dy_ext = dy0 - 1.0 - 2.0 * SQUISH_2D;
        }
    } else {
        // We're inside the triangle (2-Simplex) at (1,1)
        float zins = 2.0 - inSum;
        if (zins < xins || zins < yins) {
            // (0,0) is one of the closest two triangular vertices
            if (xins > yins) {
                xsv_ext = xsb + 2;
                ysv_ext = ysb + 0;
                dx_ext = dx0 - 2.0 - 2.0 * SQUISH_2D;
                dy_ext = dy0 + 0.0 - 2.0 * SQUISH_2D;
            } else {
                xsv_ext = xsb + 0;
                ysv_ext = ysb + 2;
                dx_ext = dx0 + 0.0 - 2.0 * SQUISH_2D;
                dy_ext = dy0 - 2.0 - 2.0 * SQUISH_2D;
            }
        } else {
            // (1,0) and (0,1) are the closest two vertices
            dx_ext = dx0;
            dy_ext = dy0;
            xsv_ext = xsb;
            ysv_ext = ysb;
        }
        xsb += 1;
        ysb += 1;
        dx0 = dx0 - 1.0 - 2.0 * SQUISH_2D;
        dy0 = dy0 - 1.0 - 2.0 * SQUISH_2D;
    }

    // Contribution (0,0) or (1,1)
    float attn0 = 2.0 - dx0 * dx0 - dy0 * dy0;
    if (attn0 > 0.0) {
        attn0 *= attn0;
        value += attn0 * attn0 * Extrapolate2d(xsb, ysb, dx0, dy0);
    }

    // Extra Vertex
    float attn_ext = 2.0 - dx_ext * dx_ext - dy_ext * dy_ext;
    if (attn_ext > 0.0) {
        attn_ext *= attn_ext;
        value += attn_ext * attn_ext * Extrapolate2d(xsv_ext, ysv_ext, dx_ext, dy_ext);
    }

    return value / NORM_2D;
}
