layout(std430, binding = 0) buffer PermutationBuffer {
    int perm[256];
};

// 3D gradient index permutation (256 values)
layout(std430, binding = 1) buffer PermGradIndex3DBuffer {
    int permGradIndex3D[256];
};

int GetPerm(int index) {
    return perm[index & 255];
}

int GetPermGradIndex3D(int index) {
    return permGradIndex3D[index & 255];
}
