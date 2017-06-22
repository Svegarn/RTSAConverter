#include <fbxsdk.h>
#include <filesystem>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>

namespace fs = std::experimental::filesystem;

using namespace fbxsdk;

struct Vertex
{
	float position[3];
	float normal[3];
	float uv[2];
};

// Initialize FBX
FbxManager* manager;
FbxIOSettings* ios;
FbxImporter* importer;

void LoadStaticMesh( std::string path_ )
{
	if (!importer->Initialize(path_.c_str(), -1, manager->GetIOSettings()))
	{	
		std::cout << "Error loading file: " << path_ << "\n";
		getchar();
		exit(-1);
	}
	
	FbxScene* scene = FbxScene::Create( manager, "scene" );
	importer->Import(scene);

	FbxNode* root = scene->GetRootNode();

	FbxGeometryConverter converter(manager);
	converter.Triangulate(scene, true);

	for( int i = 0; i < root->GetChildCount(); i++ )
		if ( strcmp( root->GetChild( i )->GetTypeName(), "Mesh" ) == 0)
		{
			// Get mesh data
			FbxMesh* mesh = root->GetChild(i)->GetMesh();
			
			FbxVector4* controlPoints = mesh->GetControlPoints();
			FbxVector4 normal;
			FbxVector2 uv;

			int numVertices = mesh->GetControlPointsCount();
			int numPolygons = mesh->GetPolygonCount();
			int vertexCount = mesh->GetPolygonVertexCount();

			int polySize = 0;
			int vertexIndex = 0;

			std::vector<Vertex> vertices;

			FbxLayerElementArrayTemplate<FbxVector2>* uvArray;
			mesh->GetTextureUV(&uvArray);
			
			for (int i = 0; i < numPolygons; i++) {
				polySize = mesh->GetPolygonSize(i);

				for (int j = 0; j < polySize; j++) {
					vertexIndex = mesh->GetPolygonVertex(i, j);
					mesh->GetPolygonVertexNormal(i, j, normal);

					vertices.push_back(Vertex{
						(float)controlPoints[vertexIndex][0],
						(float)controlPoints[vertexIndex][1],
						-(float)controlPoints[vertexIndex][2],
						(float)normal[0],
						(float)normal[1],
						-(float)normal[2],
						(float)uvArray->GetAt(mesh->GetTextureUVIndex(i, j))[0],
						1.0f - (float)uvArray->GetAt(mesh->GetTextureUVIndex(i, j))[1]
						});
				}
				std::swap(vertices[vertices.size()-2], vertices[vertices.size()-1]);
			}

			// Write mesh data
			int last = path_.find_last_of("/\\");
			std::string fileName = std::string("Output/") + path_.substr(last + 1, path_.size() - last - 5) + std::string(".rtsa");
			std::ofstream file(fileName, std::ios::binary);
			
			int finalCount = (int)vertices.size();
			file.write((char*)&finalCount, sizeof(int));
			file.write((char*)vertices.data(), sizeof(Vertex) * finalCount);

			file.close();
			break;
		}
	
	scene->Destroy();
}

void main()
{
	// Initialize FBX
	manager = FbxManager::Create();

	ios = FbxIOSettings::Create( manager, IOSROOT );
	manager->SetIOSettings( ios );

	importer = FbxImporter::Create( manager, "" );

	// Convert
	std::string path = "Assets";
    for ( auto & p : fs::directory_iterator( path ) )
		LoadStaticMesh( fs::absolute( p ).string() );

	// Cleanup
	importer->Destroy();
	ios->Destroy();
	manager->Destroy();
}