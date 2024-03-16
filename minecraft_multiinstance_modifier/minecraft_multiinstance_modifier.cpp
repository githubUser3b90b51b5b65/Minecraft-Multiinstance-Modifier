// minecraft_multiinstance_modifier.cpp : Defines the entry point for the application.
//
#pragma comment(lib,"shell32.lib");
#pragma comment(lib."windowsapp.lib")
#include "framework.h"
#include "minecraft_multiinstance_modifier.h"
#include "modify.h"
#define MAX_LOADSTRING 100
// Global Variables:
const wchar_t* text_1 = L"解除 Minecraft 多实例限制";
int g_FontSize = 16;
int colorSwitch = 0;
COLORREF oldColor;
HDC hdc;
HWND hWnd;
PWSTR startPath;
PWSTR newPath;
TCHAR display[MAX_PATH];
wchar_t MCpathstore[MAX_PATH];
wchar_t command[1024];
PWSTR path = NULL;
HWND startButtonSET;
BOOL g_DisableAltF4 = FALSE;
BOOL DontReload = FALSE;
UINT dpi;
UINT dpiX;
HWND startButton;
BOOL TextoutHighDPI_Support = FALSE;
int horizonText = 0;
HFONT g_hFont = NULL;
HFONT g_hFont_Text = NULL;
HANDLE hMutex;
int scaledXDrawText;
int scaledYDrawText;
int screenWidth, screenHeight, startX, startY;
using namespace winrt;
using namespace Windows::Management::Deployment;
using namespace Windows::Foundation;
using namespace Windows::Storage;
// DPI感知处理
void DrawScaledText(HDC hdc, LPCWSTR text, int baseX, int baseY, UINT dpi) {

	int fontSize = -MulDiv(g_FontSize, dpi, 96);
	// 创建适应高DPI的字体
	g_hFont_Text = CreateFont(fontSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
		OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, TEXT("Microsoft YaHei"));
	SelectObject(hdc, g_hFont_Text);
	float scalingFactor = (float)dpi / 96;
	scaledXDrawText = (int)(baseX * scalingFactor);
	scaledYDrawText = (int)(baseY * scalingFactor);

	TextOut(hdc, scaledXDrawText, scaledYDrawText, text, wcslen(text));
}
void AdjustWindowSizeForDPI(HWND hWnd, int baseWidth, int baseHeight, UINT dpi)
{
	float scalingFactor = (float)dpi / 96;
	int scaledWidth = (int)(baseWidth * scalingFactor);
	int scaledHeight = (int)(baseHeight * scalingFactor);
	// 获取屏幕分辨率
	screenWidth = GetSystemMetrics(SM_CXSCREEN);
	screenHeight = GetSystemMetrics(SM_CYSCREEN);

	// 计算左上角的位置，使窗口居中
	startX = (screenWidth - scaledWidth) / 2;
	startY = (screenHeight - scaledHeight) / 2;
	SetWindowPos(hWnd, NULL, startX, startY, scaledWidth, scaledHeight, SWP_NOZORDER);
}
void AdjustButtonForDPI(HWND hWnd, HWND hButton, int baseX, int baseY, int baseWidth, int baseHeight, UINT dpi) {
	float scalingFactor = (float)dpi / 96;
	int scaledX = (int)(baseX * scalingFactor);
	int scaledY = (int)(baseY * scalingFactor);
	int scaledWidth = (int)(baseWidth * scalingFactor);
	int scaledHeight = (int)(baseHeight * scalingFactor);

	SetWindowPos(hButton, NULL, scaledX, scaledY, scaledWidth, scaledHeight, SWP_NOZORDER);
}
// 系统版本检测处理
BOOL IsWindows10OrGreater() {
	OSVERSIONINFOEX osvi;
	DWORDLONG dwlConditionMask = 0;

	// 初始化OSVERSIONINFOEX结构体
	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	osvi.dwMajorVersion = 10; // Windows 10的主版本号

	// 初始化条件掩码
	VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL);

	// 检查系统版本是否满足条件
	return VerifyVersionInfo(&osvi, VER_MAJORVERSION, dwlConditionMask);
}
// 创建字体并应用到按钮
void SetButtonFont(HWND hwndButton, UINT dpi) {
	LOGFONT lf = { 0 };
	lf.lfHeight = -MulDiv(10, dpi, 72); // 根据 dpiY 动态计算 10pt 字体大小
	lf.lfWeight = FW_NORMAL; // 字体粗细
	lf.lfCharSet = DEFAULT_CHARSET; // 字符集
	wcscpy_s(lf.lfFaceName, L"Microsoft YaHei"); // 字体名称

	// 检查是否已存在字体对象
	if (g_hFont) {
		DeleteObject(g_hFont);
	}
	g_hFont = CreateFontIndirect(&lf);
	if (g_hFont) {
		SendMessage(hwndButton, WM_SETFONT, WPARAM(g_hFont), TRUE);
	}
}
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: Place code here.
		// 多实例检测
	HANDLE hMutex = CreateMutexW(NULL, FALSE, L"Global\\Onlyoneinstance_minecraft_multiinstance_modifier_e31b282068f1");
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		// 如果存在，则说明程序已在运行
		MessageBoxW(NULL, L"多实例修改器已经在运行中！", L"错误", MB_ICONEXCLAMATION | MB_OK);
		CloseHandle(hMutex);
		return 0;
	}
	// 版本检测
	if (!IsWindows10OrGreater()) {
		MessageBoxW(NULL, L"此程序只能在NT10或更高版本的Windows系统上运行。", L"错误", MB_ICONERROR);
		return -1;
	}
	// 初始化DPI感知
	//SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	// Initialize global strings
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_MINECRAFTMODIFIER, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MINECRAFTMODIFIER));

	MSG msg;

	// Main message loop:
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = NULL;
	//wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_??));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	//wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_??);
	wcex.lpszClassName = szWindowClass;
	//wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
	wcex.hIconSm = NULL;

	return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // Store instance handle in our global variable

	// 预设窗口样式 (WS_OVERLAPPEDWINDOW)
	DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
	// 创建窗口
	hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, hInstance, nullptr);
	dpi = GetDpiForWindow(hWnd); // 获取窗口所在显示器的DPI
	// 高DPI适配 根据DPI调整窗口大小
	AdjustWindowSizeForDPI(hWnd, 300, 110, dpi);
	// 创建按钮
	startButton = CreateWindow(L"BUTTON", L"确定", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 10, 30, 265, 30, hWnd, (HMENU)ID_BUTTON_CLICK, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
	SetButtonFont(startButton,dpi);
	// 根据DPI调整按钮位置
	AdjustButtonForDPI(hWnd, startButton, 10, 30, 265, 30, dpi);

	if (!hWnd)
	{
		return FALSE;
	}
	// 显示窗口
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);
	return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
DWORD WINAPI ExecuteHiddenCommand(LPVOID lpParam) {
	LPCWSTR command = static_cast<LPCWSTR>(lpParam);

	STARTUPINFOW si = { 0 };
	PROCESS_INFORMATION pi = { 0 };
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;

	wchar_t cmdBuffer[1024];
	wcsncpy_s(cmdBuffer, command, _TRUNCATE);

	BOOL result = CreateProcessW(NULL, cmdBuffer, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

	if (!result) {
		return 0; // 创建进程失败
	}

	// 等待创建的进程结束
	WaitForSingleObject(pi.hProcess, INFINITE);

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return 1;
}
void NewThread(HWND hWnd, const wchar_t* MCpathstore);
void EnableCloseButton(HWND hWnd);
void StartExecuteHiddenCommand(HWND hWnd, LPCWSTR command) {
	HANDLE hThread = CreateThread(NULL, 0, ExecuteHiddenCommand, (LPVOID)command, 0, NULL);
	if (hThread) {
		// 等待线程结束
		WaitForSingleObject(hThread, INFINITE);
		CloseHandle(hThread);
		// 更新UI
		g_FontSize = 16;
		colorSwitch = 1;
		horizonText = 3;
		text_1 = L"修改成功";
		SetWindowTextW(startButtonSET, L"成功");
		EnableCloseButton(hWnd);
		g_DisableAltF4 = FALSE;
		DontReload = TRUE;
		// 请求重绘窗口以更新UI
		InvalidateRect(hWnd, NULL, TRUE);
		UpdateWindow(hWnd);
		//MessageBoxW(hWnd,L"TEST", L"2", MB_OK);
		return;
	}
	else {
		g_FontSize = 16;
		colorSwitch = 2;
		text_1 = L"创建线程失败";
		EnableCloseButton(hWnd);
		g_DisableAltF4 = FALSE;
		DontReload = TRUE;
		// 请求重绘窗口以更新UI
		InvalidateRect(hWnd, NULL, TRUE);
		UpdateWindow(hWnd);
		return;
	}
}
void GetMCPath();
BOOL IsUUID(const wchar_t* fileName) {
	// UUID格式 8-4-4-4-12，总共36个字符
	if (wcslen(fileName) != 36) {
		return false;
	} 
	if (fileName[8] != L'-' || fileName[13] != L'-' || fileName[18] != L'-' || fileName[23] != L'-') {
		return false;
	} 
	return true;
}
void DisableCloseButton(HWND hWnd);
void ModifyXMLFile(const wchar_t* xmlFilePath);
void TraverseDirectory(const wchar_t* directoryPath) {
	WIN32_FIND_DATA findData;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	wchar_t searchPath[MAX_PATH];
	wchar_t fullPath[MAX_PATH];
	std::vector<std::wstring> xmlFilesToProcess;

	// 构建搜索路径, *代表搜索目录下的所有内容
	wsprintf(searchPath, L"%s\\*", directoryPath);
	hFind = FindFirstFile(searchPath, &findData);
	// 未找到任何文件
	if (hFind == INVALID_HANDLE_VALUE) {
		g_FontSize = 16;
		colorSwitch = 2;
		text_1 = L"找不到对应文件";
		SetWindowTextW(startButtonSET, L"失败");
		DontReload = TRUE;
		return;
	}
	do {
		if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			// 排除当前目录(.)和上级目录(..)
			if (wcscmp(findData.cFileName, L".") != 0 && wcscmp(findData.cFileName, L"..") != 0) {
				if (IsUUID(findData.cFileName)) {
					// 构建子目录路径
					wsprintf(fullPath, L"%s\\%s", directoryPath, findData.cFileName);

					// 构建子目录下可能的AppxManifest.xml文件路径
					wchar_t xmlFilePath[MAX_PATH];
					wsprintf(xmlFilePath, L"%s\\AppxManifest.xml", fullPath);

					// 检查文件是否存在
					if (GetFileAttributes(xmlFilePath) != INVALID_FILE_ATTRIBUTES) {
						// 存在，则将其添加到处理列表中
						xmlFilesToProcess.push_back(xmlFilePath);
					}
				}
			}
		}
	} while (FindNextFile(hFind, &findData) != 0);

	FindClose(hFind);

	// 遍历结束后，处理所有XML文件
	for (const auto& xmlFilePath : xmlFilesToProcess) {
		ModifyXMLFile(xmlFilePath.c_str());
	}
	//
	return;
}
void NewThread(HWND hWnd, const wchar_t* MCpathstore) {
	std::thread([hWnd, MCpathstore]() {
		wchar_t command[2048];
		swprintf(command, 2048, L"powershell -Command \"Get-AppxPackage Microsoft.MinecraftUWP* | Remove-AppxPackage -PreserveApplicationData; Add-AppxPackage -Register '%s'\"", MCpathstore);
		StartExecuteHiddenCommand(hWnd, command);
		}).detach(); // detach()让线程在后台运行
}
void DisableCloseButton(HWND hWnd){
	HMENU hSysMenu = GetSystemMenu(hWnd, FALSE);
	if (hSysMenu != NULL)
	{

		EnableMenuItem(hSysMenu, SC_CLOSE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
	}
}
void EnableCloseButton(HWND hWnd) {
	HMENU hSysMenu = GetSystemMenu(hWnd, FALSE);
	if (hSysMenu != NULL)
	{

		EnableMenuItem(hSysMenu, SC_CLOSE, MF_BYCOMMAND | MF_ENABLED);
	}
}
void GetMCPath() {
	//先获取当前安装的MC的目录
	init_apartment();

	PackageManager packageManager;
	auto packages = packageManager.FindPackagesForUser(L"", L"Microsoft.MinecraftUWP_8wekyb3d8bbwe");

	std::wstring path;
	for (const auto& package : packages) {
		// 获取安装位置
		auto installedLocation = package.InstalledLocation();
		path = installedLocation.Path().c_str();
		// 只获取一次
		break;
	}
	if (!path.empty()) {
		// 如果找到了路径，就显示它
		//MessageBoxW(nullptr, path.c_str(), L"MCBE installation path TEST", MB_OK);
		wcscpy_s(MCpathstore, MAX_PATH, path.c_str());
		wcscat_s(MCpathstore, MAX_PATH, L"\\AppxManifest.xml");

	}
	else {
		// 如果没有找到路径，显示一条错误消息
		colorSwitch = 2;
		g_FontSize = 16;
		horizonText = 2;
		text_1 = L"未找到 Minecraft 安装目录";
		SetWindowTextW(startButtonSET, L"失败");
		DontReload = TRUE;
		return;
	}
}
void ModifyXMLFile(const wchar_t* xmlFilePath) {
	// 用于存储文件内容的字符串
	std::wstringstream fileContentStream;

	// 打开文件读取内容
	std::wifstream file(xmlFilePath);
	if (file.is_open()) {
		fileContentStream << file.rdbuf();
		file.close();
	}
	else {
		// 文件打开失败，可能需要记录日志或处理错误
		colorSwitch = 2;
		text_1 = L"文件读取失败";
		//text_1_X = 80;
		g_FontSize = 16;
		SetWindowTextW(startButtonSET, L"失败");
		DontReload = TRUE;
		return;
	}

	// 将文件内容转换为std::wstring以方便处理
	std::wstring fileContent = fileContentStream.str();

	// 检查是否已经包含了需要添加的字符串
	std::wstring namespaceStr_checking = L"xmlns:desktop4=\"http://schemas.microsoft.com/appx/manifest/desktop/windows10/4\"";
	std::wstring multipleInstancesStr_checking = L"desktop4:SupportsMultipleInstances=\"true\"";
	if (fileContent.find(namespaceStr_checking) != std::wstring::npos || fileContent.find(multipleInstancesStr_checking) != std::wstring::npos) {
		// 如果已经包含了这些字符串，表示已经修改过，取消操作
		return;
	}

	std::wstring searchString = L"<Package xmlns=";
	size_t pos = fileContent.find(searchString);
	if (pos != std::wstring::npos) {
		// 找第一个">"以确定插入位置
		size_t insertPos = fileContent.find(L">", pos);
		if (insertPos != std::wstring::npos) {
			// 构造要插入的字符串
			std::wstring toInsert = L" xmlns:desktop4=\"http://schemas.microsoft.com/appx/manifest/desktop/windows10/4\"";
			// 在">"符号前插入字符串
			fileContent.insert(insertPos, toInsert);
		}
		else {
			// 文件打开失败，可能需要记录日志或处理错误
			colorSwitch = 2;
			//text_1_X = 110;
			g_FontSize = 16;
			text_1 = L"修改失败";
			SetWindowTextW(startButtonSET, L"失败");
			DontReload = TRUE;
			return;
		}
		// 再次查找 "<Application Id=" 部分
		std::wstring appSearchString = L"<Application Id=";
		pos = fileContent.find(appSearchString);
		if (pos != std::wstring::npos) {
			// 找第一个">"以确定插入位置
			size_t appInsertPos = fileContent.find(L">", pos);
			if (appInsertPos != std::wstring::npos) {
				// 构造要插入的字符串
				std::wstring appToInsert = L" desktop4:SupportsMultipleInstances=\"true\"";
				// 在">"符号前插入字符串
				fileContent.insert(appInsertPos, appToInsert);
			}
			else {
				colorSwitch = 2;
				g_FontSize = 16;
				horizonText = 4;
				text_1 = L"修改失败";
				SetWindowTextW(startButtonSET, L"失败");
				DontReload = TRUE;
				return;
			}
		}
		// 将修改后的内容写回文件
		std::wofstream outFile(xmlFilePath);
		if (outFile.is_open()) {
			outFile << fileContent;
			outFile.close();
		}
		else {
			colorSwitch = 2;
			//text_1_X = 80;
			g_FontSize = 16;
			text_1 = L"文件保存失败";
			SetWindowTextW(startButtonSET, L"失败");
			DontReload = TRUE;
			return;
		}
	}
	else {
		colorSwitch = 2;
		//text_1_X = 110;
		g_FontSize = 16;
		text_1 = L"修改失败";
		SetWindowTextW(startButtonSET, L"失败");
		DontReload = TRUE;
		return;
	}
}



void ModifyFile() {
	WIN32_FIND_DATA findData;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	wchar_t searchPath[MAX_PATH];
	// 获取AppData\Roaming路径
	HRESULT result = SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &path);
	if (result == S_OK) {
		// 计算新路径的总长度
		size_t additionalPathLength = wcslen(L"\\.minecraft_bedrock\\versions") + 1;
		size_t newPathLength = wcslen(path) + additionalPathLength;
		// 为新路径分配内存
		newPath = (PWSTR)malloc(newPathLength * sizeof(WCHAR));
		// 复制原始路径到新路径
		wcscpy_s(newPath, newPathLength, path);
		// 追加额外的路径字符串
		wcscat_s(newPath, newPathLength, L"\\.minecraft_bedrock\\versions");
		startPath = newPath;
		TraverseDirectory(startPath);
		//wcscpy_s(display, newPath);
		//MessageBoxW(hWnd, display, L"2", MB_OK);
	}
	// 重载判断
	if (DontReload == FALSE) {
		// 暂时禁用关闭按钮
		GetMCPath();
		DisableCloseButton(hWnd);
		g_DisableAltF4 = TRUE;
		g_FontSize = 16;
		colorSwitch = 0;
		text_1 = L"正在尝试重载 Minecraft...";
		// 请求重绘窗口以更新UI
		InvalidateRect(hWnd, NULL, TRUE);
		UpdateWindow(hWnd);
		// 防止程序主线程被占用，在新线程执行操作
		NewThread(hWnd, MCpathstore);
	}
	// 重绘窗口
	CoTaskMemFree(path);
	InvalidateRect(hWnd, NULL, TRUE);
	UpdateWindow(hWnd);
}
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

	switch (message)
	{
	case WM_DPICHANGED:{
		dpiX = LOWORD(wParam);
		//UINT dpiY = HIWORD(wParam);
		// 根据新DPI重新调整窗口和控件的大小
		AdjustWindowSizeForDPI(hWnd, 300, 110, dpiX);
		AdjustButtonForDPI(hWnd, startButton, 10, 30, 265, 30, dpiX);
		SetButtonFont(startButton, dpiX);
		TextoutHighDPI_Support = TRUE;
		InvalidateRect(hWnd, NULL, TRUE);
		UpdateWindow(hWnd);
	}
	break;
	case WM_CREATE:
	{

	}
	break;
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case ID_BUTTON_CLICK:
			startButtonSET = GetDlgItem(hWnd, ID_BUTTON_CLICK);
			EnableWindow(startButtonSET, FALSE);
			SetWindowTextW(startButtonSET, L"等待");
			ModifyFile();
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code that uses hdc here...
		HFONT hFont;
		hFont = CreateFont(MulDiv(g_FontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Microsoft YaHei"));
		SelectObject(hdc, hFont);
		if (colorSwitch == 1)
		{
			oldColor = SetTextColor(hdc, RGB(87, 166, 74)); // 绿
		}
		else if (colorSwitch == 0)
		{
			oldColor = SetTextColor(hdc, RGB(0, 0, 0)); // 黑
		}
		else if (colorSwitch == 2)
		{
			oldColor = SetTextColor(hdc, RGB(255, 0, 0)); // 红
		}
		// TEXT部分
		//TextOut(hdc, text_1_X, text_1_Y, text_1, wcslen(text_1));
		if (!TextoutHighDPI_Support) {
			if (horizonText == 0) {
				DrawScaledText(hdc, text_1, 45, 4, dpi);
			}
			else if (horizonText == 1) {
				DrawScaledText(hdc, text_1, 15, 4, dpi);
			}
			else if (horizonText == 2) {
				DrawScaledText(hdc, text_1, 45, 4, dpi);
			}
			else if (horizonText == 3) {
				DrawScaledText(hdc, text_1, 110, 4, dpi);
			}
			else if (horizonText == 4) {
				DrawScaledText(hdc, text_1, 110, 4, dpi);
			}
		}
		else {
			if (horizonText == 0) {
				DrawScaledText(hdc, text_1, 45, 4, dpiX);
			}
			else if (horizonText == 1) {
				DrawScaledText(hdc, text_1, 15, 4, dpiX);
			}
			else if (horizonText == 2) {
				DrawScaledText(hdc, text_1, 45, 4, dpiX);
			}
			else if (horizonText == 3) {
				DrawScaledText(hdc, text_1, 110, 4, dpiX);
			}
			else if (horizonText == 4) {
				DrawScaledText(hdc, text_1, 110, 4, dpiX);
			}
		}
		DeleteObject(g_hFont_Text);
		EndPaint(hWnd, &ps);
	}
	break;
	case WM_CLOSE:
		if (g_DisableAltF4) {
			return 0;
		}
		DestroyWindow(hWnd);
	case WM_DESTROY: {
		if (g_hFont) {
			DeleteObject(g_hFont);
			g_hFont = NULL;
		}
		if (g_hFont_Text) {
			DeleteObject(g_hFont_Text);
			g_hFont_Text = NULL;
		}
		CloseHandle(hMutex);
		PostQuitMessage(0);
	}
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
