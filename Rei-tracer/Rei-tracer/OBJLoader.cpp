#include "OBJLoader.h"
#include <sstream>

using namespace DirectX;

unsigned OBJLoader::LoadOBJ(const std::string & filename, Triangle * triangleArray, unsigned maxCount) const
{

	std::ifstream fin(filename);

	std::vector<XMFLOAT3> positions;
	std::vector<XMFLOAT3> normals;
	std::vector<XMFLOAT2> texcoords;

	std::vector<unsigned> positionIndices;
	std::vector<unsigned> normalIndices;
	std::vector<unsigned> texcoordIndices;

	std::vector<unsigned> finishedIndices;

	if (filename.substr(filename.size() - 3) == "obj")
	{


		for (std::string line; std::getline(fin, line);)
		{
			std::istringstream input(line);
			std::string type;
			input >> type;

			if (type == "v")
			{
				XMFLOAT3 pos;
				input >> pos.x >> pos.y >> pos.z;
				positions.push_back(pos);
			}
			else if (type == "vt")
			{
				XMFLOAT2 tex;
				input >> tex.x >> tex.y;
				texcoords.push_back(tex);
			}
			else if (type == "vn")
			{
				XMFLOAT3 normal;
				input >> normal.x >> normal.y >> normal.z;
				normals.push_back(normal);
			}
			else if (type == "f")
			{
				int pos, tex, nor;
				char garbage;
				for (int i = 0; i < 3; ++i)
				{
					input >> pos >> garbage >> tex >> garbage >> nor;
					positionIndices.push_back(pos);
					texcoordIndices.push_back(tex);
					normalIndices.push_back(nor);

				}
			}
		}
	}
	else
	{
		return 0;
	}

	std::vector<XMFLOAT3> realPos;
	realPos.reserve(positionIndices.size());
	std::vector<XMFLOAT2> realTex;
	realTex.reserve(texcoordIndices.size());
	std::vector<XMFLOAT3> realNor;
	realNor.reserve(normalIndices.size());



	for (auto& pos : positionIndices)
	{
		realPos.push_back(positions[pos - 1]);
	}
	for (auto& tex : texcoordIndices)
	{
		realTex.push_back(texcoords[tex - 1]);
	}
	for (auto& nor : normalIndices)
	{
		realNor.push_back(normals[nor - 1]);
	}
	std::vector<XMFLOAT4> tan1;
	std::vector<XMFLOAT4> tan2;
	tan1.resize(realNor.size(), XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f));
	tan2.resize(realNor.size(), XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f));

	std::vector<XMFLOAT4> realTan;
	realTan.resize(realNor.size(), XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f));
	for (unsigned i = 0; i < positionIndices.size(); i += 3)
	{
		const XMFLOAT3& v1 = realPos[i];
		const XMFLOAT3& v2 = realPos[i + 1];
		const XMFLOAT3& v3 = realPos[i + 2];

		const XMFLOAT2& u1 = realTex[i];
		const XMFLOAT2& u2 = realTex[i + 1];
		const XMFLOAT2& u3 = realTex[i + 2];

		//Edge positions
		XMFLOAT3 deltaPos1(v2.x - v1.x, v2.y - v1.y, v2.z - v1.z);
		XMFLOAT3 deltaPos2(v3.x - v1.x, v3.y - v1.y, v3.z - v1.z);
		//Edge UVs
		XMFLOAT2 deltaTex1(u2.x - u1.x, u3.x - u1.x);
		XMFLOAT2 deltaTex2(u2.y - u1.y, u3.y - u1.y);

		float r = 1.0f / (deltaTex1.x * deltaTex2.y - deltaTex1.y * deltaTex2.x);
		
		XMFLOAT4 tangent(r*(deltaPos1.x * deltaTex2.y - deltaPos2.x * deltaTex1.y), r*(deltaPos1.y * deltaTex2.y - deltaPos2.y * deltaTex1.y), r*(deltaPos1.z * deltaTex2.y - deltaPos2.z * deltaTex1.y), 1.0f);

		XMFLOAT3 sdir = XMFLOAT3((deltaTex2.y*deltaPos1.x - deltaTex2.x*deltaPos2.x) * r, (deltaTex2.y*deltaPos1.y - deltaTex2.x*deltaPos2.y)*r, (deltaTex2.y*deltaPos1.z - deltaTex2.x*deltaPos2.z)*r);
		XMFLOAT3 tdir = XMFLOAT3((deltaTex1.x*deltaPos2.x - deltaTex1.y*deltaPos1.x)*r, (deltaTex1.x*deltaPos2.y - deltaTex1.y*deltaPos1.y)*r, (deltaTex1.x*deltaPos2.z - deltaTex1.y*deltaPos1.z)*r);

		tan1[i].x += sdir.x;
		tan1[i].y += sdir.y;
		tan1[i].z += sdir.z;

		tan2[i].x += tdir.x;
		tan2[i].y += tdir.y;
		tan2[i].z += tdir.z;

		tan1[i + 1].x += sdir.x;
		tan1[i + 1].y += sdir.y;
		tan1[i + 1].z += sdir.z;

		tan2[i + 1].x += tdir.x;
		tan2[i + 1].y += tdir.y;
		tan2[i + 1].z += tdir.z;

		tan1[i + 2].x += sdir.x;
		tan1[i + 2].y += sdir.y;
		tan1[i + 2].z += sdir.z;

		tan2[i + 2].x += tdir.x;
		tan2[i + 2].y += tdir.y;
		tan2[i + 2].z += tdir.z;
	}

	for (unsigned i = 0; i < realPos.size(); ++i)
	{
		XMVECTOR n = XMLoadFloat3(&realNor[i]);
		XMVECTOR t = XMLoadFloat4(&tan1[i]);

		XMVECTOR tangent = XMVector3Normalize(t - n * XMVector3Dot(n, t));

		XMStoreFloat4(&realTan[i], tangent);
		//Calculate handedness
		realTan[i].w = XMVectorGetX(XMVector3Dot(XMVector3Cross(n, t), XMLoadFloat4(&tan2[i]))) < 0.0f ? -1.0f : 1.0f;
	}

	auto nrOfVertices = realPos.size();
	if (nrOfVertices / 3 > maxCount)
		return 0;

	for (int i = 0; i < nrOfVertices; i += 3)
	{
		triangleArray[i / 3].v1 = TriangleVertex(realPos[i], realNor[i], realTan[i], realTex[i]);
		triangleArray[i / 3].v2 = TriangleVertex(realPos[i + 1], realNor[i + 1], realTan[i + 1], realTex[i + 1]);
		triangleArray[i / 3].v3 = TriangleVertex(realPos[i + 2], realNor[i + 2], realTan[i + 2], realTex[i + 2]);
	}

	return (unsigned)nrOfVertices / 3;

}

unsigned OBJLoader::PartitionMesh(Triangle * triangles, unsigned triangleCount, unsigned offset, OctNode ** octTree, unsigned levels, unsigned& nodeCountOut, unsigned maxCount) const
{
	/*
		k-ary tree
	  Stored in an array:
	  [A][AA][AB][AC][AD][AE][AF][AG][AH][AAA][AAB][AAC][AAD][AAE][AAF][AAG][AAH][ABA][ABB][ABC]...
	   0  1   2   3   4   5   6   7   8   9    10   11   12   13   14   15   16   17   18   19
	   childindexN = (parentindex * 8) + N
	   i.e.
	   indexOf(AAB) = indexOf(AA) * 8 + 2 = 10
	*/
	XMFLOAT3 centerPoint;
	float x, y, z;
	x = y = z = 0;
	float maxx, maxy, maxz;
	float minx, miny, minz;
	maxx = maxy = maxz = -FLT_MAX;
	minx = miny = minz = FLT_MAX;
	for (int i = 0; i < triangleCount; i++)
	{
		if (triangles[i].v1.posx > maxx)
			maxx = triangles[i].v1.posx;
		if (triangles[i].v2.posx > maxx)
			maxx = triangles[i].v2.posx;
		if (triangles[i].v3.posx > maxx)
			maxx = triangles[i].v3.posx;

		if (triangles[i].v1.posy > maxy)
			maxy = triangles[i].v1.posy;
		if (triangles[i].v2.posy > maxy)
			maxy = triangles[i].v2.posy;
		if (triangles[i].v3.posy > maxy)
			maxy = triangles[i].v3.posy;

		if (triangles[i].v1.posz > maxz)
			maxz = triangles[i].v1.posz;
		if (triangles[i].v2.posz > maxz)
			maxz = triangles[i].v2.posz;
		if (triangles[i].v3.posz > maxz)
			maxz = triangles[i].v3.posz;
		
		//min
		if (triangles[i].v1.posx < minx)
			minx = triangles[i].v1.posx;
		if (triangles[i].v2.posx < minx)
			minx = triangles[i].v2.posx;
		if (triangles[i].v3.posx < minx)
			minx = triangles[i].v3.posx;

		if (triangles[i].v1.posy < miny)
			miny = triangles[i].v1.posy;
		if (triangles[i].v2.posy < miny)
			miny = triangles[i].v2.posy;
		if (triangles[i].v3.posy < miny)
			miny = triangles[i].v3.posy;

		if (triangles[i].v1.posz < minz)
			minz = triangles[i].v1.posz;
		if (triangles[i].v2.posz < minz)
			minz = triangles[i].v2.posz;
		if (triangles[i].v3.posz < minz)
			minz = triangles[i].v3.posz;
	}


	XMFLOAT3 centerPos = XMFLOAT3((maxx + minx) / 2.0f, (maxy + miny) / 2.0f, (maxz + minz) / 2.0f);
	XMFLOAT3 halflengths = XMFLOAT3((maxx - minx) / 2.0f, (maxy - miny) / 2.0f, (maxy - miny) / 2.0f);
	//halflengths.x += 0.01f;
	//halflengths.y += 0.01f;
	//halflengths.z += 0.01f;

	nodeCountOut = 0;
	for (int i = levels; i >= 0; i--)
	{
		nodeCountOut += _Pow_int(8, i);
	}
	*octTree = new OctNode[nodeCountOut];
	OctNode* tree = *octTree;
	_BuildOctTree(tree, 0, nodeCountOut, centerPos, halflengths);

	std::vector<Triangle> newTriangles;
	newTriangles.reserve(2048);
	std::vector<Triangle> nodeTriangles;
	nodeTriangles.reserve(2048);
	uint8_t* taken = new uint8_t[triangleCount];
	memset(taken, 0, sizeof(uint8_t) * triangleCount);
	//For every node, check every triangle
	//If a triangle is only partially inside a node, it is the parent node who gets it
	//triangle must be fully contained to go into a node
	for (int i = nodeCountOut - 1; i >= 0; i--)
	{
		for (int it = 0; it < triangleCount; it++)
		{
			if (!taken[it])
			{
				if (_NodeContainsTriangle(tree[i], triangles[it]))
				{
					nodeTriangles.push_back(triangles[it]);
					taken[it] = 1;
				}
			}
		}
		
		tree[i].lower = offset + newTriangles.size();
		tree[i].upper = offset + newTriangles.size() + nodeTriangles.size();
		for (auto& tri : nodeTriangles)
			newTriangles.push_back(tri);
		nodeTriangles.clear();
	}
	delete[] taken;

	memcpy(triangles, &newTriangles[0], sizeof(Triangle) * newTriangles.size());

	return newTriangles.size();
}

void OBJLoader::_BuildOctTree(OctNode * octTree, unsigned index, unsigned stop, DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 halflengths) const
{
	if (index >= stop)
		return;

	octTree[index].posx = pos.x;
	octTree[index].posy = pos.y;
	octTree[index].posz = pos.z;
	octTree[index].halfx = halflengths.x;
	octTree[index].halfy = halflengths.y;
	octTree[index].halfz = halflengths.z;
	
	halflengths.x /= 2.0f;
	halflengths.y /= 2.0f;
	halflengths.z /= 2.0f;

	for (int i = 0; i < 8; i++)
	{
		unsigned childIndex = (8 * index) + (i + 1);
		float offsetx = i % 2 == 0 ? -1 : 1;
		float offsetz = i % 4 < 2 ? 1 : -1;
		float offsety = i < 4 ? 1 : -1;
		XMFLOAT3 newpos = pos;
		newpos.x += offsetx * (halflengths.x);
		newpos.y += offsety * (halflengths.y);
		newpos.z += offsetz * (halflengths.z);
		_BuildOctTree(octTree, childIndex, stop, newpos, halflengths);
	}

}

bool OBJLoader::_NodeContainsTriangle(const OctNode & node, const Triangle & t) const
{
	//XMVECTOR center = XMVectorSet(node.posx, node.posy, node.posz, 1.0f);
	//XMVECTOR vertices[3];
	//vertices[0] = XMVectorSet(t.v1.posx, t.v1.posy, t.v1.posz, 1.0f);
	//vertices[1] = XMVectorSet(t.v2.posx, t.v2.posy, t.v2.posz, 1.0f);
	//vertices[2] = XMVectorSet(t.v3.posx, t.v3.posy, t.v3.posz, 1.0f);

	//XMVECTOR planes[6];
	//planes[0] = XMVectorSet(1.0f, 0.0f, 0.0f, (node.posx - node.halfx));
	//planes[1] = XMVectorSet(-1.0f, 0.0f, 0.0f, -(node.posx + node.halfx));
	//planes[2] = XMVectorSet(0.0f, 1.0f, 0.0f, (node.posy - node.halfy));
	//planes[3] = XMVectorSet(0.0f, -1.0f, 0.0f, -(node.posy + node.halfy));
	//planes[4] = XMVectorSet(0.0f, 0.0f, 1.0f, (node.posz - node.halfz));
	//planes[5] = XMVectorSet(0.0f, 0.0f, -1.0f, -(node.posz + node.halfz));

	//bool completelyOutside = true;
	//for (int i = 0; i < 3; i++)
	//{
	//	bool vertInside = true;
	//	for (int j = 0; j < 6; j++)
	//	{
	//		float test = XMVectorGetX(XMVector3Dot(vertices[i] - (planes[j] * XMVectorGetW(planes[j])) , planes[j]));
	//		if (test < 0.0f)
	//		{
	//			vertInside = false;
	//		}
	//	}
	//	if (vertInside)
	//		completelyOutside = false;
	//}
	//if (completelyOutside)
	//	return false;

	if ((t.v1.posx >= node.posx - node.halfx && t.v1.posx <= node.posx + node.halfx &&
		t.v1.posy >= node.posy - node.halfy && t.v1.posy <= node.posy + node.halfy &&
		t.v1.posz >= node.posz - node.halfz && t.v1.posz <= node.posz + node.halfz) &&
		(t.v2.posx >= node.posx - node.halfx && t.v2.posx <= node.posx + node.halfx &&
		t.v2.posy >= node.posy - node.halfy && t.v2.posy <= node.posy + node.halfy &&
		t.v2.posz >= node.posz - node.halfz && t.v2.posz <= node.posz + node.halfz) &&
		(t.v3.posx >= node.posx - node.halfx && t.v3.posx <= node.posx + node.halfx &&
		t.v3.posy >= node.posy - node.halfy && t.v3.posy <= node.posy + node.halfy &&
		t.v3.posz >= node.posz - node.halfz && t.v3.posz <= node.posz + node.halfz))
		return true;

	return false;

}
