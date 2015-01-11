/*
//-----------------------------------------------------------------------------
// Basic Camera/Viewpoint Movement Using Direct 3D

//-----------------------------------------------------------------------------
// Include these files
#include <Windows.h>	// Windows library (for window functions, menus, dialog boxes, etc)
#include <d3dx9.h>		// Direct 3D library (for all Direct 3D funtions).

#include <iostream>
#include <string>
#include <fstream>
// for using PI
#define _USE_MATH_DEFINES
#include <math.h>


#define D3D_DEBUG_INFO

//-----------------------------------------------------------------------------
// Global variables

LPDIRECT3D9             g_pD3D           = NULL; // Used to create the D3DDevice
LPDIRECT3DDEVICE9       g_pd3dDevice     = NULL; // The rendering device
LPDIRECT3DVERTEXBUFFER9 g_pVertexBuffer  = NULL; // Buffer to hold vertices for the rectangle

// A structure for our custom vertex type
struct CUSTOMVERTEX
{
    float x, y, z;      // X, Y, Z position of the vertex.
    DWORD colour;       // The vertex color
};

// The structure of a vertex in our vertex buffer...
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ | D3DFVF_DIFFUSE)

float g_CameraX = 0.0f;
float g_CameraY = 0.0f;
float g_CameraZ = -35.0f;

//-----------------------------------------------------------------------------
// Initialise Direct 3D.
// Requires a handle to the window in which the graphics will be drawn.

HRESULT SetupD3D(HWND hWnd)
{
    // Create the D3D object, return failure if this can't be done.
    if (NULL == (g_pD3D = Direct3DCreate9(D3D_SDK_VERSION))) return E_FAIL;

    // Set up the structure used to create the D3DDevice
    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D16;

    // Create the D3DDevice
    if (FAILED(g_pD3D -> CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
                                      D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                      &d3dpp, &g_pd3dDevice)))
    {
        return E_FAIL;
    }

	// Turn on the Z buffer
	g_pd3dDevice -> SetRenderState(D3DRS_ZENABLE, TRUE);

	// Turn off the lighting, as we're using our own vertex colours.
	g_pd3dDevice -> SetRenderState(D3DRS_LIGHTING, FALSE);

	// Display objects in wireframe
	g_pd3dDevice -> SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);

    return S_OK;
}

//-----------------------------------------------------------------------------
// Release (delete) all the resources used by this program.
// Only release things if they are valid (i.e. have a valid pointer).
// If not, the program will crash at this point.

VOID CleanUp()
{
    if (g_pVertexBuffer != NULL) g_pVertexBuffer -> Release();
    if (g_pd3dDevice != NULL)	 g_pd3dDevice -> Release();
    if (g_pD3D != NULL)			 g_pD3D -> Release();
}

int GetID()
{
	static int ID = 0;
	std::ofstream to("Output.txt");
	to << "\n" << ID;
	to.close();
	return ID++;
}


//-----------------------------------------------------------------------------
// Set up the scene geometry.
// For this program, this is a rectangle, consisting of 2 triangles (6 vertices)
HRESULT SetupGeometry()
{
	HRESULT SetupCone();
	HRESULT SetupSpherePoints();
	HRESULT SetupSphereTriangles();

	return SetupSphereTriangles();
	//return SetupSpherePoints();
	//return SetupCone();
}

HRESULT SetupSphereTriangles()
{
	void SetupVertexGeometry( CUSTOMVERTEX* pV, int index, float x, float y, float z, DWORD c);
	
	// should be divisors of 360 respectively 180
	int verticalSlices = 36;
	int horizontalSlices = 18;

	// Calculate the number of vertices required, and the size of the buffer to hold them.
	int Vertices = 100000;//2 + (horzontalSlices - 1) * verticalSlices;	// Six vertices for the rectangle.
	int BufferSize = Vertices * sizeof(CUSTOMVERTEX);

	// Now get Direct3D to create the vertex buffer.
	// The vertex buffer for the rectangle doesn't exist at this point, but the pointer to
	// it does (g_pVertexBuffer)
	if (FAILED(g_pd3dDevice -> CreateVertexBuffer(BufferSize, 0, D3DFVF_CUSTOMVERTEX, D3DPOOL_DEFAULT, &g_pVertexBuffer, NULL)))
	{
		return E_FAIL; // if the vertex buffer culd not be created.
	}

	// Fill the buffer with appropriate vertices to describe the rectangle.
	// The rectangle will be made from two triangles...

	// Create a pointer to the first vertex in the buffer
	// Also lock it, so nothing else can touch it while the values are being inserted.
	CUSTOMVERTEX* pVertices;
    if (FAILED(g_pVertexBuffer -> Lock(0, 0, (void**)&pVertices, 0)))
    {
		return E_FAIL;  // if the pointer to the vertex buffer could not be established.
	}

	// Fill the vertex buffers with data...  place the rectangle at the centre of the display
	
	// original radius of the sphere
	float r = 5.0f;
	// radius of the current ring
	float rn = 0.0f;
	// the y-coordinate for all points on a ring
	float yCoord = 0.0f;

	float rnOld = 0.0f;
	// the y-coordinate for all points on a ring
	float yCoordOld = 0.0f;

	// transforms degree to radian
	float degrees_to_radians = D3DX_PI / 180.0f;

	using namespace std;
	ofstream to("SphereOutput.txt");
	to << "\nStart...";

	int vertexCount = 0;

	int angleMod = 360/verticalSlices;

	// outer loop to determine y-coordinate and and new radius for rings around the sphere
	for(int alpha = 180; alpha >= 0; alpha -= 180/horizontalSlices)
	{
		rn = r * sin(alpha * degrees_to_radians);
		yCoord = r * cos(alpha * degrees_to_radians);

		// inner loop to compute x and z coordinates for points lying on a ring around the sphere at the current y-coordinate
		for(int beta = 0; beta > -360; beta -= angleMod)
		{
			// create 2 triangles that form a rectangular plate

			SetupVertexGeometry( pVertices, GetID(), rnOld * sin(beta * degrees_to_radians), yCoordOld, rnOld * cos(beta * degrees_to_radians), 0x0000ff00); // (green)
			SetupVertexGeometry( pVertices, GetID(), rn * sin(beta * degrees_to_radians), yCoord, rn * cos(beta * degrees_to_radians), 0x0000ff00); // (green)
			SetupVertexGeometry( pVertices, GetID(), rnOld * sin((beta-angleMod) * degrees_to_radians), yCoordOld, rnOld * cos((beta-angleMod) * degrees_to_radians), 0x0000ff00); // (green)
				
			SetupVertexGeometry( pVertices, GetID(), rn * sin(beta * degrees_to_radians), yCoord, rn * cos(beta * degrees_to_radians), 0x0000ff00); // (green)
			SetupVertexGeometry( pVertices, GetID(), rn * sin((beta-angleMod) * degrees_to_radians), yCoord, rn * cos((beta-angleMod) * degrees_to_radians), 0x0000ff00); // (green)
			SetupVertexGeometry( pVertices, GetID(), rnOld * sin((beta-angleMod) * degrees_to_radians), yCoordOld, rnOld * cos((beta-angleMod) * degrees_to_radians), 0x0000ff00); // (green)
				
			vertexCount+= 6;
		}
		rnOld = rn;
		yCoordOld = yCoord;
	}
	to << "\n\nVertexCount: " << vertexCount;
	to.close();

	// Unlock the vertex buffer...
	g_pVertexBuffer -> Unlock();

	return S_OK;
}

HRESULT SetupSpherePoints()
{
	void SetupVertexGeometry( CUSTOMVERTEX* pV, int index, float x, float y, float z, DWORD c);
	
	// should be divisors of 360 respectively 180
	int verticalSlices = 36;
	int horzontalSlices = 30;

	// Calculate the number of vertices required, and the size of the buffer to hold them.
	int Vertices = 10000;//2 + (horzontalSlices - 1) * verticalSlices;	// Six vertices for the rectangle.
	int BufferSize = Vertices * sizeof(CUSTOMVERTEX);

	// Now get Direct3D to create the vertex buffer.
	// The vertex buffer for the rectangle doesn't exist at this point, but the pointer to
	// it does (g_pVertexBuffer)
	if (FAILED(g_pd3dDevice -> CreateVertexBuffer(BufferSize, 0, D3DFVF_CUSTOMVERTEX, D3DPOOL_DEFAULT, &g_pVertexBuffer, NULL)))
	{
		return E_FAIL; // if the vertex buffer culd not be created.
	}

	// Fill the buffer with appropriate vertices to describe the rectangle.
	// The rectangle will be made from two triangles...

	// Create a pointer to the first vertex in the buffer
	// Also lock it, so nothing else can touch it while the values are being inserted.
	CUSTOMVERTEX* pVertices;
    if (FAILED(g_pVertexBuffer -> Lock(0, 0, (void**)&pVertices, 0)))
    {
		return E_FAIL;  // if the pointer to the vertex buffer could not be established.
	}

	// Fill the vertex buffers with data...  place the rectangle at the centre of the display
	
	// original radius of the sphere
	float r = 5.0f;
	// radius of the current ring
	float rn = 0.0f;
	// the y-coordinate for all points on a ring
	float yCoord = 0.0f;

	// transforms degree to radian
	float degrees_to_radians = M_PI / 180.0f;

	using namespace std;
	ofstream to("SphereOutput.txt");
	to << "\nStart...";

	int vertexCount = 0;

	// outer loop to determine y-coordinate and and new radius for rings around the sphere
	for(int alpha = 0; alpha <= 180; alpha += 180/horzontalSlices)
	{
		rn = r * sin(alpha * degrees_to_radians);
		yCoord = r * cos(alpha * degrees_to_radians);
		to << "\n\nAlpha: " << alpha << "\nNew Radius: " << rn << "\nYCoord: " << yCoord;

		// make sure that the loop is not used for the two endpoints of the sphere, otherwise 36 vertices would be added at these
		// very positions
		if(alpha != 0 && alpha != 180)
		{
			// inner loop to compute x and z coordinates for points lying on a ring around the sphere at the current y-coordinate
			for(int beta = 0; beta < 360; beta += 360/verticalSlices)
			{
				to << "\nDrawVertex: " << rn * cos(beta * degrees_to_radians) << ", " << yCoord << ", " << rn * sin(beta * degrees_to_radians) ;
				SetupVertexGeometry( pVertices, GetID(), rn * cos(beta * degrees_to_radians), yCoord, rn * sin(beta * degrees_to_radians), 0x00ff0000); // (red)
				++vertexCount;
			}
		}else
		{
			to << "\nDrawVertex: Out Of LOOP Vertex";
			SetupVertexGeometry( pVertices, GetID(), 0, yCoord, 0, 0x00ff0000); // (red)
			++vertexCount;
		}
	}
	to << "\n\nVertexCount: " << vertexCount;
	to.close();

	// Unlock the vertex buffer...
	g_pVertexBuffer -> Unlock();

	return S_OK;
}


HRESULT SetupCone()
{
	void SetupVertexGeometry( CUSTOMVERTEX* pV, int index, float x, float y, float z, DWORD c);
	
	// Calculate the number of vertices required, and the size of the buffer to hold them.
	int Vertices = 100;	// Six vertices for the rectangle.
	int BufferSize = Vertices * sizeof(CUSTOMVERTEX);

	// Now get Direct3D to create the vertex buffer.
	// The vertex buffer for the rectangle doesn't exist at this point, but the pointer to
	// it does (g_pVertexBuffer)
	if (FAILED(g_pd3dDevice -> CreateVertexBuffer(BufferSize, 0, D3DFVF_CUSTOMVERTEX, D3DPOOL_DEFAULT, &g_pVertexBuffer, NULL)))
	{
		return E_FAIL; // if the vertex buffer culd not be created.
	}

	// Fill the buffer with appropriate vertices to describe the rectangle.
	// The rectangle will be made from two triangles...

	// Create a pointer to the first vertex in the buffer
	// Also lock it, so nothing else can touch it while the values are being inserted.
	CUSTOMVERTEX* pVertices;
    if (FAILED(g_pVertexBuffer -> Lock(0, 0, (void**)&pVertices, 0)))
    {
		return E_FAIL;  // if the pointer to the vertex buffer could not be established.
	}

	// Fill the vertex buffers with data...  place the rectangle at the centre of the display
	
	/*
	SetupVertexGeometry( pVertices, 0, -5, 5,  0, 0x00ff0000); // (red)
	SetupVertexGeometry( pVertices, 1, 2,  5,  0, 0x00ffff00); // (yellow)
	SetupVertexGeometry( pVertices, 2,  -5, 2,  0, 0x0000ff00); // (green)
	*/
	/*
	// radius
	float r = 5.0f;
	// transforms degree to radian
	float degrees_to_radians = M_PI / 180.0f;

	
	// draw the base plate, indices 0 to 37 
	SetupVertexGeometry( pVertices, GetID(),  0, 0,  0, 0x00ff0000); // (red)
	
	for(int i = 0; i <= 360; i += 10)
	{
		SetupVertexGeometry( pVertices, GetID(), r * cos(static_cast<float>(i * degrees_to_radians)), 0, r * sin(static_cast<float>(i * degrees_to_radians)), 0x00ff0000); // (red)
	}
	
	// draw the cone indices 38 to 75
	SetupVertexGeometry( pVertices, GetID(),  0, 20,  0, 0x0000ff00); // (green)
	
	for(int i = 0; i >= -360; i -= 10)
	{
		SetupVertexGeometry( pVertices, GetID(), r * cos(static_cast<float>(i * degrees_to_radians)), 0, r * sin(static_cast<float>(i * degrees_to_radians)), 0x0000ff00); // (green)
	}


	// Unlock the vertex buffer...
	g_pVertexBuffer -> Unlock();

	return S_OK;
}

// Set up a vertex data.
void SetupVertexGeometry( CUSTOMVERTEX* pV, int index, float x, float y, float z, DWORD c)
{
	pV[index].x = x;
	pV[index].y = y;
	pV[index].z = z;
	pV[index].colour = c;
}

//-----------------------------------------------------------------------------
// Set up the view - the view and projection matrices.
void SetupViewMatrices()
{
	// Set up the view matrix.
	// This defines which way the 'camera' will look at, and which way is up.
    D3DXVECTOR3 vCamera(g_CameraX, g_CameraY, g_CameraZ);
    //D3DXVECTOR3 vLookat(0.0f, 0.0f, g_CameraZ + 10.0f);
	D3DXVECTOR3 vLookat(0.0f, 0.0f, 0.0f);

    D3DXVECTOR3 vUpVector(0.0f, 1.0f, 0.0f);
    D3DXMATRIX  matView;
    D3DXMatrixLookAtLH( &matView, &vCamera, &vLookat, &vUpVector);
    g_pd3dDevice -> SetTransform(D3DTS_VIEW, &matView);

	// Set up the projection matrix.
	// This transforms 2D geometry into a 3D space.
    D3DXMATRIX matProj;
    D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI/4, 1.0f, 1.0f, 100.0f);
    g_pd3dDevice -> SetTransform(D3DTS_PROJECTION, &matProj);
}

//-----------------------------------------------------------------------------
// Render the scene.

void Render()
{
    // Clear the backbuffer to a black color
    g_pd3dDevice -> Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 250), 1.0f, 0);

    // Begin the scene
    if (SUCCEEDED(g_pd3dDevice -> BeginScene()))
    {
        void RenderCone();
		void RenderSpherePoints();
		void RenderSphereTriangles();

		// Define the viewpoint.
        SetupViewMatrices();

        // Render the contents of the vertex buffer.
        g_pd3dDevice -> SetStreamSource(0, g_pVertexBuffer, 0, sizeof(CUSTOMVERTEX));
        g_pd3dDevice -> SetFVF(D3DFVF_CUSTOMVERTEX);
        
		//RenderCone();
		//RenderSpherePoints();
		RenderSphereTriangles();

        // End the scene.
        g_pd3dDevice -> EndScene();
    }

    // Present the backbuffer to the display.
    g_pd3dDevice -> Present(NULL, NULL, NULL, NULL);
}

void RenderSphereTriangles()
{
	g_pd3dDevice -> DrawPrimitive(D3DPT_TRIANGLELIST, 0, 8280);
}

void RenderSpherePoints()
{
	g_pd3dDevice -> DrawPrimitive(D3DPT_POINTLIST, 0, 10000);
}

void RenderCone()
{
	g_pd3dDevice -> DrawPrimitive(D3DPT_TRIANGLEFAN, 0, 36);
	g_pd3dDevice -> DrawPrimitive(D3DPT_TRIANGLEFAN, 38, 36);
}


//-----------------------------------------------------------------------------
// The window's message handling function.

LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_DESTROY:
		{
            CleanUp();
            PostQuitMessage(0);
            return 0;
		}

        // Respond to a keyboard event.
		case WM_CHAR: 
            switch (wParam)
            {
				case 'w':
                    g_CameraZ += 0.2f;
                    return 0;
                break;

				case 's':
                    g_CameraZ -= 0.2f;
                    return 0;
                break;

				case 'a':
                    g_CameraX -= 0.2f;
                    return 0;
                break;

				case 'd':
                    g_CameraX += 0.2f;
                    return 0;
                break;

				case 'q':
                    g_CameraY -= 0.2f;
                    return 0;
                break;

				case 'e':
                    g_CameraY += 0.2f;
                    return 0;
                break;
            }
        break;

    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

//-----------------------------------------------------------------------------
// WinMain() - The application's entry point.
// This sort of procedure is mostly standard, and could be used in most
// DirectX applications.

int WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR, int)
{
    // Register the window class
    WNDCLASSEX wc = {sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L,
                     GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
                     "Camera Moving", NULL};
    RegisterClassEx(&wc);

    // Create the application's window
    HWND hWnd = CreateWindow( "Camera Moving", "Moving Camera Example",
                              WS_OVERLAPPEDWINDOW, 100, 100, 300, 300,
                              GetDesktopWindow(), NULL, wc.hInstance, NULL);

    // Initialize Direct3D
    if (SUCCEEDED(SetupD3D(hWnd)))
    {
        // Create the scene geometry
        if (SUCCEEDED(SetupGeometry()))
        {
            // Show the window
            ShowWindow(hWnd, SW_SHOWDEFAULT);
            UpdateWindow(hWnd);

            // Enter the message loop
            MSG msg;
            ZeroMemory(&msg, sizeof(msg));
            while (msg.message != WM_QUIT)
            {
                if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
                else
                    Render();
            }
        }
    }

    UnregisterClass("Camera Moving", wc.hInstance);
    return 0;
}
*/