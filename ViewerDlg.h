
// ViewerDlg.h: 헤더 파일
//

#pragma once

#define MAX_FILES 20000 // 최대 이미지 파일 개수
struct FILE_LIST
{
	CString m_FileName; // 리스트 박스에 등록된 파일의 "파일이름"
	CString m_FileExt; // 확장자
};

// CViewerDlg 대화 상자
class CViewerDlg : public CDialogEx
{
// 생성입니다.
public:
	CViewerDlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.

	// ***********************************************
	FILE_LIST m_Gallery[MAX_FILES]; // 파일 목록 저장용 버퍼 배열
	CString m_FilePath; // 리스트 박스에 등록된 파일의 "경로 + 폴더명"
	cv::Mat LoadImageFile();

	// ***********************************************
	// YOLOv4
	// ***********************************************
	CString m_strHome; // 실행 파일이 저장된 폴더 경로
	BOOL LoadClassName(const CString strFilePath); // 클래스 이름 목록 파일 읽기
	std::vector<bbox_t> DETECTOR(cv::Mat cv_image); // 객체 탐지 라이브러리 호출
	std::vector<CString> m_vecClassName; // 클래스 이름 목록을 저장하는 벡터
	Detector* m_pDetector; // YOLO 라이브러리 객체 포인터
	void DrawObjectInformation(std::vector<bbox_t>& objects, cv::Mat& image);

	// ***********************************************

	// ***********************************************
	CString GetVideoFileName(); // 비디오 파일 이름 읽기
	void DisplayVideoStream(CString strVideoFileName); // 비디오 파일 읽어서 화면에
	

	// ***********************************************
	void DisplayWebcamImage(); // 웹캠 이미지 프레임 디스플레이
	// ***********************************************
	
// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_VIEWER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.


// 구현입니다.
protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CString m_str_message;
	CListCtrl m_list_image;
	afx_msg void OnBnClickedButtonLoadFileList();

	afx_msg void OnLvnItemchangedListImage(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBnClickedButtonClose();
	afx_msg void OnBnClickedButtonOpenVideo();
	afx_msg void OnBnClickedButtonWebcam();
};
