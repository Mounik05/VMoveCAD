#pragma once

#include <vector>
#include "vcVec.h"

#include "VctCaxScene.h"
#include "VctException.h"
using namespace Vct;
typedef Cax::uint32_t uint32_t;

class vcRemoveDuplicateVertices
{
public:
	vcRemoveDuplicateVertices(void);
	~vcRemoveDuplicateVertices(void);
	
	static void RemoveDuplicateVertices(std::vector<vcVec> OriCoordSet,std::vector<vcVec> &NewCoordSet,uint32_t *pCaxCoordIndexIntArray,int iCoordIndexCount,std::vector<vcVec> OriColorSet,std::vector<vcVec> &NewColorSet);
	static void RemoveDuplicateVertices(std::vector<vcVec> OriCoordSet,std::vector<float> &NewCoordSet,uint32_t *pCaxCoordIndexIntArray,int iCoordIndexCount,std::vector<vcVec> OriColorSet,std::vector<float> &NewColorSet,float fUnitScaleFactor);
};

