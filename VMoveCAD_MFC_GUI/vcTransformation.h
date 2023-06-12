#pragma once

class vcTransformation
{
	static void GetAxisAndAngle(const float kRot[3][3], float axis[3], float& angle);
	static void QDUDecomposition (float m[3][3],float kQ[3][3],float kD[3], float kU[3]);

public:
	vcTransformation(void);
	~vcTransformation(void);

	static void GetTransformationMatrix(const float *trans,const float *rotation,const float *scale,float result_matrix[16]);
	static void GetTransformationMatrix(const float *trans,const float *rotation,const float *scale,float result_matrix[4][4]);

	static void DecomposeMatrix(float mat4x4[4][4],float rotation[4],float translation[3],float scale[3]);

};
