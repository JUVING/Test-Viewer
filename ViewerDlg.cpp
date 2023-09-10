
// ViewerDlg.cpp: 구현 파일
//

#include "pch.h"
#include "framework.h"
#include "Viewer.h"
#include "ViewerDlg.h"
#include "afxdialogex.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CViewerDlg 대화 상자



CViewerDlg::CViewerDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_VIEWER_DIALOG, pParent)
	, m_str_message(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CViewerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_STATIC_MESSAGE, m_str_message);
	DDX_Control(pDX, IDC_LIST_IMAGE, m_list_image);
}

BEGIN_MESSAGE_MAP(CViewerDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_LOAD_FILE_LIST, &CViewerDlg::OnBnClickedButtonLoadFileList)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_IMAGE, &CViewerDlg::OnLvnItemchangedListImage)
	ON_BN_CLICKED(IDC_BUTTON_CLOSE, &CViewerDlg::OnBnClickedButtonClose)
	ON_BN_CLICKED(IDC_BUTTON_OPEN_VIDEO, &CViewerDlg::OnBnClickedButtonOpenVideo)
	ON_BN_CLICKED(IDC_BUTTON_WEBCAM, &CViewerDlg::OnBnClickedButtonWebcam)
END_MESSAGE_MAP()


// CViewerDlg 메시지 처리기

BOOL CViewerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO: 여기에 추가 초기화 작업을 추가합니다.
// -----------------------------------------
// 실행파일(exe)이 저장된 디렉토리 경로 얻기...
// -----------------------------------------
	wchar_t path[MAX_PATH] = { 0 };
	GetModuleFileName(NULL, path, MAX_PATH); // "...\Exe\Evaluation.exe" 경로
	CString msg(path);
	int n = msg.ReverseFind('\\'); // ...\Exe\ 까지 위치, 0,1,...n
	m_strHome = msg.Left(n + 1); // ...\Exe\ 까지 문자열복사, 좌측에서 (n+1)개 복사
	// -----------------------------------------
	// Initialize Darknet YOLOv3 detector & load model files
	// -----------------------------------------
	CString str_cfg = m_strHome + L"detector\\yolov4-patient.cfg";
	CString str_wgt = m_strHome + L"detector\\yolov4-patient_last.weights";
	int gpu_id = 0;
	// YOLOv4 라이브러리 객체 생성
	m_pDetector = new Detector(std::string(CT2CA(str_cfg)),
		std::string(CT2CA(str_wgt)));
	// 클래스 이름 목록 파일 읽기
	LoadClassName(m_strHome + L"detector\\obj-patient.names");

	// 이 대화 상자의 아이콘을 설정합니다.  응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	// TODO: 여기에 추가 초기화 작업을 추가합니다.

	// ***********************************************
	// 영상파일 목록 - List Control 초기화
	LV_COLUMN lvColumn;
	wchar_t* list[3] = { _T("No"), _T("Files"), _T("Others") };
	int nWidth[3] = { 80, 180, 100 };
	for (int i = 0; i < 3; i++)
	{
		lvColumn.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
		lvColumn.fmt = LVCFMT_LEFT; // CENTER;
		if (i == 3) lvColumn.fmt = LVCFMT_RIGHT; // 인식결과 에러판정
		lvColumn.pszText = list[i];
		lvColumn.iSubItem = i;
		lvColumn.cx = nWidth[i];
		m_list_image.InsertColumn(i, &lvColumn);
	}
	m_list_image.SetExtendedStyle(m_list_image.GetExtendedStyle() |
		LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	// ***********************************************
	
	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

BOOL CViewerDlg::LoadClassName(const CString strFilePath)
{
	m_vecClassName.clear();
	wchar_t buffer[125];
	// coco.names -> UTF-8 decoding
	FILE* stream = _wfopen(strFilePath, L"rt+,ccs=UTF-8");
	if (stream == NULL) {
		AfxMessageBox(L"Can't open " + strFilePath);
		return false;
	}
	// 텍스트를 한 줄씩 읽어서 vector에 저장한다.
	// 한 줄 읽으면 끝에 dummy 문자추가 된 것 제거해야 한다.
	while (fgetws(buffer, 125, stream) != NULL) {
		CString strClass(buffer);
		CString strOnlyChar = strClass.Left(strClass.GetLength() - 1);
		m_vecClassName.push_back(strOnlyChar);
	}
	fclose(stream);
	return TRUE;
}

// ----------------------------------------------------------
// Darknet YOLO library call
// ----------------------------------------------------------
std::vector<bbox_t> CViewerDlg::DETECTOR(cv::Mat cv_image)
{
	std::vector<bbox_t> obj = m_pDetector->detect(cv_image, 0.35); //(cv_image, 0.45) 여기를 수정하면 임계치값(threshhold) 수정
	return obj;
}

float bboxOverlap(const cv::Rect& box1, const cv::Rect& box2)
{
	int x1 = std::max(box1.x, box2.x);
	int y1 = std::max(box1.y, box2.y);
	int x2 = std::min(box1.x + box1.width, box2.x + box2.width);
	int y2 = std::min(box1.y + box1.height, box2.y + box2.height);
	int w = std::max(0, x2 - x1);
	int h = std::max(0, y2 - y1);
	float overlap = (float)(w * h) / (box1.width * box1.height + box2.width * box2.height - w * h);
	return overlap;
}


void CViewerDlg::DrawObjectInformation(std::vector<bbox_t>& objects, cv::Mat& image)
{
	const float overlapThreshold = 0.3f; // IOU 임계값

	std::vector<bool> kept(objects.size(), true);

	for (int i = 0; i < objects.size(); i++)
	{
		if (!kept[i]) continue; // 이미 다른 박스와 합쳐진 객체에 대해서는 skip

		int linethickness = 3;
		cv::Scalar boxcolor = (objects[i].obj_id == 0 || objects[i].obj_id == 1) ? cv::Scalar(0, 0, 255) : (objects[i].obj_id == 2 ? cv::Scalar(0, 165, 255) : (objects[i].obj_id == 4 || objects[i].obj_id == 5 ? cv::Scalar(255, 0, 0) : cv::Scalar(0, 255, 0)));
		cv::Rect box1(cv::Point(objects[i].x, objects[i].y), cv::Point(objects[i].x + objects[i].w, objects[i].y + objects[i].h));

		// print class name
		CString strName = m_vecClassName[objects[i].obj_id];
		std::string stringText = std::string(CT2CA(strName.operator LPCWSTR()));
		int fontface = 0;
		double fontscale = 0.7;
		int fontthickness = 2;
		int baseline = 0;
		cv::Size text = cv::getTextSize(stringText, fontface, fontscale, fontthickness, &baseline);
		
		cv::rectangle(image, cv::Point(objects[i].x, objects[i].y), cv::Point(objects[i].x + objects[i].w, objects[i].y + objects[i].h), (objects[i].obj_id == 0 || objects[i].obj_id == 1) ? cv::Scalar(0, 0, 255) : (objects[i].obj_id == 2 ? cv::Scalar(0, 165, 255) : (objects[i].obj_id == 4 || objects[i].obj_id == 5 ? cv::Scalar(255, 0, 0) : cv::Scalar(0, 255, 0))), linethickness);
		cv::rectangle(image, cv::Rect(box1.x, box1.y - text.height, text.width, text.height), objects[i].obj_id == 0 || objects[i].obj_id == 1 ? cv::Scalar(0, 0, 255) : (objects[i].obj_id == 2 ? cv::Scalar(0, 165, 255) : (objects[i].obj_id == 3 ? cv::Scalar(0, 255, 0) : cv::Scalar(255, 0, 0))), CV_FILLED); //오브젝트 아이디 박스 생상
		cv::putText(image, stringText, cv::Point(box1.x, box1.y), fontface, fontscale, cv::Scalar(255, 255, 255), fontthickness, cv::LINE_AA);

		for (int j = i + 1; j < objects.size(); j++)
		{
			//if (!kept[j]) continue; // 이미 다른 박스와 합쳐진 객체에 대해서는 skip

			cv::Rect box2(cv::Point(objects[j].x, objects[j].y), cv::Point(objects[j].x + objects[j].w, objects[j].y + objects[j].h));

			float iou = bboxOverlap(box1, box2);
			if (iou > overlapThreshold)
			{
				// 같은 객체에 속하는 박스를 합쳐줌
				kept[j] = false;
				box1 = box1 | box2;
			}
		}
	}
}

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다.  문서/뷰 모델을 사용하는 MFC 애플리케이션의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void CViewerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.
		
		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR CViewerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

// *****************************************************************
// Get Video file name
// *****************************************************************
CString CViewerDlg::GetVideoFileName()
{
	CFileDialog dlg(TRUE, _T("All(*.*)"), _T("*.*"), OFN_HIDEREADONLY |OFN_OVERWRITEPROMPT,_T("All(*.*)|*.*|(*.mp4)|*.mp4|(*.mov)|*.mov|(*.avi)|*.avi|"));
	if (dlg.DoModal() == IDOK)
	{
		CString strVideoFilePathName = dlg.GetPathName(); // 경로명+파일이름+확장자
		CString strFileName = dlg.GetFileName(); // 파일이름 + 확장자
		m_str_message.Format(L"%s ", strFileName);
		UpdateData(false); // variable -> control
		return strVideoFilePathName;
	}
}

void CViewerDlg::DisplayVideoStream(CString strVideoFileName)
{
	// Convert CString to std::string
	CT2CA convertedString(strVideoFileName);
	std::string s = std::string(convertedString);
	Mat frame; // 비디오 이미지 1장을 OpenCV 포맷으로 저장하는 버퍼
	VideoCapture cap(s); // 비디오 파일을 오픈한다.
	if (cap.isOpened() == false) {
		AfxMessageBox(strVideoFileName + L"\nCannot open the video file");
		return;
	}
	// get the frames properties of the video
	int w = cap.get(cv::CAP_PROP_FRAME_WIDTH); // video image width
	int h = cap.get(cv::CAP_PROP_FRAME_HEIGHT); // video image depth
	double fps = cap.get(cv::CAP_PROP_FPS); // video image frame per second
	int total_frames = cap.get(cv::CAP_PROP_FRAME_COUNT); // Total frame count
	int count_frames = 0;
	bool is_captured = false;
	CString msg_head = m_str_message; // file information(file_name,w,h)

	while (true)
	{
		bool bSuccess = cap.read(frame); // read a frame from video
		if (bSuccess == false) { // Breaking at the end of the video
			break;
		}

		// -------------------------------------------
		// YOLOv4 객체 탐지 함수 호출
		std::vector<bbox_t> objects = DETECTOR(frame);
		DrawObjectInformation(objects, frame);

		// Iterate through detected objects
		for (auto obj : objects) 
		{
			// Check if object ID is 0 or 1
			if (obj.obj_id == 0 || obj.obj_id == 1) 
			{
				// Save the frame as an image
				
				if (!is_captured) 
				{
					// 현재 시간 가져오기
					std::time_t now = std::time(nullptr);
					// 시간 형식 설정
					std::tm time_struct;
					localtime_s(&time_struct, &now);
					char buffer[80];
					std::strftime(buffer, 80, "%Y-%m-%d %H-%M-%S", &time_struct);

					// 파일 경로 설정
					std::string save_path ="C:/visualstudia/Test Viewer - 중복제거 - 복사본/darknet-master-with-GPU/Vision/Exe/사고접수/[00 횡단보도]" + std::string(buffer) + ".jpg";
					// 이미지 저장
					imwrite(save_path, frame);
					std::cout << "Captured frame " << count_frames << std::endl;
					is_captured = true;
				}
			}
		}
		// -------------------------------------------

		::imshow("Video Display", frame); // 화면에 비디오 이미지 출력
		frame.release();
		CString msg;
		msg.Format(L"\n(w=%d h=%d %.2f fps) frame cnt= %d:(%d)",
			w, h, fps, ++count_frames, total_frames);
		m_str_message = msg_head + msg;
		UpdateData(false); // variable -> control
		if (waitKey(10) == 27) { // {Esc}키를 치면 종료
			break;
		}
	}
	cap.release();
}

// **************************************************************
// Load Image File (함수의 원형이 헤드파일에 정의되어 있음)
// **************************************************************
cv::Mat CViewerDlg::LoadImageFile()
{
	cv::Mat mat_mage;
	CFileDialog dlg(TRUE, _T("jpg(*.*)"), _T("*.jpg"), OFN_HIDEREADONLY |
		OFN_OVERWRITEPROMPT,
		_T("All(*.*)|*.*|(*.bmp)|*.bmp|(*.jpg)|*.jpg|"));
	if (dlg.DoModal() == IDOK) {
		CString strImagePath = dlg.GetPathName(); // 경로명+파일이름+확장자
		CString strFileName = dlg.GetFileName(); // 파일이름 + 확장자
		CT2CA pszString(strImagePath);
		std::string strPath(pszString);
		mat_mage = cv::imread(strPath); // opencv image read
	}
	return mat_mage;
}

void CViewerDlg::OnBnClickedButtonLoadFileList()
{
	// *************************************************************
// Open multiple files
// *************************************************************
	CFileDialog dlg(TRUE, _T("jpg(*.jpg)"), _T("*.jpg"),OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_EXPLORER | OFN_ALLOWMULTISELECT, _T("Image Files (*.jpg,*.bmp,*.raw )|*.jpg;*.bmp;*.raw|Image Files(*.jpg) | *.jpg | Image Files(*.jpeg) | *.jpeg | Image Files(*.bmp) | *.bmp | Image Files(*.raw) | *.raw | Image Files(*.RAW) | *.RAW | "));
			wchar_t* pszFile = new wchar_t[65535 * 20];
	ZeroMemory(pszFile, 65535 * 20);
	dlg.m_ofn.lpstrFile = pszFile;
	dlg.m_ofn.nMaxFile = 65535 * 20;
	dlg.m_ofn.nFileOffset = 0;
	CString FileTitle, FileName;
	CString strCnt;
	LV_ITEM lvItem;
	int index = 0;
	// *************************************************************
	// Open dialog for multiple file selection
	// *************************************************************
	if (dlg.DoModal() == IDOK)
	{
		POSITION pos = dlg.GetStartPosition();
		m_list_image.DeleteAllItems();
		while (pos != NULL)
		{
			// **************************************
			// Read current file information
			// **************************************
			CString strPathName = dlg.GetNextPathName(pos);
			FileName = strPathName.Mid(strPathName.ReverseFind('\\') + 1);
			AfxExtractSubString(FileTitle, FileName, 0, '.');
			m_Gallery[index].m_FileName = FileTitle; // 읽은 파일이름
			m_Gallery[index].m_FileExt =
				FileName.Mid(FileName.ReverseFind('.') + 1); // "확장자" 받기
			m_FilePath = strPathName.Left(strPathName.ReverseFind('\\') + 1);
			// "파일경로" 받기
			strCnt.Format(_T("%04d"), index + 1);
			lvItem.mask = LVIF_TEXT;
			lvItem.iItem = index;
			lvItem.iSubItem = 0;
			int length = 1024;
			LPWSTR pwsz = strCnt.GetBuffer(length);
			lvItem.pszText = pwsz;
			m_list_image.InsertItem(&lvItem);
			m_list_image.SetItemText(index, 1,
				m_Gallery[index].m_FileName);
			index++;
		}
		// *************************************************
		// (추가) 읽은 파일 목록에서 첫 번째 이미지 파일을 화면에 출력한다
		// *************************************************
		CString strFilePath = m_FilePath + m_Gallery[0].m_FileName + L"." +m_Gallery[0].m_FileExt;
		CT2CA pszString(strFilePath);
		std::string strPath(pszString);
		cv::Mat image = cv::imread(strPath); // opencv image read
		imshow("Image Display", image); // 화면에 이미지 출력
		m_str_message.Format(L"image w=(%d), H=(%d) BytesPerPixel= %d bytes", image.cols, image.rows, image.channels());
			UpdateData(false); // 변수 값을 다이얼로그 컨트롤로 복사
		image.release(); // cv::Mat 형식 이미지 데이터 메모리 해제

	}
}



void CViewerDlg::OnLvnItemchangedListImage(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	if (pNMLV->uNewState == 0) {
		*pResult = 0;
		return;
	}
	// 리스트 컨트롤에서 선택한 파일의 위치를 알아낸다.
	POSITION pos = m_list_image.GetFirstSelectedItemPosition();
	int nIndex = m_list_image.GetNextSelectedItem(pos);
	// 선택된 항목을 파란색 바로 칠하기 ...
	ListView_SetItemState(m_list_image.GetSafeHwnd(), nIndex, LVIS_FOCUSED |
		LVIS_SELECTED, 0x000F);
	UpdateData(true);
	// *************************************************************
	// 리스트 컨트롤에서 선택된 영상의 처리
	// *************************************************************
	CString strFilePath = m_FilePath + m_Gallery[nIndex].m_FileName + L"." +m_Gallery[nIndex].m_FileExt;
	CT2CA pszString(strFilePath);
	std::string strPath(pszString);
	cv::Mat cv_image = cv::imread(strPath); // opencv image read

	// *************************************************************
	// CHECK PROCESSING TIME (START)
	// *************************************************************
	clock_t start = clock();
	std::vector<bbox_t> objects = DETECTOR(cv_image);
	DrawObjectInformation(objects, cv_image);
	// *************************************************************
	// CHECK PROCESSING TIME (START)
	// *************************************************************
	clock_t finish = clock();
	float du_get = (float)(finish - start) / CLOCKS_PER_SEC;
	CString strMsg;
	strMsg.Format(_T("%.2f sec"), du_get);
	m_list_image.SetItemText(nIndex, 2, strMsg);
	// *************************************************************

	imshow("Image Display", cv_image); // opencv image 화면에 이미지 출력
	m_str_message.Format(L"image w=(%d), H=(%d) BytesPerPixel= %d bytes", cv_image.cols, cv_image.rows, cv_image.channels());
	UpdateData(false); // 변수 값을 다이얼로그 컨트롤로 복사
	*pResult = 0;
}



// *****************************************************************
// Display Video file
// *****************************************************************
void CViewerDlg::DisplayWebcamImage()
{
	Mat frame; // 비디오 이미지 1장을 OpenCV 포맷으로 저장하는 버퍼
	VideoCapture cap(0); // 웹캠을 초기화한다. 내장 웹캠(0), 외장 웹캠(1)
	if (cap.isOpened() == false) {
		AfxMessageBox(L"Can not open Webcam!!");
		return;
	}
	cap.set(cv::CAP_PROP_FRAME_WIDTH, 800); // 가로 크기 설정(기본:640)
	cap.set(cv::CAP_PROP_FRAME_HEIGHT, 600); // 세로 크기 설정(기본:480)
	// get the frames properties of the video
	int w = cap.get(cv::CAP_PROP_FRAME_WIDTH); // Webcam image width
	int h = cap.get(cv::CAP_PROP_FRAME_HEIGHT); // Webcam image depth
	int count_frames = 0;
	CString msg_head = m_str_message; // file information(file_name,w,h)
	while (true)
	{
		bool bSuccess = cap.read(frame); // read a frame from Webcam
		if (bSuccess == false) { // Breaking for read error!!
			break;
		}

		imshow("Webcam Display", frame); // 화면에 이미지 출력
		frame.release();
		m_str_message.Format(L"(w=%d h=%d) frame cnt= %d", w, h, ++count_frames);
		UpdateData(false); // variable -> control

		if (waitKey(10) == 27) { // {Esc}키를 치면 종료
			break;
		}
	}
	cap.release();
}

void CViewerDlg::OnBnClickedButtonClose()
{
	CDialog::OnOK();
}


void CViewerDlg::OnBnClickedButtonOpenVideo()
{
	CString strVideoFileName = GetVideoFileName();
	DisplayVideoStream(strVideoFileName);
}



void CViewerDlg::OnBnClickedButtonWebcam()
{
	DisplayWebcamImage();
}
