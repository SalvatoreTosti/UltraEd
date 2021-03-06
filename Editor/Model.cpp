#include "Model.h"

CModel::CModel()
{
}

CModel::CModel(const char* filePath)
{
  // Assign the model a unique id.
  CoCreateGuid(&m_id);
  
  m_vertexBuffer = 0;

  m_texture = 0;
  
  m_position = D3DXVECTOR3(0, 0, 0);
  
  m_scale = D3DXVECTOR3(1, 1, 1);
  
  m_rotation = D3DXVECTOR3(0, 0, 0);
  
  Assimp::Importer importer;
  
  const aiScene* scene = importer.ReadFile(filePath,
    aiProcess_Triangulate | aiProcess_FlipUVs |
    aiProcess_OptimizeMeshes);
  
  if(scene)
  {
    Process(scene->mRootNode, scene);
  }
}

CModel::~CModel()
{
}

void CModel::Process(aiNode* node, const aiScene* scene)
{
  int i;
  
  for(i = 0; i < node->mNumMeshes; i++)
  {
    aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
    std::vector<MeshVertex> verts = CMesh(node->mTransformation, mesh).Vertices();
    m_vertices.insert(m_vertices.end(), verts.begin(), verts.end()); 
  }
  
  for(i = 0; i < node->mNumChildren; i++)
  {
    Process(node->mChildren[i], scene);
  }
}

IDirect3DVertexBuffer8* CModel::GetBuffer(IDirect3DDevice8* device)
{
  if(m_vertexBuffer == NULL)
  {
    if(FAILED(device->CreateVertexBuffer(
      m_vertices.size() * sizeof(MeshVertex),
      0, 
      D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1,
      D3DPOOL_DEFAULT,
      &m_vertexBuffer)))
    {
      return NULL;
    }
    
    VOID* pVertices;
    if(FAILED(m_vertexBuffer->Lock(0, m_vertices.size() * sizeof(MeshVertex),
      (BYTE**)&pVertices, 0)))
    {
      return NULL;
    }
    
    memcpy(pVertices, &m_vertices[0], m_vertices.size() * sizeof(MeshVertex));
    m_vertexBuffer->Unlock();
  }
  
  return m_vertexBuffer;
}

std::vector<MeshVertex> CModel::GetVertices()
{
  return m_vertices;
}

void CModel::Render(IDirect3DDevice8 *device, ID3DXMatrixStack *matrixStack)
{
  IDirect3DVertexBuffer8* buffer = GetBuffer(device);
  
  if(buffer != NULL)
  {
    matrixStack->Push();
    matrixStack->MultMatrixLocal(&GetMatrix());
    
    if(m_texture != NULL) device->SetTexture(0, m_texture);

    device->SetTransform(D3DTS_WORLD, matrixStack->GetTop());
    device->SetStreamSource(0, buffer, sizeof(MeshVertex));
    device->SetVertexShader(D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1);
    device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, m_vertices.size() / 3);

    device->SetTexture(0, NULL);
    
    matrixStack->Pop();
  }
}

void CModel::Release()
{
  if(m_vertexBuffer != NULL)
  {
    m_vertexBuffer->Release();
    m_vertexBuffer = 0;
  }
}

GUID CModel::GetId()
{
  return m_id;
}

D3DXVECTOR3 CModel::GetPosition()
{
  return m_position;
}

void CModel::SetPosition(D3DXVECTOR3 position)
{
  m_position = position;
}

void CModel::SetScale(D3DXVECTOR3 scale)
{
  m_scale = scale;
}

void CModel::Move(D3DXVECTOR3 position, D3DXVECTOR3 along)
{
  D3DXVECTOR3 xVector(position.x, 0, 0);
  D3DXVec3Scale(&xVector, &xVector, along.x);
  
  D3DXVECTOR3 yVector(0, position.y, 0);
  D3DXVec3Scale(&yVector, &yVector, along.y);
  
  D3DXVECTOR3 zVector(0, 0, position.z);
  D3DXVec3Scale(&zVector, &zVector, along.z);
  
  m_position += (xVector + yVector + zVector);
}

void CModel::Scale(D3DXVECTOR3 position, D3DXVECTOR3 along)
{
  D3DXVECTOR3 xVector(position.x, 0, 0);
  D3DXVec3Scale(&xVector, &xVector, along.x);
  
  D3DXVECTOR3 yVector(0, position.y, 0);
  D3DXVec3Scale(&yVector, &yVector, along.y);
  
  D3DXVECTOR3 zVector(0, 0, position.z);
  D3DXVec3Scale(&zVector, &zVector, along.z);
  
  m_scale += (xVector + yVector + zVector);
}

void CModel::Rotate(D3DXVECTOR3 position, D3DXVECTOR3 along)
{
  D3DXVECTOR3 xVector(position.x, 0, 0);
  D3DXVec3Scale(&xVector, &xVector, along.x);
  
  D3DXVECTOR3 yVector(0, position.y, 0);
  D3DXVec3Scale(&yVector, &yVector, along.y);
  
  D3DXVECTOR3 zVector(0, 0, position.z);
  D3DXVec3Scale(&zVector, &zVector, along.z);
  
  m_rotation += (xVector + yVector + zVector);
}

D3DXMATRIX CModel::GetMatrix()
{
  D3DXMATRIX translation;
  D3DXMatrixTranslation(&translation, m_position.x, m_position.y, m_position.z);
  
  D3DXMATRIX scale;
  D3DXMatrixScaling(&scale, m_scale.x, m_scale.y, m_scale.z);
  
  D3DXMATRIX rotationX;
  D3DXMatrixRotationX(&rotationX, m_rotation.x);
  
  D3DXMATRIX rotationY;
  D3DXMatrixRotationY(&rotationY, m_rotation.y);
  
  D3DXMATRIX rotationZ;
  D3DXMatrixRotationZ(&rotationZ, m_rotation.z);
  
  return scale * rotationX * rotationY * rotationZ * translation;
}

BOOL CModel::Pick(D3DXVECTOR3 orig, D3DXVECTOR3 dir)
{
  std::vector<MeshVertex> vertices = GetVertices();
  
  // Test all faces in this model.
  for(int j = 0; j < vertices.size() / 3; j++)
  {
    D3DXVECTOR3 v0 = vertices[3 * j + 0].position;
    D3DXVECTOR3 v1 = vertices[3 * j + 1].position;
    D3DXVECTOR3 v2 = vertices[3 * j + 2].position;
    
    // Transform the local vert positions based of the models
    // local matrix so when the model is moved around we can still click it.
    D3DXVec3TransformCoord(&v0, &v0, &GetMatrix());
    D3DXVec3TransformCoord(&v1, &v1, &GetMatrix());
    D3DXVec3TransformCoord(&v2, &v2, &GetMatrix());
    
    // Check if the pick ray passes through this point.
    if(IntersectTriangle(orig, dir, v0, v1, v2))
    {
      return TRUE;
    }
  }

  return FALSE;
}

BOOL CModel::IntersectTriangle(const D3DXVECTOR3& orig,
                               const D3DXVECTOR3& dir, D3DXVECTOR3& v0,
                               D3DXVECTOR3& v1, D3DXVECTOR3& v2)
{
  // Find vectors for two edges sharing vert0
  D3DXVECTOR3 edge1 = v1 - v0;
  D3DXVECTOR3 edge2 = v2 - v0;
  
  // Begin calculating determinant - also used to calculate U parameter.
  D3DXVECTOR3 pvec;
  D3DXVec3Cross(&pvec, &dir, &edge2);
  
  // If determinant is near zero, ray lies in plane of triangle.
  FLOAT det = D3DXVec3Dot(&edge1, &pvec);
  
  if(det < 0.0001f) return FALSE;
  
  // Calculate U parameter and test bounds.
  D3DXVECTOR3 tvec = orig - v0;
  FLOAT u = D3DXVec3Dot(&tvec, &pvec);
  if(u < 0.0f || u > det) return FALSE;
  
  // Prepare to test V parameter.
  D3DXVECTOR3 qvec;
  D3DXVec3Cross(&qvec, &tvec, &edge1);
  
  // Calculate V parameter and test bounds.
  FLOAT v = D3DXVec3Dot(&dir, &qvec);
  if(v < 0.0f || u + v > det) return FALSE;
  
  return TRUE;
}

BOOL CModel::LoadTexture(IDirect3DDevice8 *device, const char *filePath)
{
  if(FAILED(D3DXCreateTextureFromFile(device, filePath, &m_texture)))
  {
    return FALSE;
  }

  return TRUE;
}