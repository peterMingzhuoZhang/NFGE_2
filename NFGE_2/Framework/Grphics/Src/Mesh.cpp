//====================================================================================================
// Filename:	Mesh.cpp
// Created by:	Mingzhuo Zhang
// Date:		2022/9
//====================================================================================================

#include "Precompiled.h"
#include "Mesh.h"

using namespace NFGE::Graphics;

MeshPC MeshBuilder::CreateCubePC()
{
	VertexPC mVertices[] =
	{
		{{ -1.0f,  1.0f, -1.0f }, Colors::AliceBlue },
		{{  1.0f,  1.0f, -1.0f }, Colors::Beige },
		{{ -1.0f, -1.0f, -1.0f }, Colors::BlueViolet },
		{{  1.0f, -1.0f, -1.0f }, Colors::DeepSkyBlue },
		{{ -1.0f,  1.0f, 1.0f },  Colors::Honeydew },
		{{  1.0f,  1.0f, 1.0f },  Colors::Indigo },
		{{ -1.0f, -1.0f, 1.0f },  Colors::LemonChiffon },
		{{  1.0f, -1.0f, 1.0f },  Colors::OrangeRed }
	};

	uint32_t mIndices[] =
	{
		0,1,2,//|
		1,3,2,//|--- Front face

		1,7,3,//|
		1,5,7,//|--- Right face

		5,4,6,//|
		5,6,7,//|--- Back face

		4,0,2,//|
		4,2,6,//|--- Left face

		4,1,0,//|
		4,5,1,//|--- Top face

		3,7,6,//|
		2,3,6 //|--- Bottom face
	};

	MeshPC mesh;
	mesh.mVertices.insert(mesh.mVertices.end(), std::begin(mVertices), std::end(mVertices));
	mesh.mIndices.insert(mesh.mIndices.end(), std::begin(mIndices), std::end(mIndices));
	return mesh;
}

MeshPC NFGE::Graphics::MeshBuilder::CreateFlipCubePC(float size)
{
	VertexPC mVertices[] =
	{
		{ { -size,  size, -size },	Colors::Black},
		{ { size,  size, -size },	Colors::Black},
		{ { -size, -size, -size },	Colors::Black},
		{ { size, -size, -size },	Colors::Black},
		{ { -size,  size, size },	Colors::Black},
		{ { size,  size, size },	Colors::Black},
		{ { -size, -size, size },	Colors::Black},
		{ { size, -size, size },	Colors::Black}
	};

	constexpr uint32_t mIndices[] =
	{
		0,2,1,//|
		1,2,3,//|--- Front face

		1,3,7,//|
		1,7,5,//|--- Right face

		5,6,4,//|
		5,7,6,//|--- Back face

		4,2,0,//|
		4,6,2,//|--- Left face

		4,0,1,//|
		4,1,5,//|--- Top face

		3,6,7,//|
		2,6,3 //|--- Bottom face
	};

	MeshPC mesh;
	mesh.mVertices.insert(mesh.mVertices.end(), std::begin(mVertices), std::end(mVertices));
	mesh.mIndices.insert(mesh.mIndices.end(), std::begin(mIndices), std::end(mIndices));
	return mesh;
}

MeshPC NFGE::Graphics::MeshBuilder::CreatePlanePC(int row, int col, float unit)
{
	std::vector<VertexPC> mVertices;
	std::vector<uint32_t> mIndices;

	Math::Vector3 initialPos{ -(col * unit) * 0.5f, (row * unit) * 0.5f, 0.0f };

	for (int i = 0; i < row; i++)
	{
		for (int j = 0; j < col; j++)
		{
			mVertices.push_back({
				{
					initialPos.x + static_cast<float>(j) * unit,
					initialPos.y - static_cast<float>(i) * unit,
					initialPos.z
				}, Colors::Orange });

			if ((i != row - 1) && (j != col - 1))
			{
				mIndices.push_back(static_cast<uint32_t>((i * col + j)));				//|															//           (i*col+j)     ______  (i*col+j+1)              
				mIndices.push_back(static_cast<uint32_t>(((i + 1) * col + j + 1)));		//|--- left triangle										//                        |\    |               
				mIndices.push_back(static_cast<uint32_t>(((i + 1) * col + j)));			//|															//                        | \   | <-- RightTri  
																																					//                        |  \  |               
				mIndices.push_back(static_cast<uint32_t>((i * col + j)));				//|															//            LeftTri --> |   \ |               
				mIndices.push_back(static_cast<uint32_t>((i * col + j + 1)));			//|--- right triangle										//                        |    \|               
				mIndices.push_back(static_cast<uint32_t>(((i + 1) * col + j + 1)));		//|															//        ((i+1)*col+j)   |-----|   ((i+1)*col+j+1)            
			}
		}
	}

	MeshPC mesh;
	mesh.mVertices.insert(mesh.mVertices.end(), std::begin(mVertices), std::end(mVertices));
	mesh.mIndices.insert(mesh.mIndices.end(), std::begin(mIndices), std::end(mIndices));
	return mesh;
}

MeshPC NFGE::Graphics::MeshBuilder::CreateCylinderPC(int row, int col, float radius, float height)
{
	std::vector<VertexPC> mVertices;
	std::vector<uint32_t> mIndices;
	float unitHeight = height / (row - 1);
	float unitAngleRad = -Math::Constants::TwoPi / (col - 1);

	int topPointIndex = row * col;
	int botPointIndex = row * col + 1;

	for (int i = 0; i < row; i++)
	{
		for (int j = 0; j < col; j++)
		{
			mVertices.push_back({
				{
					radius * sin(j * unitAngleRad),
					height * 0.5f - static_cast<float>(i) * unitHeight,
					radius * cos(j * unitAngleRad)
				}, (i == 0 || i == row - 1) ? Colors::PowderBlue : Colors::Orange });

			if ((i == 0) && (j != col - 1))
			{
				mIndices.push_back(static_cast<uint32_t>(topPointIndex));							//|
				mIndices.push_back(static_cast<uint32_t>(0 + j + 1));					//|--- top circle
				mIndices.push_back(static_cast<uint32_t>(0 + j));						//|
			}
			if ((i != row - 1) && (j != col - 1))
			{
				mIndices.push_back(static_cast<uint32_t>((i * col + j)));				//|
				mIndices.push_back(static_cast<uint32_t>(((i + 1) * col + j + 1)));		//|--- left triangle
				mIndices.push_back(static_cast<uint32_t>(((i + 1) * col + j)));			//|

				mIndices.push_back(static_cast<uint32_t>((i * col + j)));				//|
				mIndices.push_back(static_cast<uint32_t>((i * col + j + 1)));			//|--- right triangle
				mIndices.push_back(static_cast<uint32_t>(((i + 1) * col + j + 1)));		//|
			}
			if ((i == row - 1) && (j != col - 1))
			{
				mIndices.push_back(static_cast<uint32_t>(botPointIndex));						//|
				mIndices.push_back(static_cast<uint32_t>(i * col + j));					//|--- bottom circle
				mIndices.push_back(static_cast<uint32_t>(i * col + j + 1));				//|
			}
		}
	}

	mVertices.push_back({
		{
			0.0f,
			height * 0.5f,
			0.0f
		}, Colors::Orange });

	mVertices.push_back({
		{
			0.0f,
			height * 0.5f - static_cast<float>(row - 1) * unitHeight,
			0.0f
		}, Colors::Orange });

	MeshPC mesh;
	mesh.mVertices.insert(mesh.mVertices.end(), std::begin(mVertices), std::end(mVertices));
	mesh.mIndices.insert(mesh.mIndices.end(), std::begin(mIndices), std::end(mIndices));
	return mesh;
}

MeshPC NFGE::Graphics::MeshBuilder::CreateConePC(int row, int col, float radius, float height)
{
	std::vector<VertexPC> mVertices;
	std::vector<uint32_t> mIndices;
	float unitHeight = height / (row - 1);
	float unitAngleRad_hori = -Math::Constants::TwoPi / (col - 1);
	float unitRadius = radius / (row - 1);
	for (int i = 0; i < row; i++)
	{
		for (int j = 0; j < col; j++)
		{
			mVertices.push_back({
				{
					unitRadius * i * sin(j * unitAngleRad_hori),
					height * 0.5f - static_cast<float>(i) * unitHeight,
					unitRadius * i * cos(j * unitAngleRad_hori)
				}, (i == 0 || i == row - 1) ? Colors::PowderBlue : Colors::Orange });

			if ((i != row - 1) && (j != col - 1))
			{
				mIndices.push_back(static_cast<uint32_t>((i * col + j)));				//|
				mIndices.push_back(static_cast<uint32_t>(((i + 1) * col + j + 1)));		//|--- left triangle
				mIndices.push_back(static_cast<uint32_t>(((i + 1) * col + j)));			//|

				mIndices.push_back(static_cast<uint32_t>((i * col + j)));				//|
				mIndices.push_back(static_cast<uint32_t>((i * col + j + 1)));			//|--- right triangle
				mIndices.push_back(static_cast<uint32_t>(((i + 1) * col + j + 1)));		//|
			}
			if ((i == row - 1) && (j != 0) && (j != col - 1))
			{
				mIndices.push_back(static_cast<uint32_t>(i * col));						//|
				mIndices.push_back(static_cast<uint32_t>(i * col + j));					//|--- bottom circle
				mIndices.push_back(static_cast<uint32_t>(i * col + j + 1));				//|
			}
		}
	}

	MeshPC mesh;
	mesh.mVertices.insert(mesh.mVertices.end(), std::begin(mVertices), std::end(mVertices));
	mesh.mIndices.insert(mesh.mIndices.end(), std::begin(mIndices), std::end(mIndices));
	return mesh;
}

MeshPC NFGE::Graphics::MeshBuilder::CreateSpherePC(int row, int col, float radius)
{
	std::vector<VertexPC> mVertices;
	std::vector<uint32_t> mIndices;
	float unitAngleRad_Horiz = -Math::Constants::TwoPi / (col - 1);
	float unitAngleRad_Verti = -Math::Constants::Pi / (row - 1);
	float unitHeight = 2.0f * radius / static_cast<float>(row - 1);
	for (int i = 0; i < row; i++)
	{
		for (int j = 0; j < col; j++)
		{
			mVertices.push_back({
				{
					(radius * sin(i * unitAngleRad_Verti)) * sin(j * unitAngleRad_Horiz),
					(radius * cos(i * unitAngleRad_Verti)),
					(radius * sin(i * unitAngleRad_Verti)) * cos(j * unitAngleRad_Horiz)
				}, (i == 0 || i == row - 1) ? Colors::PowderBlue : Colors::Orange });

			if ((i == 0) && (j != 0) && (j != col - 1))
			{
				mIndices.push_back(static_cast<uint32_t>(0));							//|
				mIndices.push_back(static_cast<uint32_t>(0 + j + 1));					//|--- top circle
				mIndices.push_back(static_cast<uint32_t>(0 + j));						//|
			}
			if ((i != row - 1) && (j != col - 1))
			{
				mIndices.push_back(static_cast<uint32_t>((i * col + j)));				//|
				mIndices.push_back(static_cast<uint32_t>(((i + 1) * col + j + 1)));		//|--- left triangle
				mIndices.push_back(static_cast<uint32_t>(((i + 1) * col + j)));			//|

				mIndices.push_back(static_cast<uint32_t>((i * col + j)));				//|
				mIndices.push_back(static_cast<uint32_t>((i * col + j + 1)));			//|--- right triangle
				mIndices.push_back(static_cast<uint32_t>(((i + 1) * col + j + 1)));		//|
			}
			if ((i == row - 1) && (j != 0) && (j != col - 1))
			{
				mIndices.push_back(static_cast<uint32_t>(i * col));						//|
				mIndices.push_back(static_cast<uint32_t>(i * col + j));					//|--- bottom circle
				mIndices.push_back(static_cast<uint32_t>(i * col + j + 1));				//|
			}
		}
	}

	MeshPC mesh;
	mesh.mVertices.insert(mesh.mVertices.end(), std::begin(mVertices), std::end(mVertices));
	mesh.mIndices.insert(mesh.mIndices.end(), std::begin(mIndices), std::end(mIndices));
	return mesh;
}

MeshPC NFGE::Graphics::MeshBuilder::CreateTorusPC(int row, int col, float innerRadius, float outerRadius)
{
	std::vector<VertexPC> mVertices;
	std::vector<uint32_t> mIndices;

	float cylinderRadius = (outerRadius - innerRadius) * 0.5f;
	float realRadius = innerRadius + cylinderRadius;
	float unitAngleRad_Horiz = Math::Constants::TwoPi / (col - 1);
	float unitAngleRad_Verti = Math::Constants::TwoPi / (row - 1);
	for (int i = 0; i < row; i++)
	{
		for (int j = 0; j < col; j++)
		{
			mVertices.push_back({
				{
					//----------------| sin of current portion of Horiz angle |----------
					//                | give us [0] to [-1] to [0] to [1]     |
					//                v                                       v
					cylinderRadius * sin(j * unitAngleRad_Horiz),

					//--------------------------------|             Length of the Proj of Vertex on RealRadius direction                                    |
					//--------------------------------| RealRadius + (clinderRadius - proj of [Vertex position vector] on local circle on Radius direction) |
					//                                v                                                                                                     v
					sin(unitAngleRad_Verti * i) * (realRadius + cylinderRadius - (cylinderRadius * cos(j * unitAngleRad_Horiz))),
					cos(unitAngleRad_Verti * i) * (realRadius + cylinderRadius - (cylinderRadius * cos(j * unitAngleRad_Horiz)))
				}, (i == 0 || i == row - 1) ? Colors::PowderBlue : Colors::Orange });

			if ((i == 0) && (j != 0) && (j != col - 1))
			{
				mIndices.push_back(static_cast<uint32_t>(0));							//|
				mIndices.push_back(static_cast<uint32_t>(0 + j + 1));					//|--- top circle
				mIndices.push_back(static_cast<uint32_t>(0 + j));						//|
			}
			if ((i != row - 1) && (j != col - 1))
			{
				mIndices.push_back(static_cast<uint32_t>((i * col + j)));				//|
				mIndices.push_back(static_cast<uint32_t>(((i + 1) * col + j)));			//|
				mIndices.push_back(static_cast<uint32_t>(((i + 1) * col + j + 1)));		//|--- left triangle

				mIndices.push_back(static_cast<uint32_t>((i * col + j)));				//|
				mIndices.push_back(static_cast<uint32_t>(((i + 1) * col + j + 1)));		//|
				mIndices.push_back(static_cast<uint32_t>((i * col + j + 1)));			//|--- right triangle
			}
			if ((i == row - 1) && (j != 0) && (j != col - 1))
			{
				mIndices.push_back(static_cast<uint32_t>(i * col));						//|
				mIndices.push_back(static_cast<uint32_t>(i * col + j));					//|--- bottom circle
				mIndices.push_back(static_cast<uint32_t>(i * col + j + 1));				//|
			}
		}
	}

	MeshPC mesh;
	mesh.mVertices.insert(mesh.mVertices.end(), std::begin(mVertices), std::end(mVertices));
	mesh.mIndices.insert(mesh.mIndices.end(), std::begin(mIndices), std::end(mIndices));
	return mesh;
}

MeshPX NFGE::Graphics::MeshBuilder::CreateCubePX()
{

	//		  4	 ______________________ 5  
	//			|\				       \
	//			| \				        \
	//		  6	|  \			         \---->7
	//			\   \_0___________________\ 1  
	//			 \  |					  |
	//			  \ |					  |
	//			  2\|_____________________| 3
	constexpr VertexPX mVertices[] =
	{
		{{ -1.0f,  1.0f, -1.0f }, {0.0f,0.0f} },	//--- 0 --- 0
		{{  1.0f,  1.0f, -1.0f }, {1.0f,0.0f} },	//--- 1 --- 1
		{{ -1.0f, -1.0f, -1.0f }, {0.0f,1.0f} },	//--- 2 --- 2
		{{  1.0f, -1.0f, -1.0f }, {1.0f,1.0f} },	//--- 3 --- 3

		{{  1.0f,  1.0f, -1.0f }, {0,0} },	//--- 1 --- 4
		{{  1.0f,  1.0f, 1.0f },  {1,0} },	//--- 5 --- 5
		{{  1.0f, -1.0f, -1.0f }, {0,1} },	//--- 3 --- 6
		{{  1.0f, -1.0f, 1.0f },  {1,1} },	//--- 7 --- 7

		{{  1.0f,  1.0f, 1.0f },  {0,0} },	//--- 5 --- 8
		{{ -1.0f,  1.0f, 1.0f },  {1,0} },	//--- 4 --- 9
		{{  1.0f, -1.0f, 1.0f },  {0,1} },	//--- 7 --- 10
		{{ -1.0f, -1.0f, 1.0f },  {1,1} },	//--- 6 --- 11

		{{ -1.0f,  1.0f, 1.0f },  {0,0} },	//--- 4 --- 12
		{{ -1.0f,  1.0f, -1.0f }, {0,1} },	//--- 0 --- 13
		{{ -1.0f, -1.0f, 1.0f },  {1,0} },	//--- 6 --- 14
		{{ -1.0f, -1.0f, -1.0f }, {1,1} },	//--- 2 --- 15

		{{ -1.0f,  1.0f, 1.0f },  {0,0} },	//--- 4 --- 16
		{{  1.0f,  1.0f, 1.0f },  {0,1} },	//--- 5 --- 17
		{{ -1.0f,  1.0f, -1.0f }, {1,0} },	//--- 0 --- 18
		{{  1.0f,  1.0f, -1.0f }, {1,1} },	//--- 1 --- 19

		{{  1.0f, -1.0f, -1.0f }, {0,0} },	//--- 3 --- 20
		{{ -1.0f, -1.0f, -1.0f }, {0,1} },	//--- 2 --- 21
		{{  1.0f, -1.0f, 1.0f },  {1,0} },	//--- 7 --- 22
		{{ -1.0f, -1.0f, 1.0f },  {1,1} },	//--- 6 --- 23
	};

	constexpr uint32_t mIndices[] =
	{
		0,1,2,		//|
		1,3,2,		//|--- Front face

		4,5,6,		//|
		5,7,6,		//|--- Right face

		8,9,10,		//|
		9,11,10,	//|--- Back face

		12,13,14,	//|
		13,15,14,	//|--- Left face

		16,17,18,	//|
		17,19,18,	//|--- Top face

		21,20,22,	//|
		21,22,23	//|--- Bottom face
	};

	MeshPX mesh;
	mesh.mVertices.insert(mesh.mVertices.end(), std::begin(mVertices), std::end(mVertices));
	mesh.mIndices.insert(mesh.mIndices.end(), std::begin(mIndices), std::end(mIndices));
	return mesh;
}


MeshPX NFGE::Graphics::MeshBuilder::CreatePlanePX()
{
	VertexPX mVertices[] =
	{
		{{-1.0f,1.0f,0.0f},{0.0f,0.0f}},
		{ { 1.0f,1.0f,0.0f },{ 1.0f,0.0f } },
		{ { 1.0f,-1.0f,0.0f },{ 1.0f,1.0f } },
		{ { -1.0f,-1.0f,0.0f },{ 0.0f,1.0f } }
	};
	uint32_t mIndices[] =
	{
		0,1,2,0,2,3
	};



	MeshPX mesh;
	mesh.mVertices.insert(mesh.mVertices.end(), std::begin(mVertices), std::end(mVertices));
	mesh.mIndices.insert(mesh.mIndices.end(), std::begin(mIndices), std::end(mIndices));
	return mesh;
}

MeshPX NFGE::Graphics::MeshBuilder::CreatePlanePX(int row, int col, float unit)
{
	std::vector<VertexPX> mVertices;
	std::vector<uint32_t> mIndices;

	Math::Vector3 initialPos{ -(col * unit) * 0.5f, (row * unit) * 0.5f, 0.0f };
	float uUnit = 1.0f / (col - 1);
	float vUnit = 1.0f / (row - 1);

	for (int i = 0; i < row; i++)
	{
		for (int j = 0; j < col; j++)
		{
			mVertices.push_back({
				{
					initialPos.x + static_cast<float>(j) * unit,
					initialPos.y - static_cast<float>(i) * unit,
					initialPos.z
				}, {(float)j * uUnit , (float)i * vUnit} });

			if ((i != row - 1) && (j != col - 1))
			{
				mIndices.push_back(static_cast<uint32_t>((i * col + j)));				//|															//           (i*col+j)     ______  (i*col+j+1)              
				mIndices.push_back(static_cast<uint32_t>(((i + 1) * col + j + 1)));		//|--- left triangle										//                        |\    |               
				mIndices.push_back(static_cast<uint32_t>(((i + 1) * col + j)));			//|															//                        | \   | <-- RightTri  
																																					//                        |  \  |               
				mIndices.push_back(static_cast<uint32_t>((i * col + j)));				//|															//            LeftTri --> |   \ |               
				mIndices.push_back(static_cast<uint32_t>((i * col + j + 1)));			//|--- right triangle										//                        |    \|               
				mIndices.push_back(static_cast<uint32_t>(((i + 1) * col + j + 1)));		//|															//        ((i+1)*col+j)   |-----|   ((i+1)*col+j+1)            
			}
		}
	}

	MeshPX mesh;
	mesh.mVertices.insert(mesh.mVertices.end(), std::begin(mVertices), std::end(mVertices));
	mesh.mIndices.insert(mesh.mIndices.end(), std::begin(mIndices), std::end(mIndices));
	return mesh;
}

MeshPX NFGE::Graphics::MeshBuilder::CreateSpherePX(int row, int col, float radius)
{
	std::vector<VertexPX> mVertices;
	std::vector<uint32_t> mIndices;
	float unitAngleRad_Horiz = -Math::Constants::TwoPi / (col - 1);
	float unitAngleRad_Verti = -Math::Constants::Pi / (row - 1);
	float unitHeight = 2.0f * radius / static_cast<float>(row - 1);
	float uUnit = 1.0f / (col - 1);
	float vUnit = 1.0f / (row - 1);

	for (int i = 0; i < row; i++)
	{
		for (int j = 0; j < col; j++)
		{
			mVertices.push_back({
				{
					(radius * sin(i * unitAngleRad_Verti)) * sin(j * unitAngleRad_Horiz),
					(radius * cos(i * unitAngleRad_Verti)),
					(radius * sin(i * unitAngleRad_Verti)) * cos(j * unitAngleRad_Horiz)
				},{ (float)j * uUnit , (float)i * vUnit } });

			if ((i == 0) && (j != 0) && (j != col - 1))
			{
				mIndices.push_back(static_cast<uint32_t>(0));							//|
				mIndices.push_back(static_cast<uint32_t>(0 + j + 1));					//|--- top circle
				mIndices.push_back(static_cast<uint32_t>(0 + j));						//|
			}
			if ((i != row - 1) && (j != col - 1))
			{
				mIndices.push_back(static_cast<uint32_t>((i * col + j)));				//|
				mIndices.push_back(static_cast<uint32_t>(((i + 1) * col + j + 1)));		//|--- left triangle
				mIndices.push_back(static_cast<uint32_t>(((i + 1) * col + j)));			//|

				mIndices.push_back(static_cast<uint32_t>((i * col + j)));				//|
				mIndices.push_back(static_cast<uint32_t>((i * col + j + 1)));			//|--- right triangle
				mIndices.push_back(static_cast<uint32_t>(((i + 1) * col + j + 1)));		//|
			}
			if ((i == row - 1) && (j != 0) && (j != col - 1))
			{
				mIndices.push_back(static_cast<uint32_t>(i * col));						//|
				mIndices.push_back(static_cast<uint32_t>(i * col + j));					//|--- bottom circle
				mIndices.push_back(static_cast<uint32_t>(i * col + j + 1));				//|
			}
		}
	}

	MeshPX mesh;
	mesh.mVertices.insert(mesh.mVertices.end(), std::begin(mVertices), std::end(mVertices));
	mesh.mIndices.insert(mesh.mIndices.end(), std::begin(mIndices), std::end(mIndices));
	return mesh;
}

MeshPX NFGE::Graphics::MeshBuilder::CreateTorusPX(int row, int col, float innerRadius, float outerRadius)
{
	std::vector<VertexPX> mVertices;
	std::vector<uint32_t> mIndices;

	float cylinderRadius = (outerRadius - innerRadius) * 0.5f;
	float realRadius = innerRadius + cylinderRadius;
	float unitAngleRad_Horiz = Math::Constants::TwoPi / (col - 1);
	float unitAngleRad_Verti = Math::Constants::TwoPi / (row - 1);
	float uUnit = 1.0f / (col - 1);
	float vUnit = 1.0f / (row - 1);

	for (int i = 0; i < row; i++)
	{
		for (int j = 0; j < col; j++)
		{
			mVertices.push_back({
				{
					//----------------| sin of current portion of Horiz angle |----------
					//                | give us [0] to [-1] to [0] to [1]     |
					//                v                                       v
					cylinderRadius * sin(j * unitAngleRad_Horiz),

					//--------------------------------|             Length of the Proj of Vertex on RealRadius direction                                    |
					//--------------------------------| RealRadius + (clinderRadius - proj of [Vertex position vector] on local circle on Radius direction) |
					//                                v                                                                                                     v
					-sin(unitAngleRad_Verti * i) * (realRadius + cylinderRadius - (cylinderRadius * cos(j * unitAngleRad_Horiz))),
					cos(unitAngleRad_Verti * i) * (realRadius + cylinderRadius - (cylinderRadius * cos(j * unitAngleRad_Horiz)))
				}, { (float)j * uUnit , (float)i * vUnit } });

			if ((i == 0) && (j != 0) && (j != col - 1))
			{
				mIndices.push_back(static_cast<uint32_t>(0));							//|
				mIndices.push_back(static_cast<uint32_t>(0 + j + 1));					//|--- top circle
				mIndices.push_back(static_cast<uint32_t>(0 + j));						//|
			}
			if ((i != row - 1) && (j != col - 1))
			{
				mIndices.push_back(static_cast<uint32_t>((i * col + j)));				//|
				mIndices.push_back(static_cast<uint32_t>(((i + 1) * col + j + 1)));		//|--- left triangle
				mIndices.push_back(static_cast<uint32_t>(((i + 1) * col + j)));			//|

				mIndices.push_back(static_cast<uint32_t>((i * col + j)));				//|
				mIndices.push_back(static_cast<uint32_t>((i * col + j + 1)));			//|--- right triangle
				mIndices.push_back(static_cast<uint32_t>(((i + 1) * col + j + 1)));		//|
			}
			if ((i == row - 1) && (j != 0) && (j != col - 1))
			{
				mIndices.push_back(static_cast<uint32_t>(i * col));						//|
				mIndices.push_back(static_cast<uint32_t>(i * col + j));					//|--- bottom circle
				mIndices.push_back(static_cast<uint32_t>(i * col + j + 1));				//|
			}
		}
	}

	MeshPX mesh;
	mesh.mVertices.insert(mesh.mVertices.end(), std::begin(mVertices), std::end(mVertices));
	mesh.mIndices.insert(mesh.mIndices.end(), std::begin(mIndices), std::end(mIndices));
	return mesh;
}

MeshPX NFGE::Graphics::MeshBuilder::CreateNDCQuad()
{
	MeshPX mesh;
	//
	// (-1,+1)----------------------------(+1,+1)
	//    |             ^                    |
	//	  |             +->                  |
	//	  |									 |
	// (-1,-1)----------------------------(+1,-1)

	mesh.mVertices.emplace_back(VertexPX{ {-1.0f,-1.0f,0.0f},{0.0f, 1.0f} });
	mesh.mVertices.emplace_back(VertexPX{ {-1.0f,+1.0f,0.0f},{0.0f, 0.0f} });
	mesh.mVertices.emplace_back(VertexPX{ {+1.0f,+1.0f,0.0f},{1.0f, 0.0f} });
	mesh.mVertices.emplace_back(VertexPX{ {+1.0f,-1.0f,0.0f},{1.0f, 1.0f} });

	mesh.mIndices.emplace_back(0);
	mesh.mIndices.emplace_back(1);
	mesh.mIndices.emplace_back(2);

	mesh.mIndices.emplace_back(0);
	mesh.mIndices.emplace_back(2);
	mesh.mIndices.emplace_back(3);

	return mesh;
}

MeshPN NFGE::Graphics::MeshBuilder::CreateSpherePN(int row, int col, float radius)
{
	std::vector<VertexPN> mVertices;
	std::vector<uint32_t> mIndices;
	float unitAngleRad_Horiz = -Math::Constants::TwoPi / (col - 1);
	float unitAngleRad_Verti = -Math::Constants::Pi / (row - 1);
	float unitHeight = 2.0f * radius / static_cast<float>(row - 1);
	float uUnit = 1.0f / (col - 1);
	float vUnit = 1.0f / (row - 1);

	for (int i = 0; i < row; i++)
	{
		for (int j = 0; j < col; j++)
		{
			Math::Vector3 pos
			{
				(radius * sin(i * unitAngleRad_Verti)) * sin(j * unitAngleRad_Horiz),
				(radius * cos(i * unitAngleRad_Verti)),
				(radius * sin(i * unitAngleRad_Verti)) * cos(j * unitAngleRad_Horiz)
			};
			mVertices.push_back(
				{
					pos,
					Math::Normalize(pos)
				});

			if ((i == 0) && (j != 0) && (j != col - 1))
			{
				mIndices.push_back(static_cast<uint32_t>(0));							//|
				mIndices.push_back(static_cast<uint32_t>(0 + j + 1));					//|--- top circle
				mIndices.push_back(static_cast<uint32_t>(0 + j));						//|
			}
			if ((i != row - 1) && (j != col - 1))
			{
				mIndices.push_back(static_cast<uint32_t>((i * col + j)));				//|
				mIndices.push_back(static_cast<uint32_t>(((i + 1) * col + j + 1)));		//|--- left triangle
				mIndices.push_back(static_cast<uint32_t>(((i + 1) * col + j)));			//|

				mIndices.push_back(static_cast<uint32_t>((i * col + j)));				//|
				mIndices.push_back(static_cast<uint32_t>((i * col + j + 1)));			//|--- right triangle
				mIndices.push_back(static_cast<uint32_t>(((i + 1) * col + j + 1)));		//|
			}
			if ((i == row - 1) && (j != 0) && (j != col - 1))
			{
				mIndices.push_back(static_cast<uint32_t>(i * col));						//|
				mIndices.push_back(static_cast<uint32_t>(i * col + j));					//|--- bottom circle
				mIndices.push_back(static_cast<uint32_t>(i * col + j + 1));				//|
			}
		}
	}

	MeshPN mesh;
	mesh.mVertices.insert(mesh.mVertices.end(), std::begin(mVertices), std::end(mVertices));
	mesh.mIndices.insert(mesh.mIndices.end(), std::begin(mIndices), std::end(mIndices));
	return mesh;
}

MeshPN NFGE::Graphics::MeshBuilder::CreateConePN(int row, int col, float radius, float height)
{
	std::vector<VertexPN> mVertices;
	std::vector<uint32_t> mIndices;
	float unitHeight = height / (row - 1);
	float unitAngleRad_hori = -Math::Constants::TwoPi / (col - 1);
	float unitRadius = radius / (row - 1);
	row++; // for bottom layer mVertices which gonna have two on same position for different normal
	uint32_t lastIndex = static_cast<uint32_t>(row * col);
	for (int i = 0; i < row; i++)
	{
		for (int j = 0; j < col; j++)
		{
			Math::Vector3 pos;
			if (i != row - 1)
			{
				pos =
				{
					unitRadius * i * sin(j * unitAngleRad_hori),
					height * 0.5f - static_cast<float>(i) * unitHeight,
					unitRadius * i * cos(j * unitAngleRad_hori)
				};
			}
			else
			{
				pos =
				{
					unitRadius * (i - 1) * sin(j * unitAngleRad_hori),
					height * 0.5f - static_cast<float>(i - 1) * unitHeight,
					unitRadius * (i - 1) * cos(j * unitAngleRad_hori)
				};
			}

			Math::Vector3 tangent = Math::Normalize({ -pos.z, 0.0f, pos.x });
			Math::Vector3 normal;
			if (i == 0)
			{
				normal = { 0.0f, 1.0f, 0.0f };
			}
			else if (i != row - 1)
			{
				normal = Math::Normalize(Math::Cross(mVertices[0].position - pos, tangent));
			}
			else
			{
				normal = { 0.0f, -1.0f, 0.0f };
			}

			mVertices.push_back({
				pos,
				normal });

			if ((i != row - 1) && (j != col - 1))
			{
				mIndices.push_back(static_cast<uint32_t>((i * col + j)));				//|
				mIndices.push_back(static_cast<uint32_t>(((i + 1) * col + j + 1)));		//|--- left triangle
				mIndices.push_back(static_cast<uint32_t>(((i + 1) * col + j)));			//|

				mIndices.push_back(static_cast<uint32_t>((i * col + j)));				//|
				mIndices.push_back(static_cast<uint32_t>((i * col + j + 1)));			//|--- right triangle
				mIndices.push_back(static_cast<uint32_t>(((i + 1) * col + j + 1)));		//|
			}
			if ((i == row - 1) && (j != col - 1))
			{
				mIndices.push_back(static_cast<uint32_t>((i - 1) * col + j));					//|
				mIndices.push_back(static_cast<uint32_t>((i - 1) * col + j + 1));				//|--- bottom circle
				mIndices.push_back(lastIndex);													//|
			}
		}
	}
	//--------------------------------------------------------------------------//|
	mVertices.push_back({														//|
		{																		//|
			0.0f,																//|
			height * 0.5f - static_cast<float>(row - 2) * unitHeight,			//|--- push back the last mVertices
			0.0f																//|
		},																		//|
		{0.0f,-1.0f,0.0f} });													//|
	//--------------------------------------------------------------------------//|

	MeshPN mesh;
	mesh.mVertices.insert(mesh.mVertices.end(), std::begin(mVertices), std::end(mVertices));
	mesh.mIndices.insert(mesh.mIndices.end(), std::begin(mIndices), std::end(mIndices));
	return mesh;
}

Mesh NFGE::Graphics::MeshBuilder::CreatePlane(int row, int col, float unit)
{
	std::vector<Vertex> mVertices;
	std::vector<uint32_t> mIndices;

	Math::Vector3 initialPos{ -(col * unit) * 0.5f, (row * unit) * 0.5f, 0.0f };
	float uUnit = 1.0f / (col - 1);
	float vUnit = 1.0f / (row - 1);

	for (int i = 0; i < row; i++)
	{
		for (int j = 0; j < col; j++)
		{
			mVertices.push_back({
				{
					initialPos.x + static_cast<float>(j) * unit,
					initialPos.y - static_cast<float>(i) * unit,
					initialPos.z
				},
				{
					0.0f,0.0f,-1.0f
				},
				{
					1.0f,0.0f,0.0f
				},

				{ (float)(j) , (float)(i)}
				});

			if ((i != row - 1) && (j != col - 1))
			{
				mIndices.push_back(static_cast<uint32_t>((i * col + j)));				//|															//           (i*col+j)     ______  (i*col+j+1)              
				mIndices.push_back(static_cast<uint32_t>(((i + 1) * col + j + 1)));		//|--- left triangle										//                        |\    |               
				mIndices.push_back(static_cast<uint32_t>(((i + 1) * col + j)));			//|															//                        | \   | <-- RightTri  
																																					//                        |  \  |               
				mIndices.push_back(static_cast<uint32_t>((i * col + j)));				//|															//            LeftTri --> |   \ |               
				mIndices.push_back(static_cast<uint32_t>((i * col + j + 1)));			//|--- right triangle										//                        |    \|               
				mIndices.push_back(static_cast<uint32_t>(((i + 1) * col + j + 1)));		//|															//        ((i+1)*col+j)   |-----|   ((i+1)*col+j+1)            
			}
		}
	}

	Mesh mesh;
	mesh.mVertices.insert(mesh.mVertices.end(), std::begin(mVertices), std::end(mVertices));
	mesh.mIndices.insert(mesh.mIndices.end(), std::begin(mIndices), std::end(mIndices));
	return mesh;
}

Mesh NFGE::Graphics::MeshBuilder::CreateCube()
{
	//		  4	 ______________________ 5  
	//			|\				       \
	//			| \				        \
	//		  6	|  \			         \---->7
	//			\   \_0___________________\ 1  
	//			 \  |					  |
	//			  \ |					  |
	//			  2\|_____________________| 3
	constexpr Vertex mVertices[] =
	{
		{{ -1.0f,  1.0f, -1.0f }, {0.0f, 0.0f, -1.0f}, {-1.0f,0.0f,0.0f}, {0.0f,0.0f} },	//--- 0 --- 0
		{{  1.0f,  1.0f, -1.0f }, {0.0f, 0.0f, -1.0f}, {-1.0f,0.0f,0.0f}, {1.0f,0.0f} },	//--- 1 --- 1
		{{ -1.0f, -1.0f, -1.0f }, {0.0f, 0.0f, -1.0f}, {-1.0f,0.0f,0.0f}, {0.0f,1.0f} },	//--- 2 --- 2
		{{  1.0f, -1.0f, -1.0f }, {0.0f, 0.0f, -1.0f}, {-1.0f,0.0f,0.0f}, {1.0f,1.0f} },	//--- 3 --- 3

		{{  1.0f,  1.0f, -1.0f }, {1.0f, 0.0f, 0.0f}, {0.0f,0.0f,1.0f}, {0,0} },	//--- 1 --- 4
		{{  1.0f,  1.0f, 1.0f },  {1.0f, 0.0f, 0.0f}, {0.0f,0.0f,1.0f}, {1,0} },	//--- 5 --- 5
		{{  1.0f, -1.0f, -1.0f }, {1.0f, 0.0f, 0.0f}, {0.0f,0.0f,1.0f}, {0,1} },	//--- 3 --- 6
		{{  1.0f, -1.0f, 1.0f },  {1.0f, 0.0f, 0.0f}, {0.0f,0.0f,1.0f}, {1,1} },	//--- 7 --- 7

		{{  1.0f,  1.0f, 1.0f },  {0.0f, 0.0f, 1.0f}, {1.0f,0.0f,0.0f}, {0,0} },	//--- 5 --- 8
		{{ -1.0f,  1.0f, 1.0f },  {0.0f, 0.0f, 1.0f}, {1.0f,0.0f,0.0f}, {1,0} },	//--- 4 --- 9
		{{  1.0f, -1.0f, 1.0f },  {0.0f, 0.0f, 1.0f}, {1.0f,0.0f,0.0f}, {0,1} },	//--- 7 --- 10
		{{ -1.0f, -1.0f, 1.0f },  {0.0f, 0.0f, 1.0f}, {1.0f,0.0f,0.0f}, {1,1} },	//--- 6 --- 11

		{{ -1.0f,  1.0f, 1.0f },  {-1.0f, 0.0f, 0.0f}, {0.0f,0.0f,1.0f}, {0,0} },	//--- 4 --- 12
		{{ -1.0f,  1.0f, -1.0f }, {-1.0f, 0.0f, 0.0f}, {0.0f,0.0f,1.0f}, {0,1} },	//--- 0 --- 13
		{{ -1.0f, -1.0f, 1.0f },  {-1.0f, 0.0f, 0.0f}, {0.0f,0.0f,1.0f}, {1,0} },	//--- 6 --- 14
		{{ -1.0f, -1.0f, -1.0f }, {-1.0f, 0.0f, 0.0f}, {0.0f,0.0f,1.0f}, {1,1} },	//--- 2 --- 15

		{{ -1.0f,  1.0f, 1.0f },  {0.0f, 1.0f, 0.0f}, {1.0f,0.0f,0.0f}, {0,0} },	//--- 4 --- 16
		{{  1.0f,  1.0f, 1.0f },  {0.0f, 1.0f, 0.0f}, {1.0f,0.0f,0.0f}, {0,1} },	//--- 5 --- 17
		{{ -1.0f,  1.0f, -1.0f }, {0.0f, 1.0f, 0.0f}, {1.0f,0.0f,0.0f}, {1,0} },	//--- 0 --- 18
		{{  1.0f,  1.0f, -1.0f }, {0.0f, 1.0f, 0.0f}, {1.0f,0.0f,0.0f}, {1,1} },	//--- 1 --- 19

		{{  1.0f, -1.0f, -1.0f }, {0.0f, -1.0f, 0.0f}, {-1.0f,0.0f,0.0f}, {0,0} },	//--- 3 --- 20
		{{ -1.0f, -1.0f, -1.0f }, {0.0f, -1.0f, 0.0f}, {-1.0f,0.0f,0.0f}, {0,1} },	//--- 2 --- 21
		{{  1.0f, -1.0f, 1.0f },  {0.0f, -1.0f, 0.0f}, {-1.0f,0.0f,0.0f}, {1,0} },	//--- 7 --- 22
		{{ -1.0f, -1.0f, 1.0f },  {0.0f, -1.0f, 0.0f}, {-1.0f,0.0f,0.0f}, {1,1} },	//--- 6 --- 23
	};

	constexpr uint32_t mIndices[] =
	{
		0,1,2,		//|
		1,3,2,		//|--- Front face

		4,5,6,		//|
		5,7,6,		//|--- Right face

		8,9,10,		//|
		9,11,10,	//|--- Back face

		12,13,14,	//|
		13,15,14,	//|--- Left face

		16,17,18,	//|
		17,19,18,	//|--- Top face

		21,20,22,	//|
		21,22,23	//|--- Bottom face
	};

	Mesh mesh;
	mesh.mVertices.insert(mesh.mVertices.end(), std::begin(mVertices), std::end(mVertices));
	mesh.mIndices.insert(mesh.mIndices.end(), std::begin(mIndices), std::end(mIndices));
	return mesh;
}

Mesh NFGE::Graphics::MeshBuilder::CreateCone(int row, int col, float radius, float height)
{
	std::vector<Vertex> mVertices;
	std::vector<uint32_t> mIndices;
	float unitHeight = height / (row - 1);
	float unitAngleRad_hori = -Math::Constants::TwoPi / (col - 1);
	float unitRadius = radius / (row - 1);
	row++; // for bottom layer mVertices which gonna have two on same position for different normal
	float uUnit = 1.0f / (col - 1);
	float vUnit = 1.0f / (row - 1);
	uint32_t lastIndex = static_cast<uint32_t>(row * col);
	for (int i = 0; i < row; i++)
	{
		for (int j = 0; j < col; j++)
		{
			Math::Vector3 pos;
			if (i != row - 1)
			{
				pos =
				{
					unitRadius * i * sin(j * unitAngleRad_hori),
					height * 0.5f - static_cast<float>(i) * unitHeight,
					unitRadius * i * cos(j * unitAngleRad_hori)
				};
			}
			else
			{
				pos =
				{
					unitRadius * (i - 1) * sin(j * unitAngleRad_hori),
					height * 0.5f - static_cast<float>(i - 1) * unitHeight,
					unitRadius * (i - 1) * cos(j * unitAngleRad_hori)
				};
			}

			Math::Vector3 tangent;
			Math::Vector3 normal;
			if (i == 0)
			{
				normal = Math::Normalize({ sin(unitAngleRad_hori), 0.0f, cos(j * unitAngleRad_hori) }); // Assign normal to the top points
				tangent = { -normal.z, 0.0f, -normal.x };
			}
			else if (i != row - 1)
			{
				normal = Math::Normalize({ pos.x, 0.0f, pos.z });
				tangent = { -normal.z, 0.0f, -normal.x };
			}
			else
			{
				normal = { 0.0f, -1.0f, 0.0f };
				tangent = { 1.0f, 0.0f, 0.0f };
			}

			mVertices.push_back({
				pos,
				normal,
				tangent,
				{ (float)j * uUnit , (float)i * vUnit }
				});

			if ((i != row - 1) && (j != col - 1))
			{
				mIndices.push_back(static_cast<uint32_t>((i * col + j)));				//|
				mIndices.push_back(static_cast<uint32_t>(((i + 1) * col + j + 1)));		//|--- left triangle
				mIndices.push_back(static_cast<uint32_t>(((i + 1) * col + j)));			//|

				mIndices.push_back(static_cast<uint32_t>((i * col + j)));				//|
				mIndices.push_back(static_cast<uint32_t>((i * col + j + 1)));			//|--- right triangle
				mIndices.push_back(static_cast<uint32_t>(((i + 1) * col + j + 1)));		//|
			}
			if ((i == row - 1) && (j != col - 1))
			{
				mIndices.push_back(static_cast<uint32_t>((i - 1) * col + j));					//|
				mIndices.push_back(static_cast<uint32_t>((i - 1) * col + j + 1));				//|--- bottom circle
				mIndices.push_back(lastIndex);													//|
			}
		}
	}
	//--------------------------------------------------------------------------//|
	mVertices.push_back({														//|
		{																		//|
			0.0f,																//|
			height * 0.5f - static_cast<float>(row - 2) * unitHeight,			//|--- push back the last mVertices
			0.0f																//|
		},																		//|
		{ 0.0f,-1.0f,0.0f } });													//|
																				//--------------------------------------------------------------------------//|

	Mesh mesh;
	mesh.mVertices.insert(mesh.mVertices.end(), std::begin(mVertices), std::end(mVertices));
	mesh.mIndices.insert(mesh.mIndices.end(), std::begin(mIndices), std::end(mIndices));
	return mesh;
}

Mesh NFGE::Graphics::MeshBuilder::CreateCylinder(int row, int col, float radius, float height)
{
	std::vector<Vertex> mVertices;
	std::vector<uint32_t> mIndices;
	float unitHeight = height / (row - 1);
	float unitAngleRad = -Math::Constants::TwoPi / (col - 1);
	float uUnit = 1.0f / (col - 1);
	float vUnit = 1.0f / (row - 1);

	int topPointIndex = row * col;
	int botPointIndex = row * col + 1;

	for (int i = 0; i < row; i++)
	{
		for (int j = 0; j < col; j++)
		{
			Math::Vector3 pos;
			if (i == 0)
			{
				pos =
				{
					radius * sin(j * unitAngleRad),
					height * 0.5f - static_cast<float>(i + 1) * unitHeight,
					radius * i * cos(j * unitAngleRad)
				};
			}
			else if (i != row - 1)
			{
				pos =
				{
					radius * sin(j * unitAngleRad),
					height * 0.5f - static_cast<float>(i) * unitHeight,
					radius * i * cos(j * unitAngleRad)
				};
			}
			else
			{
				pos =
				{
					radius * sin(j * unitAngleRad),
					height * 0.5f - static_cast<float>(i - 1) * unitHeight,
					radius * cos(j * unitAngleRad)
				};
			}

			Math::Vector3 tangent;
			Math::Vector3 normal;

			if (i == 0)
			{
				normal = { 0.0f, 1.0f, 0.0f };
				tangent = { -1.0f, 0.0f, 0.0f };
			}
			else if (i != row - 1)
			{
				normal = Math::Normalize({ pos.x, 0.0f, pos.z });
				tangent = { -normal.z, 0.0f, -normal.x };
			}
			else
			{
				normal = { 0.0f, -1.0f, 0.0f };
				tangent = { 1.0f, 0.0f, 0.0f };
			}

			mVertices.push_back({
				pos,
				normal,
				tangent,
				{ (float)j * uUnit , (float)i * vUnit }
				});

			if ((i == 0) && (j != col - 1))
			{
				mIndices.push_back(static_cast<uint32_t>(topPointIndex));							//|
				mIndices.push_back(static_cast<uint32_t>(0 + j + 1));					//|--- top circle
				mIndices.push_back(static_cast<uint32_t>(0 + j));						//|
			}
			if ((i != 0) && (i != row - 1) && (j != col - 1))
			{
				mIndices.push_back(static_cast<uint32_t>((i * col + j)));				//|
				mIndices.push_back(static_cast<uint32_t>(((i + 1) * col + j + 1)));		//|--- left triangle
				mIndices.push_back(static_cast<uint32_t>(((i + 1) * col + j)));			//|

				mIndices.push_back(static_cast<uint32_t>((i * col + j)));				//|
				mIndices.push_back(static_cast<uint32_t>((i * col + j + 1)));			//|--- right triangle
				mIndices.push_back(static_cast<uint32_t>(((i + 1) * col + j + 1)));		//|
			}
			if ((i == row - 1) && (j != col - 1))
			{
				mIndices.push_back(static_cast<uint32_t>(botPointIndex));						//|
				mIndices.push_back(static_cast<uint32_t>(i * col + j));					//|--- bottom circle
				mIndices.push_back(static_cast<uint32_t>(i * col + j + 1));				//|
			}
		}
	}

	mVertices.push_back({
		{
			0.0f,
			height * 0.5f,
			0.0f
		},
		{0.0f,1.0f,0.0f},
		{-1.0f,0.0f,0.0f},
		{0.0f,0.0f}
		});

	mVertices.push_back({
		{
			0.0f,
			height * 0.5f - static_cast<float>(row - 1) * unitHeight,
			0.0f
		},
		{ 0.0f,1.0f,0.0f },
		{ -1.0f,0.0f,0.0f },
		{ 0.0f,0.0f }
		});

	Mesh mesh;
	mesh.mVertices.insert(mesh.mVertices.end(), std::begin(mVertices), std::end(mVertices));
	mesh.mIndices.insert(mesh.mIndices.end(), std::begin(mIndices), std::end(mIndices));
	return mesh;
}

Mesh NFGE::Graphics::MeshBuilder::CreateSphere(int row, int col, float radius)
{
	std::vector<Vertex> mVertices;
	std::vector<uint32_t> mIndices;
	float unitAngleRad_Horiz = -Math::Constants::TwoPi / (col - 1);
	float unitAngleRad_Verti = -Math::Constants::Pi / (row - 1);
	float unitHeight = 2.0f * radius / static_cast<float>(row - 1);
	float uUnit = 1.0f / (col - 1);
	float vUnit = 1.0f / (row - 1);

	for (int i = 0; i < row; i++)
	{
		for (int j = 0; j < col; j++)
		{
			Math::Vector3 pos
			{
				(sin(i * unitAngleRad_Verti)) * sin(j * unitAngleRad_Horiz),
				(cos(i * unitAngleRad_Verti)),
				(sin(i * unitAngleRad_Verti)) * cos(j * unitAngleRad_Horiz)
			};

			Math::Vector3 tangent;
			if (i == 0)
			{
				tangent = { 1.0f, 0.0f, 0.0f };
			}
			else if (i != row - 1)
			{
				tangent = Math::Normalize({ -pos.z, 0.0f, pos.x });
			}
			else
			{
				tangent = { 1.0f, 0.0f, 0.0f };
			}

			mVertices.push_back(
				{
					pos * radius,
					pos,
					tangent,
					{ (float)j * uUnit , (float)i * vUnit }
				});

			if ((i == 0) && (j != 0) && (j != col - 1))
			{
				mIndices.push_back(static_cast<uint32_t>(0));							//|
				mIndices.push_back(static_cast<uint32_t>(0 + j + 1));					//|--- top circle
				mIndices.push_back(static_cast<uint32_t>(0 + j));						//|
			}
			if ((i != row - 1) && (j != col - 1))
			{
				mIndices.push_back(static_cast<uint32_t>((i * col + j)));				//|
				mIndices.push_back(static_cast<uint32_t>(((i + 1) * col + j + 1)));		//|--- left triangle
				mIndices.push_back(static_cast<uint32_t>(((i + 1) * col + j)));			//|

				mIndices.push_back(static_cast<uint32_t>((i * col + j)));				//|
				mIndices.push_back(static_cast<uint32_t>((i * col + j + 1)));			//|--- right triangle
				mIndices.push_back(static_cast<uint32_t>(((i + 1) * col + j + 1)));		//|
			}
			if ((i == row - 1) && (j != 0) && (j != col - 1))
			{
				mIndices.push_back(static_cast<uint32_t>(i * col));						//|
				mIndices.push_back(static_cast<uint32_t>(i * col + j));					//|--- bottom circle
				mIndices.push_back(static_cast<uint32_t>(i * col + j + 1));				//|
			}
		}
	}

	Mesh mesh;
	mesh.mVertices.insert(mesh.mVertices.end(), std::begin(mVertices), std::end(mVertices));
	mesh.mIndices.insert(mesh.mIndices.end(), std::begin(mIndices), std::end(mIndices));
	return mesh;
}

Mesh NFGE::Graphics::MeshBuilder::CreateTorus(int row, int col, float innerRadius, float outerRadius)
{
	std::vector<Vertex> mVertices;
	std::vector<uint32_t> mIndices;

	float cylinderRadius = (outerRadius - innerRadius) * 0.5f;
	float realRadius = innerRadius + cylinderRadius;
	float unitAngleRad_Horiz = Math::Constants::TwoPi / (col - 1);
	float unitAngleRad_Verti = Math::Constants::TwoPi / (row - 1);
	float uUnit = 1.0f / (col - 1);
	float vUnit = 1.0f / (row - 1);

	for (int i = 0; i < row; i++)
	{
		for (int j = 0; j < col; j++)
		{
			NFGE::Math::Vector3 circleCenterInEachRow = { 0.0f,sin(unitAngleRad_Verti * i) * realRadius, cos(unitAngleRad_Verti * i) * realRadius };
			NFGE::Math::Vector3 pos = {
				//----------------| sin of current portion of Horiz angle |----------
				//                | give us [0] to [-1] to [0] to [1]     |
				//                v                                       v
				cylinderRadius * sin(j * unitAngleRad_Horiz),

				//--------------------------------|             Length of the Proj of Vertex on RealRadius direction                                    |
				//--------------------------------| RealRadius + (clinderRadius - proj of [Vertex position vector] on local circle on Radius direction) |
				//                                v                                                                                                     v
				sin(unitAngleRad_Verti * i) * (realRadius + cylinderRadius - (cylinderRadius * cos(j * unitAngleRad_Horiz))),
				cos(unitAngleRad_Verti * i) * (realRadius + cylinderRadius - (cylinderRadius * cos(j * unitAngleRad_Horiz)))
			};

			NFGE::Math::Vector3 normal = NFGE::Math::Normalize(pos - circleCenterInEachRow);

			mVertices.push_back({
				pos,
				normal,
				{ NFGE::Math::Cross(normal, NFGE::Math::Cross({1.0f,0.0f,0.0f},circleCenterInEachRow)) },
				{(float)j * uUnit , (float)i * vUnit},
				});

			if ((i == 0) && (j != 0) && (j != col - 1))
			{
				//mIndices.push_back(static_cast<uint32_t>(0));							//|
				//mIndices.push_back(static_cast<uint32_t>(0 + j + 1));					//|--- top circle
				//mIndices.push_back(static_cast<uint32_t>(0 + j));						//|
			}
			if ((i != row - 1) && (j != col - 1))
			{
				mIndices.push_back(static_cast<uint32_t>((i * col + j)));				//|
				mIndices.push_back(static_cast<uint32_t>(((i + 1) * col + j)));			//|
				mIndices.push_back(static_cast<uint32_t>(((i + 1) * col + j + 1)));		//|--- left triangle

				mIndices.push_back(static_cast<uint32_t>((i * col + j)));				//|
				mIndices.push_back(static_cast<uint32_t>(((i + 1) * col + j + 1)));		//|
				mIndices.push_back(static_cast<uint32_t>((i * col + j + 1)));			//|--- right triangle
			}
			if ((i != row - 1) && (j == col - 1))
			{
				//mIndices.push_back(static_cast<uint32_t>((i * col + j)));				//|
				//mIndices.push_back(static_cast<uint32_t>(((i + 1) * col + j)));			//|
				//mIndices.push_back(static_cast<uint32_t>(((i + 1) * col)));		//|--- left triangle
				//
				//mIndices.push_back(static_cast<uint32_t>((i * col + j)));				//|
				//mIndices.push_back(static_cast<uint32_t>(((i + 1) * col)));		//|
				//mIndices.push_back(static_cast<uint32_t>((i * col)));			//|--- right triangle

				mIndices.push_back(static_cast<uint32_t>((i * col)));				//|
				mIndices.push_back(static_cast<uint32_t>((i * col + j)));				//|
				mIndices.push_back(static_cast<uint32_t>(((i + 1) * col)));		//|--- left trian

				mIndices.push_back(static_cast<uint32_t>((i * col + j)));				//|
				mIndices.push_back(static_cast<uint32_t>(((i + 1) * col + j)));		//|
				mIndices.push_back(static_cast<uint32_t>(((i + 1) * col)));		//|
			}
			if ((i == row - 1) && (j != 0) && (j != col - 1))
			{
				//mIndices.push_back(static_cast<uint32_t>(i * col));						//|
				//mIndices.push_back(static_cast<uint32_t>(i * col + j));					//|--- bottom circle
				//mIndices.push_back(static_cast<uint32_t>(i * col + j + 1));				//|
			}
		}
	}

	Mesh mesh;
	mesh.mVertices.insert(mesh.mVertices.end(), std::begin(mVertices), std::end(mVertices));
	mesh.mIndices.insert(mesh.mIndices.end(), std::begin(mIndices), std::end(mIndices));
	return mesh;
}

BoneMesh NFGE::Graphics::MeshBuilder::CreateTentacle(int numOfSeg, int sliceFactor, int roundFactor, float radius, float height, float(*radiusShaper)(float))
{
	std::vector<BoneVertex> mVertices;
	std::vector<uint32_t> mIndices;
	float unitHeight = height / (sliceFactor - 1);
	float unitAngleRad = -Math::Constants::TwoPi / (roundFactor - 1);
	float uUnit = 1.0f / (roundFactor - 1);
	float vUnit = 1.0f / (sliceFactor - 1);
	float unitSegmentHeight = height / numOfSeg;

	for (int i = 0; i < sliceFactor; i++)
	{
		for (int j = 0; j < roundFactor; j++)
		{
			float currentHeight = static_cast<float>(i) * unitHeight;
			float currentRadius = (1.0f - radiusShaper(currentHeight / (height))) * radius;
			if (currentRadius == 0.0f)
				currentRadius = 0.001f;

			NFGE::Math::Vector3 position{
				currentRadius * 0.5f * sin(j * unitAngleRad),
				currentHeight,
				currentRadius * cos(j * unitAngleRad)
			};
			NFGE::Math::Vector3 normal = NFGE::Math::Normalize(position - NFGE::Math::Vector3{ 0.0f, position.y, 0.0f });
			NFGE::Math::Vector3 tangent{ -normal.z, normal.y, normal.x };
			NFGE::Math::Vector2 texcoord{ 1.0f - ((float)j * uUnit), 1.0f - ((float)i * vUnit) };
			int bonemIndices[4] = { 0 };
			float boneWeights[4] = { 0.0f };
			int currentAttachBone = static_cast<int>(position.y / unitSegmentHeight);

			/*if (currentAttachBone == numOfSeg)
			{
				currentAttachBone -= 1;
			}

				bonemIndices[0] = currentAttachBone;
				boneWeights[0] = (position.y - currentAttachBone * unitSegmentHeight) / unitSegmentHeight;
				bonemIndices[1] = currentAttachBone - 1;
				boneWeights[1] = 1.0f - boneWeights[0];

				if (currentAttachBone == 0)
				{
					bonemIndices[1] = 0;
					boneWeights[1] = 0.0f;
				}*/

			if (currentAttachBone != numOfSeg)
			{
				float differenceToBoneCenter = position.y - (currentAttachBone * unitSegmentHeight + unitSegmentHeight * 0.5f);
				bool isBottomPart = differenceToBoneCenter < 0.0f;
				bonemIndices[0] = currentAttachBone;

				if (isBottomPart)
				{
					bonemIndices[1] = currentAttachBone - 1 >= 0 ? currentAttachBone - 1 : 0;
					boneWeights[0] = currentAttachBone - 1 >= 0 ? NFGE::Math::Lerp(1.0f, 0.5f, (-differenceToBoneCenter) / (unitSegmentHeight * 0.5f)) : 1.0f;
					boneWeights[1] = currentAttachBone - 1 >= 0 ? NFGE::Math::Lerp(0.0f, 0.5f, (-differenceToBoneCenter) / (unitSegmentHeight * 0.5f)) : 0.0f;
				}
				else
				{
					bonemIndices[1] = currentAttachBone + 1 < numOfSeg ? currentAttachBone + 1 : currentAttachBone;
					boneWeights[0] = currentAttachBone + 1 < numOfSeg ? NFGE::Math::Lerp(1.0f, 0.5f, (differenceToBoneCenter) / (unitSegmentHeight * 0.5f)) : 1.0f;
					boneWeights[1] = currentAttachBone + 1 < numOfSeg ? NFGE::Math::Lerp(0.0f, 0.5f, (differenceToBoneCenter) / (unitSegmentHeight * 0.5f)) : 0.0f;
				}
			}
			else
			{
				bonemIndices[0] = currentAttachBone - 1;
				boneWeights[0] = 1.0f;
			}

			//position.y = 0.0f; // (currentHeight - currentAttachBone * unitSegmentHeight);// +unitSegmentHeight * currentAttachBone;

			mVertices.push_back({
				position,
				normal,
				tangent,
				texcoord,
				{bonemIndices[0],bonemIndices[1],bonemIndices[2],bonemIndices[3]},
				{boneWeights[0],boneWeights[1],boneWeights[2],boneWeights[3]}
				});

			if ((i == 0) && (j != 0) && (j != roundFactor - 1))
			{
				mIndices.push_back(static_cast<uint32_t>(0 + j));						//||
				mIndices.push_back(static_cast<uint32_t>(0 + j + 1));					//||--- bot circle
				mIndices.push_back(static_cast<uint32_t>(0));							//||
			}
			if ((i != sliceFactor - 1) && (j != roundFactor - 1))
			{
				mIndices.push_back(static_cast<uint32_t>((i * roundFactor + j)));				//|
				mIndices.push_back(static_cast<uint32_t>(((i + 1) * roundFactor + j)));			//|
				mIndices.push_back(static_cast<uint32_t>(((i + 1) * roundFactor + j + 1)));		//|--- left triangle

				mIndices.push_back(static_cast<uint32_t>((i * roundFactor + j)));				//|
				mIndices.push_back(static_cast<uint32_t>(((i + 1) * roundFactor + j + 1)));		//|
				mIndices.push_back(static_cast<uint32_t>((i * roundFactor + j + 1)));			//|--- right triangle
			}
			if ((i == sliceFactor - 1) && (j != 0) && (j != roundFactor - 1))
			{
				mIndices.push_back(static_cast<uint32_t>(i * roundFactor + j + 1));				//|
				mIndices.push_back(static_cast<uint32_t>(i * roundFactor + j));					//|--- top circle
				mIndices.push_back(static_cast<uint32_t>(i * roundFactor));						//|
			}
		}
	}

	BoneMesh mesh;
	mesh.mVertices.insert(mesh.mVertices.end(), std::begin(mVertices), std::end(mVertices));
	mesh.mIndices.insert(mesh.mIndices.end(), std::begin(mIndices), std::end(mIndices));
	//return mesh;

	return mesh;
}

NFGE::Math::AABB MeshBuilder::ComputeBound(const Mesh& mesh)
{
	if (mesh.mVertices.empty())
		return Math::AABB();

	Math::Vector3 min = mesh.mVertices[0].position;
	Math::Vector3 max = mesh.mVertices[0].position;

	for (auto& vertex : mesh.mVertices)
	{
		min.x = Math::Min(min.x, vertex.position.x);
		min.y = Math::Min(min.y, vertex.position.y);
		min.z = Math::Min(min.z, vertex.position.z);

		max.x = Math::Max(max.x, vertex.position.x);
		max.y = Math::Max(max.y, vertex.position.y);
		max.z = Math::Max(max.z, vertex.position.z);
	}
	return Math::AABB::FromMinMax(min, max);
}

void MeshBuilder::ComputeNormals(Mesh& mesh)
{
	std::vector<NFGE::Math::Vector3> normals;
	normals.resize(mesh.GetVertexCount());

	for (uint32_t i = 0; i < mesh.GetIndexCount(); i += 3)
	{
		uint32_t i0 = mesh.GetIndex(i + 0);
		uint32_t i1 = mesh.GetIndex(i + 1);
		uint32_t i2 = mesh.GetIndex(i + 2);
		const auto& a = mesh.GetVertex(i0);
		const auto& b = mesh.GetVertex(i1);
		const auto& c = mesh.GetVertex(i2);
		Math::Vector3 ab = b.position - a.position;
		Math::Vector3 ac = c.position - a.position;
		Math::Vector3 normal = Math::Normalize(Math::Cross(ab, ac));
		normals[i0] += normal;
		normals[i1] += normal;
		normals[i2] += normal;
	}

	for (uint32_t i = 0; i < mesh.GetVertexCount(); ++i)
	{
		mesh.GetVertex(i).normal = Math::Normalize(normals[i]);
	}
}