#include "Scene.h"

CScene::CScene()
{
  ZeroMemory(&m_d3dpp, sizeof(m_d3dpp));
  m_selectedModelId = GUID_NULL;
  m_d3d8 = 0;
  m_device = 0;
}

CScene::~CScene()
{
  ReleaseResources();
  if(m_device) m_device->Release();
  if(m_d3d8) m_d3d8->Release();
}

BOOL CScene::Create(HWND handle) 
{
  m_hWnd = handle;

  if((m_d3d8 = Direct3DCreate8(D3D_SDK_VERSION)) == NULL)
  {
    return FALSE;
  }
  
  D3DDISPLAYMODE d3ddm;
  if(FAILED(m_d3d8->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm)))
  {
    return FALSE;
  }
  
  m_d3dpp.Windowed = TRUE;
  m_d3dpp.SwapEffect = D3DSWAPEFFECT_COPY_VSYNC;
  m_d3dpp.BackBufferFormat = d3ddm.Format;
  m_d3dpp.EnableAutoDepthStencil = TRUE;
  m_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
  
  if(FAILED(m_d3d8->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, handle,
    D3DCREATE_SOFTWARE_VERTEXPROCESSING, &m_d3dpp, &m_device)))
  {
    return FALSE;
  }
  
  m_gizmo.SetCamera(&m_camera);
  Resize();
  
  return TRUE;
}

void CScene::Render() 
{
  // Calculate the frame rendering speed.
  static float lastTime = (float)timeGetTime();
  float currentTime = (float)timeGetTime();
  float deltaTime = (currentTime - lastTime) * 0.001f;
  
  CheckInput(deltaTime);
  
  if(m_device)
  {
    D3DMATERIAL8 mtrl2;
    ZeroMemory(&mtrl2, sizeof(D3DMATERIAL8));
    mtrl2.Diffuse.r = mtrl2.Ambient.r = 1.0f;
    mtrl2.Diffuse.g = mtrl2.Ambient.g = 1.0f;
    mtrl2.Diffuse.b = mtrl2.Ambient.b = 1.0f;
    mtrl2.Diffuse.a = mtrl2.Ambient.a = 1.0f;
    
    D3DMATERIAL8 mtrl3;
    ZeroMemory(&mtrl3, sizeof(D3DMATERIAL8));
    mtrl3.Diffuse.r = mtrl2.Ambient.r = 1.0f;
    mtrl3.Diffuse.g = mtrl2.Ambient.g = 0.0f;
    mtrl3.Diffuse.b = mtrl2.Ambient.b = 1.0f;
    mtrl3.Diffuse.a = mtrl2.Ambient.a = 1.0f;
    
    D3DXVECTOR3 vecDir;
    D3DLIGHT8 light;
    ZeroMemory(&light, sizeof(D3DLIGHT8));
    light.Type = D3DLIGHT_DIRECTIONAL;
    light.Diffuse.r = 1.0f;
    light.Diffuse.g = 1.0f;
    light.Diffuse.b = 1.0f;
    light.Range = 1000.0f;
    vecDir = D3DXVECTOR3(90, 0.0f, 90);
    D3DXVec3Normalize((D3DXVECTOR3*)&light.Direction, &vecDir);
    
    ID3DXMatrixStack *matrixStack;
    if(!SUCCEEDED(D3DXCreateMatrixStack(0, &matrixStack))) return;
    matrixStack->LoadMatrix(&m_camera.GetViewMatrix());
    
    m_device->SetTransform(D3DTS_WORLD, matrixStack->GetTop());
    m_device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(90, 90, 90), 1.0f, 0);
    
    if(!SUCCEEDED(m_device->BeginScene())) return;
    
    m_device->SetLight(0, &light);
    m_device->LightEnable(0, TRUE);
    m_device->SetRenderState(D3DRS_ZENABLE, TRUE);
    
    // Draw the grid.
    m_grid.Render(m_device);
    
    std::map<GUID, CModel>::iterator it;
    for(it = m_models.begin(); it != m_models.end(); it++)
    {
      // Color the selected model.
      if(it->first == m_selectedModelId)
      {
        m_device->SetMaterial(&mtrl3);
      } else {
        m_device->SetMaterial(&mtrl2);
      }
      
      it->second.Render(m_device, matrixStack);
    }
    
    // Draw the gizmo on "top" of all objects in scene.
    m_device->SetRenderState(D3DRS_ZENABLE, FALSE);
    
    // Draw the gizmo.
    if(m_selectedModelId != GUID_NULL)
    {
      m_gizmo.Render(m_device, matrixStack);
    }
    
    m_device->EndScene();
    m_device->Present(NULL, NULL, NULL, NULL);
  }
  
  lastTime = currentTime;
}

void CScene::Resize() 
{
  float aspect = (float)800 / (float)600;
  float fov = 3.14f / 2.0f;
  
  D3DXMATRIX m;
  D3DXMatrixPerspectiveFovLH(&m, fov, aspect, 0.1f, 1000.0f);
  
  if(m_device)
  {
    ReleaseResources();
    m_device->Reset(&m_d3dpp);
    m_device->SetTransform(D3DTS_PROJECTION, &m);
  }
}

void CScene::CheckInput(float deltaTime)
{
  POINT mousePoint;
  GetCursorPos(&mousePoint);
  ScreenToClient(m_hWnd, &mousePoint);
  
  static POINT prevMousePoint = mousePoint;
  static int showCount = 0;
  
  if(GetAsyncKeyState('1')) m_gizmo.SetModifier(Translate);
  if(GetAsyncKeyState('2')) m_gizmo.SetModifier(Scale);
  if(GetAsyncKeyState('3')) m_gizmo.SetModifier(Rotate);
  
  if(GetAsyncKeyState(VK_LBUTTON) && m_selectedModelId != GUID_NULL)
  {
    D3DXVECTOR3 rayOrigin, rayDir;
    ScreenRaycast(mousePoint, &rayOrigin, &rayDir);
    m_gizmo.Update(rayOrigin, rayDir, &m_models[m_selectedModelId]);
  }
  else if(GetAsyncKeyState(VK_RBUTTON))
  {
    if(showCount == 0) showCount = ShowCursor(FALSE);
    
    Move(mousePoint, prevMousePoint, deltaTime);
  }
  else if(GetAsyncKeyState(VK_MBUTTON))
  {
    if(showCount == 0) showCount = ShowCursor(FALSE);
    
    m_camera.Strafe((prevMousePoint.x - mousePoint.x) * deltaTime);
    m_camera.Fly((mousePoint.y - prevMousePoint.y) * deltaTime);
  }
  else
  {
    if(showCount == -1) showCount = ShowCursor(TRUE);
    
    m_gizmo.Reset();
  }
  
  // Remeber the last position so we know how much to move the camera.
  prevMousePoint = mousePoint;
}

void CScene::Move(POINT mousePoint, POINT prevMousePoint, float deltaTime)
{
  if(GetAsyncKeyState('W')) m_camera.Walk(4.0f * deltaTime);
  if(GetAsyncKeyState('S')) m_camera.Walk(-4.0f * deltaTime);
  
  if(GetAsyncKeyState('A')) m_camera.Strafe(-4.0f * deltaTime);
  if(GetAsyncKeyState('D')) m_camera.Strafe(4.0f * deltaTime);
  
  if(GetAsyncKeyState('Q')) m_camera.Roll(4.0f * deltaTime);
  if(GetAsyncKeyState('E')) m_camera.Roll(-4.0f * deltaTime);
  
  m_camera.Yaw((mousePoint.x - prevMousePoint.x) * deltaTime);
  m_camera.Pitch((mousePoint.y - prevMousePoint.y) * deltaTime);
}

void CScene::OnMouseWheel(short zDelta) 
{
  m_camera.Walk(zDelta * 0.005f);
}

void CScene::ScreenRaycast(POINT screenPoint, D3DXVECTOR3 *origin, D3DXVECTOR3 *dir)
{
  D3DXMATRIX matProj;
  m_device->GetTransform(D3DTS_PROJECTION, &matProj);
  
  RECT clientRect;
  GetClientRect(m_hWnd, &clientRect);
  int width = clientRect.right - clientRect.left;
  int height = clientRect.bottom - clientRect.top;
  
  // Compute the vector of the pick ray in screen space.
  D3DXVECTOR3 v;
  v.x = (((2.0f * screenPoint.x) / width) - 1) / matProj._11;
  v.y = -(((2.0f * screenPoint.y) / height) - 1) / matProj._22;
  v.z = 1.0f;
  
  // Get the camera inverse view matrix.
  D3DXMATRIX matView, m;
  D3DXMatrixInverse(&m, NULL, &m_camera.GetViewMatrix());
  
  // Transform the screen space pick ray into 3D space.
  (*dir).x = v.x * m._11 + v.y * m._21 + v.z * m._31;
  (*dir).y = v.x * m._12 + v.y * m._22 + v.z * m._32;
  (*dir).z = v.x * m._13 + v.y * m._23 + v.z * m._33;
  
  // Center of screen.
  (*origin).x = m._41;
  (*origin).y = m._42;
  (*origin).z = m._43;
}

void CScene::ReleaseResources()
{
  m_grid.Release();
  m_gizmo.Release();
  
  std::map<GUID, CModel>::iterator it;
  for(it = m_models.begin(); it != m_models.end(); it++)
  {
    it->second.Release();
  }
}