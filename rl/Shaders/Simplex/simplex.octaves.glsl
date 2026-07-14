// Fractal Brownian Motion (FBM), Multiple octaves of noise for detail

// 2D Fractal Brownian Motion
float FBM2D(float x, float y, int octaves, float persistence, float lacunarity) {
    float total = 0.0;
    float frequency = 1.0;
    float amplitude = 1.0;
    float maxValue = 0.0;  // Used for normalizing result to 0.0 - 1.0

    int numOctaves = clamp(int(octaves), 1, 6);
    for(int i = 0; i < numOctaves; i++) {
        total += Noise2D(x * frequency, y * frequency) * amplitude;
        
        maxValue += amplitude;
        
        amplitude *= persistence;
        frequency *= lacunarity;
    }
    
    return total / maxValue;
}

// 3D Fractal Brownian Motion
float FBM3D(float x, float y, float z, int octaves, float persistence, float lacunarity) {
    float total = 0.0;
    float frequency = 1.0;
    float amplitude = 1.0;
    float maxValue = 0.0;  // Used for normalizing result to 0.0 - 1.0
    
    int numOctaves = clamp(int(octaves), 1, 6);
    for(int i = 0; i < numOctaves; i++) {
        total += Noise3D(x * frequency, y * frequency, z * frequency) * amplitude;
        
        maxValue += amplitude;
        
        amplitude *= persistence;
        frequency *= lacunarity;
    }
    
    return total / maxValue;
}

// Ridged multifractal noise (creates sharp, ridge-like features)
float RidgedNoise2D(float x, float y, int octaves, float persistence, float lacunarity) {
    float total = 0.0;
    float frequency = 1.0;
    float amplitude = 1.0;
    float maxValue = 0.0;
    
    int numOctaves = clamp(int(octaves), 1, 6);
    for(int i = 0; i < numOctaves; i++) {
        float signal = Noise2D(x * frequency, y * frequency);
        signal = abs(signal);  // Take absolute value
        signal = 1.0 - signal;  // Invert to create ridges
        signal = signal * signal;  // Square for sharper ridges
        
        total += signal * amplitude;
        maxValue += amplitude;
        
        amplitude *= persistence;
        frequency *= lacunarity;
    }
    
    return total / maxValue;
}

// Ridged multifractal noise 3D
float RidgedNoise3D(float x, float y, float z, int octaves, float persistence, float lacunarity) {
    float total = 0.0;
    float frequency = 1.0;
    float amplitude = 1.0;
    float maxValue = 0.0;
    
    int numOctaves = clamp(int(octaves), 1, 6);
    for(int i = 0; i < numOctaves; i++) {
        float signal = Noise3D(x * frequency, y * frequency, z * frequency);
        signal = abs(signal);  // Take absolute value
        signal = 1.0 - signal;  // Invert to create ridges
        signal = signal * signal;  // Square for sharper ridges
        
        total += signal * amplitude;
        maxValue += amplitude;
        
        amplitude *= persistence;
        frequency *= lacunarity;
    }
    
    return total / maxValue;
}

// Turbulence function (domain warping)
float Turbulence2D(float x, float y, int octaves, float persistence, float lacunarity) {
    float total = 0.0;
    float frequency = 1.0;
    float amplitude = 1.0;
    float maxValue = 0.0;

    int numOctaves = clamp(int(octaves), 1, 6);
    for(int i = 0; i < numOctaves; i++) {
        float signal = abs(Noise2D(x * frequency, y * frequency));
        total += signal * amplitude;
        maxValue += amplitude;
        
        amplitude *= persistence;
        frequency *= lacunarity;
    }
    
    return total / maxValue;
}

// Turbulence function 3D
float Turbulence3D(float x, float y, float z, int octaves, float persistence, float lacunarity) {
    float total = 0.0;
    float frequency = 1.0;
    float amplitude = 1.0;
    float maxValue = 0.0;

    int numOctaves = clamp(int(octaves), 1, 6);
    for(int i = 0; i < numOctaves; i++) {
        float signal = abs(Noise3D(x * frequency, y * frequency, z * frequency));
        total += signal * amplitude;
        maxValue += amplitude;
        
        amplitude *= persistence;
        frequency *= lacunarity;
    }
    
    return total / maxValue;
}
