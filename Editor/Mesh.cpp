#include "Mesh.h"

CMesh::CMesh(aiMatrix4x4 transform, aiMesh* mesh)
{
  for(int i = 0; i < mesh->mNumFaces; i++)
  {
    aiFace face = mesh->mFaces[i];
    for(int j = 0; j < face.mNumIndices; j++)
    {
      MeshVertex vertex;
      
      // Apply the current transform to the mesh
      // so it renders in the correct local location.
      aiVector3D transformedVertex = mesh->mVertices[face.mIndices[j]];
      aiTransformVecByMatrix4(&transformedVertex, &transform);
      
      vertex.position.x = transformedVertex.x;
      vertex.position.y = transformedVertex.y;
      vertex.position.z = transformedVertex.z;
      
      aiVector3D normal = mesh->mNormals[face.mIndices[j]];
      vertex.normal.x = normal.x;
      vertex.normal.y = normal.y;
      vertex.normal.z = normal.z;
      
      if(mesh->HasTextureCoords(0))
      {
        vertex.tu = mesh->mTextureCoords[0][face.mIndices[j]].x;
        vertex.tv = mesh->mTextureCoords[0][face.mIndices[j]].y;
      }
      
      m_vertices.push_back(vertex);
    }
  }
}

CMesh::~CMesh()
{
}

std::vector<MeshVertex> CMesh::Vertices()
{
  return m_vertices;
}