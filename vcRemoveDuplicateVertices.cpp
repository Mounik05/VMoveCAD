//#include "StdAfx.h"
#include "vcRemoveDuplicateVertices.h"
#include <sstream>
#include <map>
#include "vcUtils.h"

extern bool g_bDetailedLog;
vcRemoveDuplicateVertices::vcRemoveDuplicateVertices(void)
{
} 

vcRemoveDuplicateVertices::~vcRemoveDuplicateVertices(void)
{
}

void vcRemoveDuplicateVertices::RemoveDuplicateVertices(std::vector<vcVec> OriCoordSet,std::vector<vcVec> &NewCoordSet,uint32_t *pCaxCoordIndexIntArray,int iCoordIndexCount,std::vector<vcVec> OriColorSet,std::vector<vcVec> &NewColorSet)
{
	if(g_bDetailedLog)
	{
		vcUtils::LogMsg("vcRemoveDuplicateVertices::RemoveDuplicateVertices():Start");
	}
	std::map<std::string,int> CoordSetNIndexMap;
	std::vector<int> NewIndex;
	std::stringstream ss;

	int newCoordSize = OriCoordSet.size();
	int newIndex=0;
	std::stringstream stream; 
	std::string val;
	if(OriColorSet.size())
	{
		if(g_bDetailedLog)
		{
			vcUtils::LogMsg("vcRemoveDuplicateVertices::RemoveDuplicateVertices(): Removing duplicate coordinates and colors...");
		}
		NewCoordSet.reserve(newCoordSize);
		NewColorSet.reserve(newCoordSize);
		NewIndex.reserve(newCoordSize);
   		for(int i=0;i<newCoordSize;i++) 
		{
			vcVec v = OriCoordSet[i];
			stream.str("");  
			stream<<v.x<<","<<v.y<<","<<v.z;
			val=stream.str();

			if(CoordSetNIndexMap.find(val) == CoordSetNIndexMap.end())
			{
				NewCoordSet.push_back(v);
				NewColorSet.push_back(OriColorSet[i]);
				CoordSetNIndexMap[val] = newIndex;
				NewIndex.push_back(newIndex);
				newIndex++;
			}
			else 
			{
				NewIndex.push_back(CoordSetNIndexMap[val]);
			}
		}
		if(g_bDetailedLog)
		{
			vcUtils::LogMsg("vcRemoveDuplicateVertices::RemoveDuplicateVertices(): duplicate coordinates and colors are removed");
		}
	}
	else
	{
		if(g_bDetailedLog)
		{
			vcUtils::LogMsg("vcRemoveDuplicateVertices::RemoveDuplicateVertices(): Removing duplicate coordinates...");
		}
		NewCoordSet.reserve(newCoordSize);
		NewIndex.reserve(newCoordSize);
   		for(int i=0;i<newCoordSize;i++) 
		{
			vcVec v = OriCoordSet[i];
			stream.str("");  
			stream<<v.x<<","<<v.y<<","<<v.z;
			val=stream.str();

			if(CoordSetNIndexMap.find(val) == CoordSetNIndexMap.end())
			{
				NewCoordSet.push_back(v);
				CoordSetNIndexMap[val] = newIndex;
				NewIndex.push_back(newIndex);
				newIndex++;
			}
			else 
			{
				NewIndex.push_back(CoordSetNIndexMap[val]);
			}
		}
		if(g_bDetailedLog)
		{
			vcUtils::LogMsg("vcRemoveDuplicateVertices::RemoveDuplicateVertices(): duplicate coordinates are removed");
		}
	}
	if(g_bDetailedLog)
	{
		vcUtils::LogMsg("vcRemoveDuplicateVertices::RemoveDuplicateVertices(): Rearranging the indices...");
	}
	for(int i=0;i<iCoordIndexCount;i++)
	{
		pCaxCoordIndexIntArray[i] = NewIndex[pCaxCoordIndexIntArray[i]];
	}
	if(g_bDetailedLog)
	{
		vcUtils::LogMsg("vcRemoveDuplicateVertices::RemoveDuplicateVertices(): Indices are rearranged");
		vcUtils::LogMsg("vcRemoveDuplicateVertices::RemoveDuplicateVertices():End");
	}
}


void vcRemoveDuplicateVertices::RemoveDuplicateVertices(std::vector<vcVec> OriCoordSet,std::vector<float> &NewCoordSet,uint32_t *pCaxCoordIndexIntArray,int iCoordIndexCount,std::vector<vcVec> OriColorSet,std::vector<float> &NewColorSet,float fUnitScaleFactor)
{
	if(g_bDetailedLog)
	{
		vcUtils::LogMsg("vcRemoveDuplicateVertices::RemoveDuplicateVertices():Start");
	}
	std::map<std::string,int> CoordSetNIndexMap;
	std::vector<uint32_t> NewIndex;
	std::stringstream ss;

	int newCoordSize = OriCoordSet.size();
	int newIndex=0;
	std::stringstream stream; 
	std::string val;
	if(OriColorSet.size())
	{
		if(g_bDetailedLog)
		{
			vcUtils::LogMsg("vcRemoveDuplicateVertices::RemoveDuplicateVertices(): Removing duplicate coordinates and colors...");
		}
		NewCoordSet.reserve(newCoordSize*3);
		NewColorSet.reserve(newCoordSize*3);
		NewIndex.reserve(newCoordSize);
   		for(int i=0;i<newCoordSize;i++) 
		{
			vcVec v = OriCoordSet[i];
			stream.str("");  
			stream<<v.x<<","<<v.y<<","<<v.z;
			val=stream.str();

			if(CoordSetNIndexMap.find(val) == CoordSetNIndexMap.end())
			{
				NewCoordSet.push_back(v.x * fUnitScaleFactor);
				NewCoordSet.push_back(v.y * fUnitScaleFactor);
				NewCoordSet.push_back(v.z * fUnitScaleFactor);

				NewColorSet.push_back(OriColorSet[i].x);
				NewColorSet.push_back(OriColorSet[i].y);
				NewColorSet.push_back(OriColorSet[i].z);

				CoordSetNIndexMap[val] = newIndex;
				NewIndex.push_back(newIndex);
				newIndex++;
			}
			else 
			{
				NewIndex.push_back(CoordSetNIndexMap[val]);
			}
		}
		if(g_bDetailedLog)
		{
			vcUtils::LogMsg("vcRemoveDuplicateVertices::RemoveDuplicateVertices(): duplicate coordinates and colors are removed");
		}
	}
	else
	{
		if(g_bDetailedLog)
		{
			vcUtils::LogMsg("vcRemoveDuplicateVertices::RemoveDuplicateVertices(): Removing duplicate coordinates...");
		}

		NewCoordSet.reserve(newCoordSize*3);
		NewIndex.reserve(newCoordSize);
   		for(int i=0;i<newCoordSize;i++) 
		{
			vcVec v = OriCoordSet[i];
			stream.str("");  
			stream<<v.x<<","<<v.y<<","<<v.z;
			val=stream.str();

			if(CoordSetNIndexMap.find(val) == CoordSetNIndexMap.end())
			{
				NewCoordSet.push_back(v.x * fUnitScaleFactor);
				NewCoordSet.push_back(v.y * fUnitScaleFactor);
				NewCoordSet.push_back(v.z * fUnitScaleFactor);

				CoordSetNIndexMap[val] = newIndex;
				NewIndex.push_back(newIndex);
				newIndex++;
			}
			else 
			{
				NewIndex.push_back(CoordSetNIndexMap[val]);
			}
		}

		if(g_bDetailedLog)
		{
			vcUtils::LogMsg("vcRemoveDuplicateVertices::RemoveDuplicateVertices(): duplicate coordinates are removed");
		}
	}
	if(g_bDetailedLog)
	{
		vcUtils::LogMsg("vcRemoveDuplicateVertices::RemoveDuplicateVertices(): Rearranging the indices...");
	}
	for(int i=0;i<iCoordIndexCount;i++)
	{
		pCaxCoordIndexIntArray[i] = NewIndex[pCaxCoordIndexIntArray[i]];
	}
	if(g_bDetailedLog)
	{
		vcUtils::LogMsg("vcRemoveDuplicateVertices::RemoveDuplicateVertices(): Indices are rearranged");
		vcUtils::LogMsg("vcRemoveDuplicateVertices::RemoveDuplicateVertices():End");
	}
}




