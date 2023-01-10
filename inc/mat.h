/*
    October 2021

    Requires vec.h: https://gist.github.com/mrbid/77a92019e1ab8b86109bf103166bd04e

    Credits:
    Aaftab Munshi, Dan Ginsburg, Dave Shreiner, James William Fletcher, Intel, Gabriel Cramer
*/

#ifndef MAT_H
#define MAT_H

#include "vec.h"

typedef struct
{
    float m[4][4];
} mat;

void mIdent(mat *m);
void mCopy(mat *restrict r, const mat *restrict v);
void mMul(mat *restrict r, const mat *restrict a, const mat *restrict b);
void mMulP(vec *restrict r, const mat *restrict a, const float x, const float y, const float z);
void mMulV(vec *restrict r, const mat *restrict a, const vec v);
void mScale(mat *r, const float x, const float y, const float z);
void mTranslate(mat *r, const float x, const float y, const float z);
void mRotate(mat *r, const float radians, float x, float y, float z);
void mRotX(mat *r, const float radians);
void mRotY(mat *r, const float radians);
void mRotZ(mat *r, const float radians);
void mFrustum(mat *r, const float left, const float right, const float bottom, const float top, const float nearZ, const float farZ);
void mPerspective(mat *r, const float fovy, const float aspect, const float nearZ, const float farZ);
void mOrtho(mat *r, const float left, const float right, const float bottom, const float top, const float nearZ, const float farZ);
void mLookAt(mat *r, const vec origin, const vec unit_dir);
void mInvert(float *restrict dst, const float *restrict mat);
void mTranspose(mat *restrict r, const mat *restrict m);
void mSetViewDir(mat *r, const vec dir_norm, const vec up_norm);
void mGetViewDir(vec *r, const mat matrix); // returns normal/unit vector
void mGetDirX(vec *r, const mat matrix);
void mGetDirY(vec *r, const mat matrix);
void mGetDirZ(vec *r, const mat matrix);
void mGetPos(vec *r, const mat matrix);

//

void mIdent(mat *m)
{
    memset(m, 0x0, sizeof(mat));
    m->m[0][0] = 1.0f;
    m->m[1][1] = 1.0f;
    m->m[2][2] = 1.0f;
    m->m[3][3] = 1.0f;
}

void mCopy(mat *restrict r, const mat *restrict v)
{
    memcpy(r, v, sizeof(mat));
}

void mMul(mat *restrict r, const mat *restrict a, const mat *restrict b)
{
    mat tmp;
    for(int i = 0; i < 4; i++)
    {
        tmp.m[i][0] =	(a->m[i][0] * b->m[0][0]) +
                        (a->m[i][1] * b->m[1][0]) +
                        (a->m[i][2] * b->m[2][0]) +
                        (a->m[i][3] * b->m[3][0]) ;

        tmp.m[i][1] =	(a->m[i][0] * b->m[0][1]) + 
                        (a->m[i][1] * b->m[1][1]) +
                        (a->m[i][2] * b->m[2][1]) +
                        (a->m[i][3] * b->m[3][1]) ;

        tmp.m[i][2] =	(a->m[i][0] * b->m[0][2]) + 
                        (a->m[i][1] * b->m[1][2]) +
                        (a->m[i][2] * b->m[2][2]) +
                        (a->m[i][3] * b->m[3][2]) ;

        tmp.m[i][3] =	(a->m[i][0] * b->m[0][3]) + 
                        (a->m[i][1] * b->m[1][3]) +
                        (a->m[i][2] * b->m[2][3]) +
                        (a->m[i][3] * b->m[3][3]) ;
    }
    memcpy(r, &tmp, sizeof(mat));
}

void mMulP(vec *restrict r, const mat *restrict a, const float x, const float y, const float z)
{
    r->x =  (a->m[0][0] * x) +
            (a->m[0][1] * x) +
            (a->m[0][2] * x) +
            (a->m[0][3] * x) ;

    r->y =  (a->m[1][0] * y) +
            (a->m[1][1] * y) +
            (a->m[1][2] * y) +
            (a->m[1][3] * y) ;

    r->z =  (a->m[2][0] * z) +
            (a->m[2][1] * z) +
            (a->m[2][2] * z) +
            (a->m[2][3] * z) ;
}

void mMulV(vec *restrict r, const mat *restrict a, const vec v)
{
    r->x =  (a->m[0][0] * v.x) +
            (a->m[0][1] * v.x) +
            (a->m[0][2] * v.x) +
            (a->m[0][3] * v.x) ;

    r->y =  (a->m[1][0] * v.y) +
            (a->m[1][1] * v.y) +
            (a->m[1][2] * v.y) +
            (a->m[1][3] * v.y) ;

    r->z =  (a->m[2][0] * v.z) +
            (a->m[2][1] * v.z) +
            (a->m[2][2] * v.z) +
            (a->m[2][3] * v.z) ;

    r->w =  (a->m[3][0] * v.w) +
            (a->m[3][1] * v.w) +
            (a->m[3][2] * v.w) +
            (a->m[3][3] * v.w) ;
}

void mScale(mat *r, const float x, const float y, const float z)
{
    r->m[0][0] *= x;
    r->m[0][1] *= x;
    r->m[0][2] *= x;
    r->m[0][3] *= x;

    r->m[1][0] *= y;
    r->m[1][1] *= y;
    r->m[1][2] *= y;
    r->m[1][3] *= y;

    r->m[2][0] *= z;
    r->m[2][1] *= z;
    r->m[2][2] *= z;
    r->m[2][3] *= z;
}

void mTranslate(mat *r, const float x, const float y, const float z)
{
    r->m[3][0] += (r->m[0][0] * x + r->m[1][0] * y + r->m[2][0] * z);
    r->m[3][1] += (r->m[0][1] * x + r->m[1][1] * y + r->m[2][1] * z);
    r->m[3][2] += (r->m[0][2] * x + r->m[1][2] * y + r->m[2][2] * z);
    r->m[3][3] += (r->m[0][3] * x + r->m[1][3] * y + r->m[2][3] * z);
}

void mRotate(mat *r, const float radians, float x, float y, float z)
{
    const float mag = rsqrtss(x * x + y * y + z * z);
    const float sinAngle = sinf(radians);
    const float cosAngle = cosf(radians);
    if(mag > 0.0f)
    {
        x *= mag;
        y *= mag;
        z *= mag;

        const float xx = x * x;
        const float yy = y * y;
        const float zz = z * z;
        const float xy = x * y;
        const float yz = y * z;
        const float zx = z * x;
        const float xs = x * sinAngle;
        const float ys = y * sinAngle;
        const float zs = z * sinAngle;
        const float oneMinusCos = 1.0f - cosAngle;

        mat rotMat;
        rotMat.m[0][0] = (oneMinusCos * xx) + cosAngle;
        rotMat.m[0][1] = (oneMinusCos * xy) - zs;
        rotMat.m[0][2] = (oneMinusCos * zx) + ys;
        rotMat.m[0][3] = 0.0F; 

        rotMat.m[1][0] = (oneMinusCos * xy) + zs;
        rotMat.m[1][1] = (oneMinusCos * yy) + cosAngle;
        rotMat.m[1][2] = (oneMinusCos * yz) - xs;
        rotMat.m[1][3] = 0.0F;

        rotMat.m[2][0] = (oneMinusCos * zx) - ys;
        rotMat.m[2][1] = (oneMinusCos * yz) + xs;
        rotMat.m[2][2] = (oneMinusCos * zz) + cosAngle;
        rotMat.m[2][3] = 0.0F; 

        rotMat.m[3][0] = 0.0F;
        rotMat.m[3][1] = 0.0F;
        rotMat.m[3][2] = 0.0F;
        rotMat.m[3][3] = 1.0F;

        mMul(r, &rotMat, r);
    }
}

void mRotX(mat *r, const float radians)
{
    const float s = sinf(radians);
    const float c = cosf(radians);
    const mat t = { c, 0.f, s, 0.f,
                    0.f, 1.f, 0.f, 0.f,
                    -s, 0.f, c, 0.f,
                    0.f, 0.f, 0.f, 1.f };
    mMul(r, &t, r);
}

void mRotY(mat *r, const float radians)
{
    const float s = sinf(radians);
    const float c = cosf(radians);
    const mat t = { 1.f, 0.f, 0.f, 0.f,
                    0.f, c, -s, 0.f,
                    0.f, s, c, 0.f,
                    0.f, 0.f, 0.f, 1.f };
    mMul(r, &t, r);
}

void mRotZ(mat *r, const float radians)
{
    const float s = sinf(radians);
    const float c = cosf(radians);
    const mat t = { c, -s, 0.f, 0.f,
                    s, c, 0.f, 0.f,
                    0.f, 0.f, 1.f, 0.f,
                    0.f, 0.f, 0.f, 1.f };
    mMul(r, &t, r);
}

void mFrustum(mat *r, const float left, const float right, const float bottom, const float top, const float nearZ, const float farZ)
{
    const float dX = right - left;
    const float dY = top - bottom;
    const float dZ = farZ - nearZ;
    const float rdX = 1.f/dX;
    const float rdY = 1.f/dY;
    const float rdZ = 1.f/dZ;
    mat frust;

    if(nearZ <= 0.0f || farZ <= 0.0f || dX <= 0.0f || dY <= 0.0f || dZ <= 0.0f)
        return;

    frust.m[0][0] = 2.0f * nearZ * rdX;
    frust.m[0][1] = frust.m[0][2] = frust.m[0][3] = 0.0f;

    frust.m[1][1] = 2.0f * nearZ * rdY;
    frust.m[1][0] = frust.m[1][2] = frust.m[1][3] = 0.0f;

    frust.m[2][0] = (right + left) * rdX;
    frust.m[2][1] = (top + bottom) * rdY;
    frust.m[2][2] = -(nearZ + farZ) * rdZ;
    frust.m[2][3] = -1.0f;

    frust.m[3][2] = -2.0f * nearZ * farZ * rdZ;
    frust.m[3][0] = frust.m[3][1] = frust.m[3][3] = 0.0f;

    mMul(r, &frust, r);
}

void mPerspective(mat *r, const float fovy, const float aspect, const float nearZ, const float farZ)
{
   float frustumW, frustumH;
   frustumH = tanf(fovy * 0.002777778078f * PI ) * nearZ; // 0x3b360b62
   frustumW = frustumH * aspect;
   mFrustum(r, -frustumW, frustumW, -frustumH, frustumH, nearZ, farZ);
}

void mOrtho(mat *r, const float left, const float right, const float bottom, const float top, const float nearZ, const float farZ)
{
    const float dX = right - left;
    const float dY = top - bottom;
    const float dZ = farZ - nearZ;
    const float rdX = 1.f/dX;
    const float rdY = 1.f/dY;
    const float rdZ = 1.f/dZ;
    mat ortho;

    if(dX == 0.0f || dY == 0.0f || dZ == 0.0f)
        return;

    mIdent(&ortho);
    ortho.m[0][0] = 2.0f * rdX;
    ortho.m[3][0] = -(right + left) * rdX;
    ortho.m[1][1] = 2.0f * rdY;
    ortho.m[3][1] = -(top + bottom) * rdY;
    ortho.m[2][2] = -2.0f * rdZ;
    ortho.m[3][2] = -(nearZ + farZ) * rdZ;

    mMul(r, &ortho, r);
}

void mLookAt(mat *r, const vec origin, const vec unit_dir)
{
    vec up;
    up.x = 0.f, up.y = 1.f, up.z = 0.f;

    vec dirn;
    dirn.x = unit_dir.x;
    dirn.y = unit_dir.y;
    dirn.z = unit_dir.z;

    vec c;
    vCross(&c, up, dirn);
    vNorm(&c);

    vec rup;
    vCross(&rup, dirn, c);

    r->m[0][0] = c.x;
    r->m[0][1] = c.y;
    r->m[0][2] = c.z;

    r->m[1][0] = rup.x;
    r->m[1][1] = rup.y;
    r->m[1][2] = rup.z;

    r->m[2][0] = dirn.x;
    r->m[2][1] = dirn.y;
    r->m[2][2] = dirn.z;

    r->m[3][0] = origin.x;
    r->m[3][1] = origin.y;
    r->m[3][2] = origin.z;
}

void mInvert(float *restrict dst, const float *restrict mat)
{
    // original source: ftp://download.intel.com/design/PentiumIII/sml/24504301.pdf
    // mirrored: https://github.com/esAux/esAux-Menger/raw/main/SIMD%20Matrix%20Inverse.pdf

    // Cramer Invert
    float tmp[12]; /* temp array for pairs */
    float src[16]; /* array of transpose source matrix */
    float det; /* determinant */

    /* transpose matrix */
    for(int i = 0; i < 4; i++)
    {
        src[i] = mat[i*4];
        src[i + 4] = mat[i*4 + 1];
        src[i + 8] = mat[i*4 + 2];
        src[i + 12] = mat[i*4 + 3];
    }

    /* calculate pairs for first 8 elements (cofactors) */
    tmp[0] = src[10] * src[15];
    tmp[1] = src[11] * src[14];
    tmp[2] = src[9] * src[15];
    tmp[3] = src[11] * src[13];
    tmp[4] = src[9] * src[14];
    tmp[5] = src[10] * src[13];
    tmp[6] = src[8] * src[15];
    tmp[7] = src[11] * src[12];
    tmp[8] = src[8] * src[14];
    tmp[9] = src[10] * src[12];
    tmp[10] = src[8] * src[13];
    tmp[11] = src[9] * src[12];

    /* calculate first 8 elements (cofactors) */
    dst[0] = tmp[0]*src[5] + tmp[3]*src[6] + tmp[4]*src[7];
    dst[0] -= tmp[1]*src[5] + tmp[2]*src[6] + tmp[5]*src[7];
    dst[1] = tmp[1]*src[4] + tmp[6]*src[6] + tmp[9]*src[7];
    dst[1] -= tmp[0]*src[4] + tmp[7]*src[6] + tmp[8]*src[7];
    dst[2] = tmp[2]*src[4] + tmp[7]*src[5] + tmp[10]*src[7];
    dst[2] -= tmp[3]*src[4] + tmp[6]*src[5] + tmp[11]*src[7];
    dst[3] = tmp[5]*src[4] + tmp[8]*src[5] + tmp[11]*src[6];
    dst[3] -= tmp[4]*src[4] + tmp[9]*src[5] + tmp[10]*src[6];
    dst[4] = tmp[1]*src[1] + tmp[2]*src[2] + tmp[5]*src[3];
    dst[4] -= tmp[0]*src[1] + tmp[3]*src[2] + tmp[4]*src[3];
    dst[5] = tmp[0]*src[0] + tmp[7]*src[2] + tmp[8]*src[3];
    dst[5] -= tmp[1]*src[0] + tmp[6]*src[2] + tmp[9]*src[3];
    dst[6] = tmp[3]*src[0] + tmp[6]*src[1] + tmp[11]*src[3];
    dst[6] -= tmp[2]*src[0] + tmp[7]*src[1] + tmp[10]*src[3];
    dst[7] = tmp[4]*src[0] + tmp[9]*src[1] + tmp[10]*src[2];
    dst[7] -= tmp[5]*src[0] + tmp[8]*src[1] + tmp[11]*src[2];

    /* calculate pairs for second 8 elements (cofactors) */
    tmp[0] = src[2]*src[7];
    tmp[1] = src[3]*src[6];
    tmp[2] = src[1]*src[7];
    tmp[3] = src[3]*src[5];
    tmp[4] = src[1]*src[6];
    tmp[5] = src[2]*src[5];
    tmp[6] = src[0]*src[7];
    tmp[7] = src[3]*src[4];
    tmp[8] = src[0]*src[6];
    tmp[9] = src[2]*src[4];
    tmp[10] = src[0]*src[5];
    tmp[11] = src[1]*src[4];

    /* calculate second 8 elements (cofactors) */
    dst[8] = tmp[0]*src[13] + tmp[3]*src[14] + tmp[4]*src[15];
    dst[8] -= tmp[1]*src[13] + tmp[2]*src[14] + tmp[5]*src[15];
    dst[9] = tmp[1]*src[12] + tmp[6]*src[14] + tmp[9]*src[15];
    dst[9] -= tmp[0]*src[12] + tmp[7]*src[14] + tmp[8]*src[15];
    dst[10] = tmp[2]*src[12] + tmp[7]*src[13] + tmp[10]*src[15];
    dst[10]-= tmp[3]*src[12] + tmp[6]*src[13] + tmp[11]*src[15];
    dst[11] = tmp[5]*src[12] + tmp[8]*src[13] + tmp[11]*src[14];
    dst[11]-= tmp[4]*src[12] + tmp[9]*src[13] + tmp[10]*src[14];
    dst[12] = tmp[2]*src[10] + tmp[5]*src[11] + tmp[1]*src[9];
    dst[12]-= tmp[4]*src[11] + tmp[0]*src[9] + tmp[3]*src[10];
    dst[13] = tmp[8]*src[11] + tmp[0]*src[8] + tmp[7]*src[10];
    dst[13]-= tmp[6]*src[10] + tmp[9]*src[11] + tmp[1]*src[8];
    dst[14] = tmp[6]*src[9] + tmp[11]*src[11] + tmp[3]*src[8];
    dst[14]-= tmp[10]*src[11] + tmp[2]*src[8] + tmp[7]*src[9];
    dst[15] = tmp[10]*src[10] + tmp[4]*src[8] + tmp[9]*src[9];
    dst[15]-= tmp[8]*src[9] + tmp[11]*src[10] + tmp[5]*src[8];

    /* calculate determinant */
    det = src[0]*dst[0]+src[1]*dst[1]+src[2]*dst[2]+src[3]*dst[3];

    /* calculate matrix inverse */
    det = 1.0f/det;
    for(int j = 0; j < 16; j++){dst[j] *= det;}
}

void mTranspose(mat *r, const mat *restrict m)
{
    r->m[1][0] = m->m[0][1];
    r->m[2][0] = m->m[0][2];
    r->m[3][0] = m->m[0][3];

    r->m[0][1] = m->m[1][0];
    r->m[1][1] = m->m[1][1];
    r->m[2][1] = m->m[1][2];
    r->m[3][1] = m->m[1][3];

    r->m[0][2] = m->m[2][0];
    r->m[1][2] = m->m[2][1];
    r->m[2][2] = m->m[2][2];
    r->m[3][2] = m->m[2][3];

    r->m[0][3] = m->m[3][0];
    r->m[1][3] = m->m[3][1];
    r->m[2][3] = m->m[3][2];
    r->m[3][3] = m->m[3][3];
}

void mSetViewDir(mat *r, const vec dir_norm, const vec up_norm)
{
    vec c;
    vCross(&c, up_norm, dir_norm);
    vNorm(&c);

    vec rup;
    vCross(&rup, dir_norm, c);

    r->m[0][0] = c.x;
    r->m[0][1] = c.y;
    r->m[0][2] = c.z;

    r->m[1][0] = rup.x;
    r->m[1][1] = rup.y;
    r->m[1][2] = rup.z;

    r->m[2][0] = dir_norm.x;
    r->m[2][1] = dir_norm.y;
    r->m[2][2] = dir_norm.z;
}

void mGetViewDir(vec *r, const mat matrix)
{
    r->x = -matrix.m[0][2];
    r->y = -matrix.m[1][2];
    r->z = -matrix.m[2][2];
}

void mGetDirX(vec *r, const mat matrix)
{
    r->x = matrix.m[0][0];
    r->y = matrix.m[0][1];
    r->z = matrix.m[0][2];
}

void mGetDirY(vec *r, const mat matrix)
{
    r->x = matrix.m[1][0];
    r->y = matrix.m[1][1];
    r->z = matrix.m[1][2];
}

void mGetDirZ(vec *r, const mat matrix)
{
    r->x = matrix.m[2][0];
    r->y = matrix.m[2][1];
    r->z = matrix.m[2][2];
}

void mGetPos(vec *r, const mat matrix)
{
    r->x = matrix.m[3][0];
    r->y = matrix.m[3][1];
    r->z = matrix.m[3][2];
}

#endif