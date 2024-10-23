#include "pch.h"
#include "UMiscUtils.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <memory>
#include <thread>
#include <mmreg.h>
#include <shlobj.h>

#pragma comment(lib, "version.lib")

namespace UMiscUtils {

	CString GetRuntimeFilePath(TCHAR* filename, TCHAR* subFolder)
	{
		TCHAR szPath[MAX_PATH] = { 0 };
		::GetModuleFileName(NULL, szPath, MAX_PATH);

		CString strProcessPath(szPath);

		// ɾ���ļ������֣��õ���ͬĿ¼��·��
		PathRemoveFileSpec(strProcessPath.GetBuffer());
		strProcessPath.ReleaseBuffer();

		// ƴ��Ŀ���ļ�������·��
		if (subFolder) {
			TCHAR strTemp[MAX_PATH] = { 0 };
			PathCombine(strTemp, strProcessPath, subFolder);
			strProcessPath = strTemp;
		}

		if (filename) {
			CString strFinalFilePath;
			PathCombine(strFinalFilePath.GetBuffer(MAX_PATH), strProcessPath, filename);
			strFinalFilePath.ReleaseBuffer();

			return strFinalFilePath;
		}
		else {
			return strProcessPath;
		}
	}
	
	CString GetExeFileVersion()
	{
		TCHAR	szFileName[MAX_PATH];
		DWORD	dwHandle = 0;
		DWORD	dwSize = 0;
		TCHAR* lpData = NULL;
		VS_FIXEDFILEINFO* pvsInfo = NULL;
		UINT	unLen = 0;

		DWORD	dwFileVersionMS = 0;
		DWORD	dwFileVersionLS = 0;

		CString strBuild, strVersion;
		::memset(szFileName, NULL, sizeof(TCHAR) * _MAX_PATH);
		::GetModuleFileName(NULL, szFileName, MAX_PATH);

		if ((dwSize = ::GetFileVersionInfoSize(szFileName, &dwHandle)) > 0)
		{
			lpData = new TCHAR[dwSize + 1];
			if (::GetFileVersionInfo(szFileName, dwHandle, dwSize, lpData))
			{
				if (::VerQueryValue((const LPVOID)lpData, _T("\\"), (LPVOID*)&pvsInfo, &unLen))
				{
					dwFileVersionMS = pvsInfo->dwFileVersionMS;
					dwFileVersionLS = pvsInfo->dwFileVersionLS;

					strVersion.Format(_T("%d.%d.%d.%d"), dwFileVersionMS / (0xFFFF + 1), dwFileVersionMS % (0xFFFF + 1),
						dwFileVersionLS / (0xFFFF + 1), dwFileVersionLS % (0xFFFF + 1));
				}
			}
			delete[] lpData;
		}

		return strVersion;
	}

	CString GetProgramDataPath(TCHAR* subFolder, TCHAR* filename)
	{
		TCHAR programDataPath[MAX_PATH];
		if (SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA, NULL, 0, programDataPath) != S_OK) {
			return _T("");
		}

		CString fullPath;
		PathCombine(fullPath.GetBuffer(MAX_PATH), programDataPath, subFolder);
		fullPath.ReleaseBuffer();
		if (!PathFileExists(fullPath)) {
			CreateDirectory(fullPath, NULL);
		}

		fullPath += _T("\\"); // ���·���ָ���
		fullPath += filename;
		return fullPath;
	}

	#pragma warning(disable : 4996)
	std::string UnicodeToANSI(const std::wstring& wstr)
	{
		std::string ret;
		std::mbstate_t state = {};
		const wchar_t* src = wstr.data();
		size_t len = std::wcsrtombs(nullptr, &src, 0, &state);
		if (static_cast<size_t>(-1) != len) {
			std::unique_ptr< char[] > buff(new char[len + 1]);
			len = std::wcsrtombs(buff.get(), &src, len, &state);
			if (static_cast<size_t>(-1) != len) {
				ret.assign(buff.get(), len);
			}
		}
		return ret;
	}

	std::wstring ANSIToUnicode(const std::string& str)
	{
		std::wstring ret;
		std::mbstate_t state = {};
		const char* src = str.data();
		size_t len = std::mbsrtowcs(nullptr, &src, 0, &state);
		if (static_cast<size_t>(-1) != len) {
			std::unique_ptr< wchar_t[] > buff(new wchar_t[len + 1]);
			len = std::mbsrtowcs(buff.get(), &src, len, &state);
			if (static_cast<size_t>(-1) != len) {
				ret.assign(buff.get(), len);
			}
		}
		return ret;
	}

	void PrintThreadInfo(const std::string& tag)
	{
		std::thread::id threadId = std::this_thread::get_id();
		std::cout << "*** " << tag << " **** Thread ID: " << threadId << std::endl;
	}

	void EnableConsoleWindow()
	{
		if (::GetConsoleWindow() == NULL)
		{
			if (::AllocConsole())
			{
				FILE* stream;
				freopen_s(&stream, "CONOUT$", "w", stdout);
			}
		}
	}

	void WriteWaveFileHeader(std::fstream& file, UINT32 sampleRate, UINT16 numChannels, UINT16 bitsPerSample, UINT32 pcmDataSize)
	{
		// WAVE�ļ�ͷ
		WAVEFORMATEX waveFormat;
		waveFormat.wFormatTag = WAVE_FORMAT_PCM;
		waveFormat.nChannels = numChannels;
		waveFormat.nSamplesPerSec = sampleRate;
		waveFormat.nAvgBytesPerSec = sampleRate * numChannels * (bitsPerSample / 8);
		waveFormat.nBlockAlign = numChannels * (bitsPerSample / 8);
		waveFormat.wBitsPerSample = bitsPerSample;
		waveFormat.cbSize = 0;

		// �������ݿ�ͷ
		DWORD chunkSize = 36 + pcmDataSize;
		DWORD fmtSize = sizeof(WAVEFORMATEX);

		// д��RIFF��ʶ���ļ���С    
		file.write("RIFF", 4);
		file.write((const char*)&chunkSize, 4);

		// д��WAVE��ʶ    
		file.write("WAVE", 4);

		// д��fmt�ӿ�    
		file.write("fmt ", 4);
		file.write((const char*)&fmtSize, 4);
		file.write((const char*)&waveFormat, sizeof(WAVEFORMATEX));

		// д��data�ӿ�    
		file.write("data", 4);
		file.write((const char*)&pcmDataSize, 4);

		// ������PCM����...
	}

	bool RunExternalApp(const TCHAR* appPath, TCHAR* appParams, bool bSync)
	{
		// ����������Ϣ�ṹ��
		PROCESS_INFORMATION processInfo;
		ZeroMemory(&processInfo, sizeof(processInfo));

		// ����������Ϣ�ṹ��
		STARTUPINFO startupInfo;
		ZeroMemory(&startupInfo, sizeof(startupInfo));
		startupInfo.cb = sizeof(startupInfo);

		// �����ⲿ���
		BOOL bSuccess = CreateProcess(
			appPath,					// �ⲿ�����·��
			appParams,					// �����в���
			NULL,                       // Ĭ�ϰ�ȫ��������
			NULL,                       // Ĭ�ϰ�ȫ��������
			FALSE,                      // ���̳о��
			CREATE_NO_WINDOW,           // ��Ϊ�½��̴���CUI����
			NULL,                       // Ĭ�ϻ�������
			NULL,                       // Ĭ�Ϲ���Ŀ¼
			&startupInfo,               // ������Ϣ�ṹ��
			&processInfo                // ������Ϣ�ṹ��
		);

		if (bSuccess && bSync)
		{
			// �ȴ������˳�
			WaitForSingleObject(processInfo.hProcess, INFINITE);
		}

		// �رս��̺��̵߳ľ��
		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);

		return bSuccess == TRUE;
	}

	bool RunExternalApp(TCHAR* fullCmd, std::string* outInfo, bool bSync)
	{
		// ����������Ϣ�ṹ��
		PROCESS_INFORMATION processInfo;
		ZeroMemory(&processInfo, sizeof(processInfo));

		// ����������Ϣ�ṹ��
		STARTUPINFO startupInfo;
		ZeroMemory(&startupInfo, sizeof(startupInfo));
		startupInfo.cb = sizeof(startupInfo);

		// �Ƿ���Ҫ��ȡ����Ŀ���̨������ݣ�
		HANDLE hReadPipe = NULL;
		HANDLE hWritePipe = NULL;
		BOOL bPipeCreated = FALSE;
		if (outInfo) {
			SECURITY_ATTRIBUTES saAttr;
			saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
			saAttr.bInheritHandle = TRUE;
			saAttr.lpSecurityDescriptor = NULL;
			bPipeCreated = CreatePipe(&hReadPipe, &hWritePipe, &saAttr, 0);

			startupInfo.hStdError = hWritePipe;
			startupInfo.hStdOutput = hWritePipe;
			startupInfo.dwFlags |= STARTF_USESTDHANDLES;
		}

		//if (!visible) {
			//startupInfo.dwFlags = STARTF_USESHOWWINDOW;
			//startupInfo.wShowWindow = SW_HIDE; // ���������д���
		//}		

		// �����ⲿ���
		BOOL bSuccess = CreateProcess(
			NULL,						// �ⲿ�����·��
			fullCmd,					// �����в���
			NULL,                       // Ĭ�ϰ�ȫ��������
			NULL,                       // Ĭ�ϰ�ȫ��������
			TRUE,						// �̳о��!!!
			CREATE_NO_WINDOW,           // ��Ϊ�½��̴���CUI����
			NULL,                       // Ĭ�ϻ�������
			NULL,                       // Ĭ�Ϲ���Ŀ¼
			&startupInfo,               // ������Ϣ�ṹ��
			&processInfo                // ������Ϣ�ṹ��
			);

		if (bSuccess && bSync)
		{
			// �ȴ������˳�
			WaitForSingleObject(processInfo.hProcess, 1000);	
		}

		// ��ȡ�ܵ�����
		if (bPipeCreated) {
			CloseHandle(hWritePipe);

			DWORD dwRead;
			CHAR chBuf[4096];
			BOOL success;
			while (true) {
				success = ReadFile(hReadPipe, chBuf, sizeof(chBuf), &dwRead, NULL);
				if (!success || dwRead == 0) break;
				std::string s(chBuf, dwRead);
				*outInfo += s;
			}
			CloseHandle(hReadPipe);
		}
		
		// �رս��̺��̵߳ľ��
		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);

		return bSuccess == TRUE;
	}

	int SaveFrameToBMP(BYTE* pFrame, int nWidth, int nHeight, int nBitCount, const char* outfile)
	{
		FILE* fp = fopen(outfile, "wb");
		if (NULL == fp)
		{
			printf("file open error %s\n", outfile);
			return -1;
		}

		BITMAPFILEHEADER bmpheader;
		BITMAPINFOHEADER bmpinfo;
		memset(&bmpheader, 0, sizeof(BITMAPFILEHEADER));
		memset(&bmpinfo, 0, sizeof(BITMAPINFOHEADER));

		// set BITMAPFILEHEADER value  
		bmpheader.bfType = ('M' << 8) | 'B';
		bmpheader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
		bmpheader.bfSize = bmpheader.bfOffBits + nWidth * nHeight * nBitCount / 8;

		// set BITMAPINFOHEADER value  
		bmpinfo.biSize = sizeof(BITMAPINFOHEADER);
		bmpinfo.biWidth = nWidth;
		bmpinfo.biHeight = 0 - nHeight;
		bmpinfo.biPlanes = 1;
		bmpinfo.biBitCount = nBitCount;
		bmpinfo.biCompression = BI_RGB;
		bmpinfo.biSizeImage = 0;
		bmpinfo.biXPelsPerMeter = 100;
		bmpinfo.biYPelsPerMeter = 100;
		bmpinfo.biClrUsed = 0;
		bmpinfo.biClrImportant = 0;

		// write pic file  
		fwrite(&bmpheader, sizeof(BITMAPFILEHEADER), 1, fp);
		fwrite(&bmpinfo, sizeof(BITMAPINFOHEADER), 1, fp);
		fwrite(pFrame, nWidth * nHeight * nBitCount / 8, 1, fp);
		fclose(fp);
		return 0;
	}
}