void matrixMultiplication(float* O, float* A, float* B) {
    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r) {
            O[c * 4 + r] = 0;
            for (int i = 0; i < 4; ++i)
                O[c * 4 + r] += A[i * 4 + r] * B[c * 4 + i];
        }
}

void matrixIdentity(float* O) {
    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r)
            O[c * 4 + r] = (float)(c == r);
}

void rotateX(float* O, float angle) {
    matrixIdentity(O);
    auto cosa = cos(angle);
    auto sina = sin(angle);
    O[5] = +cosa; O[6] = +sina;
    O[9] = -sina; O[10] = +cosa;
}

void rotateY(float* O, float angle) {
    matrixIdentity(O);
    auto cosa = cos(angle);
    auto sina = sin(angle);
    O[0] = +cosa; O[2] = +sina;
    O[8] = -sina; O[10] = +cosa;
}

void translate(float* O, float x, float y, float z) {
    matrixIdentity(O);
    O[12] = x; O[13] = y; O[14] = z;
}

void perspective(float* O, float aspect, float fovy, float n, float f) {
    float R = n * tan(fovy / 2);
    float L = -R;
    float T = R / aspect;
    float B = -T;
    matrixIdentity(O);
    O[0] = 2 * n / (R - L);
    O[5] = 2 * n / (T - B);
    O[8] = (R + L) / (R - L);
    O[9] = (T + B) / (T - B);
    O[10] = -(f + n) / (f - n);
    O[11] = -1;
    O[14] = -2 * n * f / (f - n);
    O[15] = 0;
}
