
// TestFFmpegAPIDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "TestFFmpegAPI.h"
#include "TestFFmpegAPIDlg.h"
#include "afxdialogex.h"
#include <iostream>
#include "UMiscUtils.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavutil/fifo.h"
//#include "libavdevice/avdevice.h"

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

#define INBUF_SIZE 4096

static int decode_video(AVCodecContext* dec_ctx, AVFrame* frame, AVPacket* pkt)
{
	int ret = avcodec_send_packet(dec_ctx, pkt);
	if (ret < 0) {
		std::cout << "Error sending a packet for decoding... code: " << ret << std::endl;
		if (ret == AVERROR(EAGAIN)) {
			std::cout << "EAGAIN" << std::endl;
		}
		else if (ret == AVERROR(EINVAL)) {
			std::cout << "EINVAL" << std::endl;
		}
		else if (ret == AVERROR(ENOMEM)) {
			std::cout << "ENOMEM" << std::endl;
		}
		else {
			std::cout << "Unknown..." << std::endl;
		}
		return ret;
	}

	while (ret >= 0) {
		ret = avcodec_receive_frame(dec_ctx, frame);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			return 0;
		else if (ret < 0) {
			std::cout << "Error during decoding" << std::endl;
			return ret;
		}

		//printf("saving frame %3"PRId64"\n", dec_ctx->frame_num);
		//fflush(stdout);

		/* the picture is allocated by the decoder. no need to
		   free it */
		char buf[100];
		snprintf(buf, sizeof(buf), "Pic-%d.bmp", dec_ctx->frame_num);
		UMiscUtils::SaveFrameToBMP(frame->data[0], frame->width, frame->height, 24, buf);
		//pgm_save(frame->data[0], frame->linesize[0],
		//	frame->width, frame->height, buf);
	}

	return 0;
}

// 参考 FFmpeg\doc\examples\decode_video.c
void CTestFFmpegAPIDlg::OnBnClickedButtonSnapshot()
{
	mSrcFile = "D:\\Media\\bear.wmv";

	const AVCodec* codec;
	AVCodecParserContext* parser = NULL;
	AVCodecContext* c = NULL;
	FILE* f = NULL;
	AVFrame* frame = NULL;
	uint8_t inbuf[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
	uint8_t* data = NULL;
	size_t   data_size;
	int ret;
	int eof;
	AVPacket* pkt;

	int codec_id = 0;
	int videoStream = _FindVideoStreamIndex((LPCTSTR)mSrcFile, codec_id);
	if (videoStream == -1) {
		std::cout << "Failed to find a video stream in the source file." << std::endl;
		goto fail;
	}

	pkt = av_packet_alloc();
	if (!pkt)
		goto fail;

	/* set end of buffer to 0 (this ensures that no overreading happens for damaged MPEG streams) */
	memset(inbuf + INBUF_SIZE, 0, AV_INPUT_BUFFER_PADDING_SIZE);

	/* find a specific video decoder */
	codec = avcodec_find_decoder((AVCodecID)codec_id);
	if (!codec) {
		std::cout << "Codec not found" << std::endl;
		goto fail;
	}

	parser = av_parser_init(codec->id);
	if (!parser) {
		std::cout << "Parser not found" << std::endl;
		goto fail;
	}

	c = avcodec_alloc_context3(codec);
	if (!c) {
		std::cout << "Could not allocate video codec context" << std::endl;
		goto fail;
	}

	/* For some codecs, such as msmpeg4 and mpeg4, width and height
	   MUST be initialized there because this information is not
	   available in the bitstream. */

	   /* Copy codec parameters from input stream to output codec context */
	if ((ret = avcodec_parameters_to_context(c, st->codecpar)) < 0) {
		fprintf(stderr, "Failed to copy %s codec parameters to decoder context\n",
			av_get_media_type_string(type));
		return ret;
	}

	   /* open it */
	if (avcodec_open2(c, codec, NULL) < 0) {
		std::cout << "Could not open codec" << std::endl;
		goto fail;
	}

	ret = fopen_s(&f, (LPCTSTR)mSrcFile, "rb");
	if (ret != 0) {
		std::cout << "Could not open the source file" << std::endl;
		goto fail;
	}

	frame = av_frame_alloc();
	if (!frame) {
		std::cout << "Could not allocate video frame" << std::endl;
		goto fail;
	}

	do {
		/* read raw data from the input file */
		data_size = fread(inbuf, 1, INBUF_SIZE, f);
		if (ferror(f))
			break;
		eof = !data_size;

		/* use the parser to split the data into frames */
		data = inbuf;
		while (data_size > 0 || eof) {
			ret = av_parser_parse2(parser, c, &pkt->data, &pkt->size,
				data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
			if (ret < 0) {
				std::cout << "Error while parsing" << std::endl;
				goto fail;
			}
			data += ret;
			data_size -= ret;

			if (pkt->size && pkt->stream_index == videoStream)
				decode_video(c, frame, pkt);
			else if (eof)
				break;
		}
	} while (!eof);

	/* flush the decoder */
	decode_video(c, frame, NULL);

fail:
	if (f) fclose(f);
	av_parser_close(parser);
	avcodec_free_context(&c);
	av_frame_free(&frame);
	av_packet_free(&pkt);
}

int CTestFFmpegAPIDlg::_FindVideoStreamIndex(const char* filename, int& codecId)
{
	int videoStream = -1;

	// Open video file
	AVFormatContext* pFormatCtx = NULL;
	if (avformat_open_input(&pFormatCtx, filename, NULL, NULL) == 0) {
		// Dump information about the source file
		av_dump_format(pFormatCtx, 0, filename, false);

		// Retrieve stream information
		if (avformat_find_stream_info(pFormatCtx, NULL) == 0) {
			// Find the first video stream
			for (int i = 0; i < pFormatCtx->nb_streams; i++) {
				if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
					videoStream = i;
					codecId = pFormatCtx->streams[i]->codecpar->codec_id;
					break;
				}
			}
		}
	}
	else {
		std::cout << "Could not open the source file." << std::endl;
	}
	
	// Close the video file
	if (pFormatCtx) {
		avformat_close_input(&pFormatCtx);
	}

	return videoStream;
}