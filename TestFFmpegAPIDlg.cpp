
// TestFFmpegAPIDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "TestFFmpegAPI.h"
#include "TestFFmpegAPIDlg.h"
#include "afxdialogex.h"
#include <iostream>
#include "UMiscUtils.h"
#include "UFFmpegUtils.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavutil/fifo.h"
#include "libavutil/imgutils.h"

#ifdef __cplusplus
}
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CTestFFmpegAPIDlg 对话框



CTestFFmpegAPIDlg::CTestFFmpegAPIDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_TESTFFMPEGAPI_DIALOG, pParent)
	, mSrcFile(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CTestFFmpegAPIDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_SRC_FILE, mSrcFile);
}

BEGIN_MESSAGE_MAP(CTestFFmpegAPIDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_BROWSE, &CTestFFmpegAPIDlg::OnBnClickedButtonBrowse)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON_PLAY, &CTestFFmpegAPIDlg::OnBnClickedButtonPlay)
	ON_BN_CLICKED(IDC_BUTTON_SNAPSHOT, &CTestFFmpegAPIDlg::OnBnClickedButtonSnapshot)
END_MESSAGE_MAP()


// CTestFFmpegAPIDlg 消息处理程序

BOOL CTestFFmpegAPIDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	UMiscUtils::EnableConsoleWindow();

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CTestFFmpegAPIDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CTestFFmpegAPIDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CTestFFmpegAPIDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CTestFFmpegAPIDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	// TODO: 在此处添加消息处理程序代码
}

void CTestFFmpegAPIDlg::OnBnClickedButtonBrowse()
{
	CFileDialog fileDlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("MP4 Files (*.mp4)|*.mp4|MP3 Files (*.mp3)|*.mp3|All Files (*.*)|*.*||"), NULL);
	if (fileDlg.DoModal() == IDOK)
	{
		mSrcFile = fileDlg.GetPathName();
		UpdateData(FALSE);
	}
}

typedef struct PacketQueue {
	AVFifo* pkt_list;
	int nb_packets;
	int size;
	int64_t duration;
	int abort_request;
	int serial;
	//SDL_mutex* mutex;
	//SDL_cond* cond;
} PacketQueue;

typedef struct Decoder {
	AVPacket* pkt;
	PacketQueue* queue;
	AVCodecContext* avctx;
	int pkt_serial;
	int finished;
	int packet_pending;
	//SDL_cond* empty_queue_cond;
	int64_t start_pts;
	AVRational start_pts_tb;
	int64_t next_pts;
	AVRational next_pts_tb;
	//SDL_Thread* decoder_tid;
} Decoder;

typedef struct VideoState {
	const AVInputFormat* iformat;
	char* filename;
	int width, height, xleft, ytop;
	int step;

	Decoder auddec;
	Decoder viddec;
	Decoder subdec;

	AVFormatContext* ic;

	AVStream* video_st;

	int last_video_stream, last_audio_stream, last_subtitle_stream;

} VideoState;

// 参考 FFmpeg\fftools\ffplay.c
void CTestFFmpegAPIDlg::OnBnClickedButtonPlay()
{
	int err, ret;
	AVDictionary* format_opts = NULL;

	VideoState* is;
	is = (VideoState*) av_mallocz(sizeof(VideoState));
	is->filename = "D:\\Media\\Gucci.mp4";

	AVPacket* pkt = NULL;
	AVFormatContext* ic = NULL;

	pkt = av_packet_alloc();
	ic = avformat_alloc_context();

	err = avformat_open_input(&ic, is->filename, is->iformat, &format_opts);
	if (err < 0) {
		ret = -1;
		goto fail;
	}

	AVRational frame_rate = av_guess_frame_rate(is->ic, is->video_st, NULL);

	AVFrame* frame = av_frame_alloc();

	avformat_close_input(&is->ic);

fail:
	av_dict_free(&format_opts);
	av_free(is);
}

static int video_frame_count = 0;

static int decode_packet(AVCodecContext* dec, const AVPacket* pkt, AVFrame* frame)
{
	int ret = 0;
	
	// submit the packet to the decoder
	ret = avcodec_send_packet(dec, pkt);
	if (ret < 0) {
		// av_err2str(ret)
		std::cout << "Error submitting a packet for decoding" << std::endl;
		return ret;
	}

	// get all the available frames from the decoder
	while (ret >= 0) {
		ret = avcodec_receive_frame(dec, frame);
		if (ret < 0) {
			// those two return values are special and mean there is no output
			// frame available, but there were no errors during decoding
			if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
				return 0;

			std::cout << "Error during decoding" << std::endl;
			return ret;
		}

		// write the frame data to output file
		if (dec->codec->type == AVMEDIA_TYPE_VIDEO) {
			std::cout << "video_frame n : " << video_frame_count++ << std::endl;
			if (video_frame_count > 125 && video_frame_count < 129) { // test: to save 3 frames
				AVFrame* rgbFrame = UFFmpegUtils::Convert(frame, AV_PIX_FMT_BGR24);
				if (rgbFrame) {
					char szFilename[50];
					sprintf_s(szFilename, sizeof(szFilename), "pic%d.bmp", video_frame_count);
					UFFmpegUtils::SaveRGB24AsBMP(rgbFrame, szFilename);

					av_frame_free(&rgbFrame);
				}
			}
		}

		av_frame_unref(frame);
	}

	return ret;
}

static int open_codec_context(int* stream_idx, AVCodecContext** dec_ctx, AVFormatContext* fmt_ctx, enum AVMediaType type)
{
	int ret, stream_index;
	AVStream* st;
	const AVCodec* dec = NULL;

	ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
	if (ret < 0) {
		std::cout << "Could not find " << av_get_media_type_string(type) << " stream in input file" << std::endl;
		return ret;
	}
	else {
		stream_index = ret;
		st = fmt_ctx->streams[stream_index];

		// find decoder for the stream
		dec = avcodec_find_decoder(st->codecpar->codec_id);
		if (!dec) {
			std::cout << "Failed to find the codec" << std::endl;
			return AVERROR(EINVAL);
		}

		// Allocate a codec context for the decoder
		*dec_ctx = avcodec_alloc_context3(dec);
		if (!*dec_ctx) {
			std::cout << "Failed to allocate the codec context" << std::endl;
			return AVERROR(ENOMEM);
		}

		// Copy codec parameters from input stream to output codec context
		if ((ret = avcodec_parameters_to_context(*dec_ctx, st->codecpar)) < 0) {
			std::cout << "Failed to copy codec parameters to decoder context" << std::endl;
			return ret;
		}

		// Init the decoders
		if ((ret = avcodec_open2(*dec_ctx, dec, NULL)) < 0) {
			std::cout << "Failed to open the codec: " << av_get_media_type_string(type) << std::endl;
			return ret;
		}
		*stream_idx = stream_index;
	}

	return 0;
}

// 参考 FFmpeg\doc\examples\demux_decode.c
void CTestFFmpegAPIDlg::OnBnClickedButtonSnapshot()
{
	//mSrcFile = "D:\\Media\\Gucci.mp4";
	if (mSrcFile.IsEmpty()) return;

	int ret = 0;
	static AVFormatContext* fmt_ctx = NULL;
	static AVCodecContext* video_dec_ctx = NULL;
	static AVStream* video_stream = NULL, * audio_stream = NULL;

	static int video_stream_idx = -1;
	static AVFrame* frame = NULL;
	static AVPacket* pkt = NULL;

	// open input file, and allocate format context
	if (avformat_open_input(&fmt_ctx, (LPCTSTR)mSrcFile, NULL, NULL) < 0) {
		std::cout << "Could not open source file" << std::endl;
		goto Exit;
	}

	// retrieve stream information
	if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
		std::cout << "Could not find stream information" << std::endl;
		goto Exit;
	}

	if (open_codec_context(&video_stream_idx, &video_dec_ctx, fmt_ctx, AVMEDIA_TYPE_VIDEO) >= 0) {
		video_stream = fmt_ctx->streams[video_stream_idx];
		//std::cout << "Image Size: " << video_dec_ctx->width << " x " << video_dec_ctx->height << ", Pixel Format: " << av_get_pix_fmt_name(video_dec_ctx->pix_fmt) << std::endl;
	}

	// dump input information to stderr
	av_dump_format(fmt_ctx, 0, (LPCTSTR)mSrcFile, 0);

	frame = av_frame_alloc();
	if (!frame) {
		std::cout << "Could not allocate frame" << std::endl;
		ret = AVERROR(ENOMEM);
		goto Exit;
	}

	pkt = av_packet_alloc();
	if (!pkt) {
		std::cout << "Could not allocate packet" << std::endl;
		ret = AVERROR(ENOMEM);
		goto Exit;
	}

	// read frames from the file
	while (av_read_frame(fmt_ctx, pkt) >= 0) {
		// check if the packet belongs to a stream we are interested in, otherwise
		// skip it
		if (pkt->stream_index == video_stream_idx) {
			ret = decode_packet(video_dec_ctx, pkt, frame);
		}
		//else if (pkt->stream_index == audio_stream_idx)
		//	ret = decode_packet(audio_dec_ctx, pkt, frame);

		av_packet_unref(pkt);
		if (ret < 0)
			break;
	}

	// flush the decoders
	if (video_dec_ctx)
		decode_packet(video_dec_ctx, NULL, frame);

	std::cout << "Demuxing succeeded." << std::endl;

Exit:
	avcodec_free_context(&video_dec_ctx);
	avformat_close_input(&fmt_ctx);
	av_packet_free(&pkt);
	av_frame_free(&frame);
}
