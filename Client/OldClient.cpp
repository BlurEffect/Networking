/* modification had already begun but mostly old client
//-----------------------------------------------------------------------------
// Basic Camera/Viewpoint Movement Using Direct 3D

#include "stdafx.h"
#include <winsock2.h>
#include <iostream>
#include <string>
#include <fstream>
#include "Enum.h"
#include <mutex>
#include <condition_variable>
#include <future>
#include <thread>
#include <list>
#include "Networker.h"

#pragma comment(lib,"ws2_32.lib") 	// Use this library whilst linking - contains the Winsock2 implementation.

// directX includes below
//-----------------------------------------------------------------------------
// Include these files
#include <Windows.h>	// Windows library (for window functions, menus, dialog boxes, etc)
#include <d3dx9.h>		// Direct 3D library (for all Direct 3D funtions).

// for using PI
#define _USE_MATH_DEFINES
#include <math.h>
#define D3D_DEBUG_INFO

using namespace std;

// networking stuff
struct ClientSharedData
{
public:
	ClientSharedData() : 
		doDraw_(false), finishDrawing_(false), doDrawMutex_(), 
		finishDrawingMutex_(), finishDrawingCondition_(){}
	bool doDraw_;
	bool finishDrawing_;
	mutex doDrawMutex_;
	mutex finishDrawingMutex_;
	condition_variable finishDrawingCondition_;
};

enum DrawableObject
{
	Sphere
};

// holds all information required by the client to draw an object of a specific type
struct DrawingData
{
public:
	// identifier for an object known to the client
	DrawableObject object;
	// horizontal size of the object (length in world space)
	float size;
	// current position of the object (initially starting position)
	float posX, posY, posZ;
	// translation that is applied every frame
	float transX, transY, transZ;
};


// just for testing (in class -> private member of main thread)
ClientSharedData sharedData;
list<DrawingData> drawObjects;
list<DrawingData> newDrawObjects;
mutex drawListMutex;

class NetworkingThread
{
public:
	// account for errors that may occur during creation of the socket
	int operator()(ClientSharedData* sharedData)
	{
		// create an instance of the Networker class to use its functions for communication with the server
		Networker networker;
		
		// Initialise Winsock
		WSADATA WsaDat;
		if (WSAStartup(MAKEWORD(2,2), &WsaDat) != 0)
		{
			std::cout << "Winsock error - Winsock initialization failed\r\n";
			WSACleanup();
			return 0;
		}
	
		// Create our socket
		SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (clientSocket == INVALID_SOCKET)
		{
			std::cout << "Winsock error - Socket creation Failed!\r\n";
			WSACleanup();
			return 0;
		}

		// Resolve IP address for hostname.
		struct hostent *host;

		// Change this to point to server, or ip address...

		if ((host = gethostbyname("localhost")) == NULL)   // In this case 'localhost' is the local machine. Change this to a proper IP address if connecting to another machine on the network.
		{
			std::cout << "Failed to resolve hostname.\r\n";
			WSACleanup();
			return 0;
		}

		// Setup our socket address structure.
		SOCKADDR_IN SockAddr;
		SockAddr.sin_port		 = htons(8888);	// Port number
		SockAddr.sin_family		 = AF_INET;		// Use the Internet Protocol (IP)
		SockAddr.sin_addr.s_addr = *((unsigned long*)host -> h_addr);

		// Attempt to connect to server...
		if (connect(clientSocket, (SOCKADDR*)(&SockAddr), sizeof(SockAddr)) != 0)
		{
			std::cout << "Failed to establish connection with server\r\n";
			WSACleanup();
			return 0;
		}

		// everything went fine, socket created, connection to server established (notify main thread?)

		bool isServerRunning = true;
		// networking stuff
		while(isServerRunning) // while(isServerRunning && doRun)
		{
			ServerCommand serverCommand = DoNothing;
			// as long as there is no command to draw, wait for a signal from the server to start drawing
			// todo: account for other commands
			while(serverCommand == DoNothing)
			{
				// receive a command from the server
				networker.receive<ServerCommand>(clientSocket, &serverCommand, sizeof(ServerCommand), 0);
				// old call: int inDataLength = recv(clientSocket, reinterpret_cast<char*>(&serverCommand), sizeof(ServerCommand), 0);
			}

			if(serverCommand == Draw)
			{
				// receive data from server
				// acquire mutex and modify newDrawObjects, release mutex afterwards
				// wait till finish drawing and notify server
				
				
				{
					// "notify" the main thread to start drawing
					lock_guard<std::mutex> doDrawLock(sharedData ->doDrawMutex_);
					sharedData -> doDraw_ = true;
				}

				{
					// wait for notification from main thread as drawing gets finished
					unique_lock<std::mutex> finishDrawingLock(sharedData -> finishDrawingMutex_);
					sharedData -> finishDrawingCondition_.wait(finishDrawingLock, [sharedData]{return sharedData -> finishDrawing_;});
				}

				// send message to server to issue it to switch to the next client
				ClientState state = Done;
				networker.send<ClientState>(clientSocket, &state, sizeof(ClientState), 0);
				// old call: int outDataLength = send(clientSocket, reinterpret_cast<const char*>(&state), sizeof(ClientState), 0);

				{
					// reset variable
					lock_guard<std::mutex> finishDrawingLock(sharedData -> finishDrawingMutex_);
					sharedData -> finishDrawing_ = false;
				}
			}else
			{
				// command == Disconnect, server is shutting down
				isServerRunning = false;
			}
		}
		
		// finish socket
		// Shutdown our socket.
		shutdown(clientSocket, SD_SEND);

		// Close our socket entirely.
		closesocket(clientSocket);

		// Cleanup Winsock.
		WSACleanup();

		// thread finished
		return 1;
	}
};
// end of networking stuff


//-----------------------------------------------------------------------------
// Global variables

LPDIRECT3D9             g_pD3D           = NULL; // Used to create the D3DDevice
LPDIRECT3DDEVICE9       g_pd3dDevice     = NULL; // The rendering device
LPDIRECT3DVERTEXBUFFER9 g_pVertexBuffer  = NULL; // Buffer to hold vertices for the rectangle

float g_RectX = -19, g_RectY = 0, g_RectZ = 0;

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

// Set up a vertex data.
void SetupVertexGeometry( CUSTOMVERTEX* pV, int index, float x, float y, float z, DWORD c)
{
	pV[index].x = x;
	pV[index].y = y;
	pV[index].z = z;
	pV[index].colour = c;
}

// just for testing
bool isFirstRun = true;

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

	
	// if this is the first run, set initial value for g_RectX (starting point for the sphere)
	if(isFirstRun)
	{
		//RECT windowRec;
		//GetWindowRect(FindWindow(NULL,"Moving Camera Example"), &windowRec);

		// screen coordinates
		//D3DXVECTOR3 sourceVector(static_cast<float>(windowRec.left), static_cast<float>(windowRec.bottom - (windowRec.bottom - windowRec.top)/2.0f), 0.0f);

		//D3DMath_VectorMatrixMultiply
		//D3DXMatrixInverse
		
		//D3DXVec3TransformCoord(
		
		// size of the visible part of the world at the z-position where the sphere moves along
		float sizeOfViewport = (abs(g_CameraZ) * tan(D3DX_PI/8)) * 2.0f;
		//degrees_to_radians
		g_RectX = - sizeOfViewport/2.0f - 5.0f;
		g_RectY = 0;
		g_RectZ = 0;
		isFirstRun = false;
	}
}

//-----------------------------------------------------------------------------
// Render the scene.




bool doDraw = false;

void Render()
{
    // Clear the backbuffer to a black color
    g_pd3dDevice -> Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 250), 1.0f, 0);

    // Begin the scene
    if (SUCCEEDED(g_pd3dDevice -> BeginScene()))
    {
		//cout << "\n" << g_RectX;
		
		void RenderSphereTriangles();

		// Define the viewpoint.
        SetupViewMatrices();


		// get most right position of window
		RECT windowRec;
		GetWindowRect(FindWindow(NULL,"Moving Camera Example"), &windowRec);
		RECT clientArea;
		GetClientRect(FindWindow(NULL,"Moving Camera Example"), &clientArea);
		cout << "\nClientRect: " << clientArea.left << ", " << clientArea.top << ", " << clientArea.right << ", " << clientArea.bottom;


		int border = windowRec.right - (windowRec.right - windowRec.left - clientArea.right)/2.0f;

		// determine the size of the viewport in world space (at the z value where the objects are drawn)
		float sizeOfViewport = (abs(g_CameraZ) * tan(D3DX_PI/8)) * 2.0f;
		
		// superfluous
		float finalborder = sizeOfViewport/2.0f + 5.0f;

		// end get screen position

        // Render the contents of the vertex buffer.
        g_pd3dDevice -> SetStreamSource(0, g_pVertexBuffer, 0, sizeof(CUSTOMVERTEX));
        g_pd3dDevice -> SetFVF(D3DFVF_CUSTOMVERTEX);


		if(!drawObjects.empty())
		{
				
			// loop for every data in draw List
			
			
			// Construct a translation matrix to move the sphere.
			D3DXMATRIX TranslateMat;
			D3DXMatrixTranslation(&TranslateMat, g_RectX, g_RectY, g_RectZ);
			g_pd3dDevice -> SetTransform(D3DTS_WORLD, &TranslateMat);
			// Update the sphere's x co-ordinate.
			g_RectX += 0.05f;
			
			RenderSphereTriangles();

			
			if(g_RectX > finalborder - 10.0f)
			{
				// notify networking thread to start drawing on the next client
				sharedData.finishDrawing_ = true;
				sharedData.finishDrawingCondition_.notify_one();
			}
			// small offset to make sure sphere has passed the border
			if(g_RectX > finalborder + 1)
			{

				// remove element from drawing list
				
				float sizeOfViewport = (abs(g_CameraZ) * tan(D3DX_PI/8)) * 2.0f;
				g_RectX = - sizeOfViewport/2.0f - 5.0f;

				//g_pd3dDevice -> Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0,0,0), 1.0f, 0);
				//countR = 0;
			}
			//++countR;
		}

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
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

//-----------------------------------------------------------------------------
// WinMain() - The application's entry point.
// This sort of procedure is mostly standard, and could be used in most
// DirectX applications.

int WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR, int)
{
	
	// create a console in addition to the window and bind the standard streams to it
	FILE *stream;
	AllocConsole();
	freopen_s(&stream, "CONIN$", "r+t", stdin);
	freopen_s(&stream, "CONOUT$", "w+t", stdout);
	freopen_s(&stream, "CONOUT$", "w+t", stderr);
	

	// Register the window class
    WNDCLASSEX wc = {sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L,
                     GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
                     "Camera Moving", NULL};
    RegisterClassEx(&wc);

    // Create the application's window
    HWND hWnd = CreateWindow( "Camera Moving", "Moving Camera Example",
                              WS_OVERLAPPEDWINDOW, 100, 100, 300, 300,
                              GetDesktopWindow(), NULL, wc.hInstance, NULL);
	
	// get the window rectangle
	RECT windowRec;
	GetWindowRect(hWnd, &windowRec);

	int windowX = windowRec.left;
	int windowY = windowRec.top;
	int windowWidth = windowRec.right - windowRec.left;
	int windowHeight = windowRec.bottom - windowRec.top;

	cout << "\nWindow Rect: " << windowX << ", " << windowY << ", " << windowWidth << ", " << windowHeight;
	
	

	// go full retard/screen
	//HDC hDC = ::GetWindowDC(NULL); 
	//::SetWindowPos(hWnd, NULL, 0, 0, ::GetDeviceCaps(hDC, HORZRES), ::GetDeviceCaps(hDC, VERTRES), SWP_FRAMECHANGED);
    



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

			// create the networking thread
			future<int> networkFuture(async(launch::async, NetworkingThread(), &sharedData));

            while (msg.message != WM_QUIT)
            {
                if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
                else
				{
						Render();
				}
            }
        }
    }

    UnregisterClass("Camera Moving", wc.hInstance);
	// get rid of console
	FreeConsole();
    return 0;
}
*/