//#include "StdAfx.h"
#include "vcTransformation.h"
#include <math.h>
#include "vcUtils.h"
#include <sstream>

vcTransformation::vcTransformation(void)
{
}

vcTransformation::~vcTransformation(void)
{
}

void vcTransformation::DecomposeMatrix(float mat4x4[4][4],float rotation[4],float translation[3],float scale[3])
{
	float mat3x3[3][3];
	mat3x3[0][0] = mat4x4[0][0]; 
	mat3x3[0][1] = mat4x4[0][1];
	mat3x3[0][2] = mat4x4[0][2];
	mat3x3[1][0] = mat4x4[1][0];
	mat3x3[1][1] = mat4x4[1][1];
	mat3x3[1][2] = mat4x4[1][2];
	mat3x3[2][0] = mat4x4[2][0];
	mat3x3[2][1] = mat4x4[2][1];
	mat3x3[2][2] = mat4x4[2][2];

	float mat_new3x3[3][3];
	float shear[3];
	QDUDecomposition(mat3x3,mat_new3x3,scale,shear);

	float axis[3]; 
	axis[0]=0.0f;axis[1]=0.0f;axis[2]=0.0f;  
	float angle=0.0f; 

	GetAxisAndAngle(mat_new3x3,axis,angle);

	rotation[0] = axis[0];
	rotation[1] = axis[1];
	rotation[2] = axis[2];
	rotation[3] = angle;
	
	translation[0] = mat4x4[0][3];
	translation[1] = mat4x4[1][3];
	translation[2] = mat4x4[2][3];
  
	return;
}

void vcTransformation::GetAxisAndAngle(const float kRot[3][3], float axis[3], float& angle)
{
	float x,y,z,w;
	float fTrace = kRot[0][0]+kRot[1][1]+kRot[2][2];
	float fRoot;

	if ( fTrace > 0.0 )
	{
		// |w| > 1/2, may as well choose w > 1/2
		fRoot = sqrt(fTrace + 1.0f);  // 2w
		w = 0.5f*fRoot;
		fRoot = 0.5f/fRoot;  // 1/(4w)
		x = (kRot[2][1]-kRot[1][2])*fRoot;
		y = (kRot[0][2]-kRot[2][0])*fRoot;
		z = (kRot[1][0]-kRot[0][1])*fRoot;
	}
	else
	{
		// |w| <= 1/2
		static size_t s_iNext[3] = { 1, 2, 0 };
		size_t i = 0;
		if ( kRot[1][1] > kRot[0][0] )
			i = 1;
		if ( kRot[2][2] > kRot[i][i] )
			i = 2;
		size_t j = s_iNext[i];
		size_t k = s_iNext[j];

		fRoot = sqrt(kRot[i][i]-kRot[j][j]-kRot[k][k] + 1.0f);
		float* apkQuat[3] = { &x, &y, &z };
		*apkQuat[i] = 0.5f*fRoot;
		fRoot = 0.5f/fRoot;
		w = (kRot[k][j]-kRot[j][k])*fRoot;
		*apkQuat[j] = (kRot[j][i]+kRot[i][j])*fRoot;
		*apkQuat[k] = (kRot[k][i]+kRot[i][k])*fRoot;
	}

	float fSqrLength = x*x+y*y+z*z;
	if ( fSqrLength > 0.0 )
	{
		angle = 2.0*acosf(w);
		float fInvLength = 1.0f/sqrtf(fSqrLength);//Math::InvSqrt(fSqrLength);
		axis[0] = x*fInvLength; 
		axis[1] = y*fInvLength;
		axis[2] = z*fInvLength;
	}
	else
	{
		// angle is 0 (mod 2*pi), so any axis will do 
		angle = 0;//Radian(0.0);
		axis[0] = 1.0;
		axis[1] = 0.0; 
		axis[2] = 0.0;
	} 

}

void vcTransformation::QDUDecomposition (float m[3][3],float kQ[3][3],float kD[3], float kU[3]) 
{
    // Factor M = QR = QDU where Q is orthogonal, D is diagonal,
    // and U is upper triangular with ones on its diagonal.  Algorithm uses
    // Gram-Schmidt orthogonalization (the QR algorithm).
    //
    // If M = [ m0 | m1 | m2 ] and Q = [ q0 | q1 | q2 ], then
    //
    //   q0 = m0/|m0|
    //   q1 = (m1-(q0*m1)q0)/|m1-(q0*m1)q0|
    //   q2 = (m2-(q0*m2)q0-(q1*m2)q1)/|m2-(q0*m2)q0-(q1*m2)q1|
    //
    // where |V| indicates length of vector V and A*B indicates dot
    // product of vectors A and B.  The matrix R has entries
    //
    //   r00 = q0*m0  r01 = q0*m1  r02 = q0*m2
    //   r10 = 0      r11 = q1*m1  r12 = q1*m2
    //   r20 = 0      r21 = 0      r22 = q2*m2
    //
    // so D = diag(r00,r11,r22) and U has entries u01 = r01/r00,
    // u02 = r02/r00, and u12 = r12/r11.

    // Q = rotation
    // D = scaling
    // U = shear

    // D stores the three diagonal entries r00, r11, r22
    // U stores the entries U[0] = u01, U[1] = u02, U[2] = u12

    // build orthogonal matrix Q
    float fInvLength = 1.0f/sqrt(m[0][0]*m[0][0] + m[1][0]*m[1][0] + m[2][0]*m[2][0]);
    kQ[0][0] = m[0][0]*fInvLength;
    kQ[1][0] = m[1][0]*fInvLength;
    kQ[2][0] = m[2][0]*fInvLength;

    float fDot = kQ[0][0]*m[0][1] + kQ[1][0]*m[1][1] + kQ[2][0]*m[2][1];
    kQ[0][1] = m[0][1]-fDot*kQ[0][0];
    kQ[1][1] = m[1][1]-fDot*kQ[1][0];
    kQ[2][1] = m[2][1]-fDot*kQ[2][0];
    fInvLength = 1.0f/sqrt(kQ[0][1]*kQ[0][1] + kQ[1][1]*kQ[1][1] + kQ[2][1]*kQ[2][1]);
    kQ[0][1] *= fInvLength;
    kQ[1][1] *= fInvLength;
    kQ[2][1] *= fInvLength;

    fDot = kQ[0][0]*m[0][2] + kQ[1][0]*m[1][2] + kQ[2][0]*m[2][2];
    kQ[0][2] = m[0][2]-fDot*kQ[0][0];
    kQ[1][2] = m[1][2]-fDot*kQ[1][0];
    kQ[2][2] = m[2][2]-fDot*kQ[2][0];
    fDot = kQ[0][1]*m[0][2] + kQ[1][1]*m[1][2] + kQ[2][1]*m[2][2];
    kQ[0][2] -= fDot*kQ[0][1];
    kQ[1][2] -= fDot*kQ[1][1];
    kQ[2][2] -= fDot*kQ[2][1];
    fInvLength = 1.0f/sqrt(kQ[0][2]*kQ[0][2] + kQ[1][2]*kQ[1][2] + kQ[2][2]*kQ[2][2]);
    kQ[0][2] *= fInvLength;
    kQ[1][2] *= fInvLength;
    kQ[2][2] *= fInvLength;

    // guarantee that orthogonal matrix has determinant 1 (no reflections)
    float fDet = kQ[0][0]*kQ[1][1]*kQ[2][2] + kQ[0][1]*kQ[1][2]*kQ[2][0] +
				 kQ[0][2]*kQ[1][0]*kQ[2][1] - kQ[0][2]*kQ[1][1]*kQ[2][0] -
				 kQ[0][1]*kQ[1][0]*kQ[2][2] - kQ[0][0]*kQ[1][2]*kQ[2][1];

    if ( fDet < 0.0 )
    {
        for (size_t iRow = 0; iRow < 3; iRow++)
            for (size_t iCol = 0; iCol < 3; iCol++)
                kQ[iRow][iCol] = -kQ[iRow][iCol];
    }

    // build "right" matrix R
    float kR[3][3];
    kR[0][0] = kQ[0][0]*m[0][0] + kQ[1][0]*m[1][0] + kQ[2][0]*m[2][0];
    kR[0][1] = kQ[0][0]*m[0][1] + kQ[1][0]*m[1][1] + kQ[2][0]*m[2][1];
    kR[1][1] = kQ[0][1]*m[0][1] + kQ[1][1]*m[1][1] + kQ[2][1]*m[2][1];
    kR[0][2] = kQ[0][0]*m[0][2] + kQ[1][0]*m[1][2] + kQ[2][0]*m[2][2];
    kR[1][2] = kQ[0][1]*m[0][2] + kQ[1][1]*m[1][2] + kQ[2][1]*m[2][2];
    kR[2][2] = kQ[0][2]*m[0][2] + kQ[1][2]*m[1][2] + kQ[2][2]*m[2][2];
	
    // the scaling component
    kD[0] = kR[0][0];
    kD[1] = kR[1][1];
    kD[2] = kR[2][2];

    // the shear component
    float fInvD0 = 1.0f/kD[0];
    kU[0] = kR[0][1]*fInvD0;
    kU[1] = kR[0][2]*fInvD0;
    kU[2] = kR[1][2]/kD[1];
}

//Constructs the transformation_matix as float[16] from translation, rotation and scale values
void vcTransformation::GetTransformationMatrix(const float *trans,const float *rotation,const float *scale,float result_matrix[16])
{
	float x=0,y=0,z=0,ang=0;

	  x = rotation[0];
	  y = rotation[1];
	  z = rotation[2]; 
	ang = rotation[3];
	
	//Getting rotation matrix
	float rot_mat[4][4];
	rot_mat[0][0] = cos(ang)+ ((x*x)*(1-cos(ang)));       rot_mat[0][1] = ((x*y)*(1-cos(ang))) - (z*sin(ang));  rot_mat[0][2] = ((x*z)*(1-cos(ang))) + (y*sin(ang)); rot_mat[0][3] = 0;
	rot_mat[1][0] = ((y*x)*(1-cos(ang))) + (z*sin(ang));  rot_mat[1][1] = cos(ang) + ((y*y)*(1-cos(ang)));      rot_mat[1][2] = ((y*z)*(1-cos(ang))) - (x*sin(ang)); rot_mat[1][3] = 0;
	rot_mat[2][0] = ((z*x)*(1-cos(ang))) - (y*sin(ang));  rot_mat[2][1] = ((z*y)*(1-cos(ang))) + (x*sin(ang));  rot_mat[2][2] = cos(ang) + ((z*z)*(1-cos(ang)));     rot_mat[2][3] = 0;
	rot_mat[3][0] = 0;								      rot_mat[3][1] = 0;									rot_mat[3][2] = 0;									 rot_mat[3][3] = 0;

	//Applying the scale
	x = scale[0];
	y = scale[1];
	z = scale[2];

	result_matrix[0]  = rot_mat[0][0]*x;  
	result_matrix[1]  = rot_mat[1][0];
	result_matrix[2]  = rot_mat[2][0];
	result_matrix[3]  = rot_mat[3][0];

	result_matrix[4]  = rot_mat[0][1];  
	result_matrix[5]  = rot_mat[1][1]*y;
	result_matrix[6]  = rot_mat[2][1];
	result_matrix[7]  = rot_mat[3][1];

	result_matrix[8]  = rot_mat[0][2];  
	result_matrix[9]  = rot_mat[1][2];
	result_matrix[10] = rot_mat[2][2]*z;
	result_matrix[11] = rot_mat[3][2];

	result_matrix[12] = rot_mat[0][3];   
	result_matrix[13] = rot_mat[1][3];
	result_matrix[14] = rot_mat[2][3];
	result_matrix[15] = rot_mat[3][3];

	//Appplying the translation
	x = trans[0];
	y = trans[1];
	z = trans[2];

	result_matrix[12] = x;
	result_matrix[13] = y;
	result_matrix[14] = z;

}

//Constructs the transformation_matix as float[4][4] from translation, rotation and scale values
void vcTransformation::GetTransformationMatrix(const float *trans,const float *rotation,const float *scale,float result_matrix[4][4])
{
	float x=0,y=0,z=0,ang=0;

	  x = rotation[0];
	  y = rotation[1];
	  z = rotation[2]; 
	ang = rotation[3];
	
	//Getting rotation matrix
	float rot_mat[4][4];
	rot_mat[0][0] = cos(ang)+ ((x*x)*(1-cos(ang)));       rot_mat[0][1] = ((x*y)*(1-cos(ang))) - (z*sin(ang));  rot_mat[0][2] = ((x*z)*(1-cos(ang))) + (y*sin(ang)); rot_mat[0][3] = 0;
	rot_mat[1][0] = ((y*x)*(1-cos(ang))) + (z*sin(ang));  rot_mat[1][1] = cos(ang) + ((y*y)*(1-cos(ang)));      rot_mat[1][2] = ((y*z)*(1-cos(ang))) - (x*sin(ang)); rot_mat[1][3] = 0;
	rot_mat[2][0] = ((z*x)*(1-cos(ang))) - (y*sin(ang));  rot_mat[2][1] = ((z*y)*(1-cos(ang))) + (x*sin(ang));  rot_mat[2][2] = cos(ang) + ((z*z)*(1-cos(ang)));     rot_mat[2][3] = 0;
	rot_mat[3][0] = 0;								      rot_mat[3][1] = 0;									rot_mat[3][2] = 0;									 rot_mat[3][3] = 0;

	//Applying the scale
	x = scale[0]; 
	y = scale[1];
	z = scale[2];

	result_matrix[0][0] = rot_mat[0][0]*x; result_matrix[0][1] = rot_mat[0][1];   result_matrix[0][2] = rot_mat[0][2];   result_matrix[0][3] = rot_mat[0][3];
	result_matrix[1][0] = rot_mat[1][0];   result_matrix[1][1] = rot_mat[1][1]*y; result_matrix[1][2] = rot_mat[1][2];   result_matrix[1][3] = rot_mat[1][3];
	result_matrix[2][0] = rot_mat[2][0];   result_matrix[2][1] = rot_mat[2][1];	  result_matrix[2][2] = rot_mat[2][2]*z; result_matrix[2][3] = rot_mat[2][3];
	result_matrix[3][0] = rot_mat[3][0];   result_matrix[3][1] = rot_mat[3][1];	  result_matrix[3][2] = rot_mat[3][2];	 result_matrix[3][3] = rot_mat[3][3];

	//Appplying the translation
	x = trans[0];
	y = trans[1];
	z = trans[2];

	result_matrix[0][3] = x;
	result_matrix[1][3] = y;
	result_matrix[2][3] = z;
}