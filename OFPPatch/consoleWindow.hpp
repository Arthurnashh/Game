#ifndef _CONSOLE_WINDOW_HPP
#define _CONSOLE_WINDOW_HPP

#include <Es/Threads/pocritical.hpp>

void LoadAppString(HINSTANCE hInstance, UINT uID, LPSTR lpBuffer, int nBufferMax);

class ConsoleWindow
{
  struct ProgressBarInfo
  {
    __int64 totalVal;   //how many correspond to 100%
    __int64 currentVal; //how many were processed yet
    ProgressBarInfo() { totalVal=currentVal=0i64; }
  };
  ProgressBarInfo progressBarInfo;
  HINSTANCE _hInstance;
	HWND _hwndDlg;
	HWND consoleEditHWnd;
	HWND progressTextHWnd;
  HWND percentageHWnd;
  HWND progressBarHWnd;
	HWND okButton;

  /// Critical section to lock _count. It can be used for general object locking..
  mutable PoCriticalSection lock;
  bool doWaitForClosing;

	public:
	ConsoleWindow();

	void Create(HINSTANCE hInstance, bool show=true);
	virtual ~ConsoleWindow();

  inline void Enter() const { lock.enter(); }
  inline void Leave() const { lock.leave(); }
  HWND GetHWnd() const {
    Enter();
    HWND hwndDlg = _hwndDlg;
    Leave();
    return hwndDlg;
  }
	HINSTANCE GetHInstance() const {return _hInstance;}

	void SetTitle(UINT format, ...);
	//void SetTitle(const char *format, ...);
	void SetProgress(UINT format, ...);
	void SetProgress(const char *format, ...);
  void SetProgressBar(int processedSize);
  void InitProgressBar(__int64 total);
  void ShowWindow(bool show) {::ShowWindow(_hwndDlg, show ? SW_SHOW : SW_HIDE); }

	void PrintF(UINT format, ...);
  void DebugF(const char *format, ...);
  void SetWaitForClosing();  //this should be used instead of WaitForClosing()
  bool ShouldWaitForClosing();
	void WaitForClosing(bool enableOK=true);
	void ProcessMessages();
	void Close();
	void Destroy();
	bool Closed() const {return _hwndDlg==NULL;}
	void EnableOK(bool enable=true);

	virtual int DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	static int CALLBACK DlgProcCB(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
};


#endif