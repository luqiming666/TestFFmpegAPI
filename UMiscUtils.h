#pragma once

#include <afxwin.h>
#include <string>
#include <fstream>

namespace UMiscUtils {

	CString GetRuntimeFilePath(TCHAR* filename = NULL, TCHAR* subFolder = NULL);
	CString GetExeFileVersion();
	CString GetProgramDataPath(TCHAR* subFolder, TCHAR* filename);

	std::string UnicodeToANSI(const std::wstring& wstr);
	std::wstring ANSIToUnicode(const std::string& str);

	void PrintThreadInfo(const std::string& tag);
	void EnableConsoleWindow();

	void WriteWaveFileHeader(std::fstream& file, UINT32 sampleRate, UINT16 numChannels, UINT16 bitsPerSample, UINT32 pcmDataSize = 0);
	int SaveFrameToBMP(BYTE* pFrame, int nWidth, int nHeight, int nBitCount, const char* outfile);

	bool RunExternalApp(const TCHAR* appPath, TCHAR* appParams, bool bSync = false);
	bool RunExternalApp(TCHAR* fullCmd, std::string* outInfo, bool bSync = false);
}