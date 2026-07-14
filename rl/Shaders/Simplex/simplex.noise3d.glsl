float Noise3D(float x, float y, float z) {
    vec3 P = vec3(x, y, z);

    vec3 Pi = floor(P + dot(P, vec3(SKEW_3D)));
    vec3 P0 = P - Pi + dot(Pi, vec3(UNSKEW_3D));

    vec3 g = step(P0.yzx, P0.xyz);
    vec3 l = 1.0 - g;
    vec3 i1 = min(g.xyz, l.zxy);
    vec3 i2 = max(g.xyz, l.zxy);

    vec3 P1 = P0 - i1 + UNSKEW_3D;
    vec3 P2 = P0 - i2 + 2.0 * UNSKEW_3D;
    vec3 P3 = P0 - 1.0 + 3.0 * UNSKEW_3D;

    int ix = int(Pi.x) & 255;
    int iy = int(Pi.y) & 255;
    int iz = int(Pi.z) & 255;

    int i1x = int(i1.x); int i1y = int(i1.y); int i1z = int(i1.z);
    int i2x = int(i2.x); int i2y = int(i2.y); int i2z = int(i2.z);
    
    float t0 = 0.6 - dot(P0, P0);
    float t1 = 0.6 - dot(P1, P1);
    float t2 = 0.6 - dot(P2, P2);
    float t3 = 0.6 - dot(P3, P3);
    
    float n0 = max(0.0, t0) * max(0.0, t0) * Extrapolate3d(ix, iy, iz, P0.x, P0.y, P0.z);
    float n1 = max(0.0, t1) * max(0.0, t1) * Extrapolate3d(ix + i1x, iy + i1y, iz + i1z, P1.x, P1.y, P1.z);
    float n2 = max(0.0, t2) * max(0.0, t2) * Extrapolate3d(ix + i2x, iy + i2y, iz + i2z, P2.x, P2.y, P2.z);
    float n3 = max(0.0, t3) * max(0.0, t3) * Extrapolate3d(ix + 1, iy + 1, iz + 1, P3.x, P3.y, P3.z);
    
    return 32.0 * (n0 + n1 + n2 + n3);
}