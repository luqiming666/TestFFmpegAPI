
// TestFFmpegAPIDlg.h: 头文件
//

#pragma once


// CTestFFmpegAPIDlg 对话框
class CTestFFmpegAPIDlg : public CDialogEx
{
// 构造
public:
	CTestFFmpegAPIDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_TESTFFMPEGAPI_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

private:
	CString mSrcFile;

	// Utilities methods
	int _FindVideoStreamIndex(const char* filename, int& codecId);

public:
	afx_msg void OnBnClickedButtonBrowse();
	afx_msg void OnDestroy();

public:
	afx_msg void OnBnClickedButtonPlay();
	afx_msg void OnBnClickedButtonSnapshot();
};
