#pragma once
#ifndef __HDLCUR_INCLUDED__
#define __HDLCUR_INCLUDED__

#define SRT_RECORD            0xF0000000
#define SRT_PLAY              0xF0000001
#define SRT_ESCAPE            0xF0000002
#define SRT_RECORD_TRUNC      0xF0000003 // recorded but truncated...
#define SRT_MODE_31           0xF0000004 // recognize numbers 1-31 (DE)
#define SRT_MODE_FAMILY       0xF0000005 // recognize first & last names (DE)
#define SRT_MODE_NORMAL       0xF0000006 // return to normal mode (DE)
#define SRT_RECORD_SPEEREO    0xF0000007 // record in Speereo mode, 2400 bit/sec
#define SRT_RECORD_SPEEREO14  0xF0000008 // record in Speereo mode, 1400 bit/sec

typedef enum _SRT_Messages{
    WM_SRT_MAKEHYPO=WM_USER+2,
    WM_SRT_ACCEPTHYPO,
    WM_SRT_NOTIFYRECORD,
    WM_SRT_NOTIFYACTIVATE,
    WM_SRT_CHECKKEY,
    WM_SRT_NOTIFYHOTKEYCHANGE,   // wParam=hotkey
    WM_STR_REGISTER_APP,         // Регистрация программы
    WM_STR_UNREGISTER_APP,       // Освобождение программы
    WM_SRT_NOTIFY_PROGRAM_CHANGE // change program name, wParam=prg name
}SRT_Messages;

typedef enum _HDLCUT_ERROR{
    HDLCUT_NO_ERROR,
    HDLCUT_E_APPRUN,
    HDLCUT_E_MAPPING
} HDLCUT_ERROR;

typedef struct _HypoInfo{
    TCHAR* m_szPhrase;
    DWORD  m_dwId;
}HypoInfo;


// service application calls
#ifdef __cplusplus
extern "C" {
#endif

extern __declspec(dllexport)  DWORD GetCurrentRecordMode();
extern __declspec(dllexport)  void AddPhrase(LPCTSTR pszText, DWORD id);
extern __declspec(dllexport)  void SetMode(DWORD mode = SRT_MODE_NORMAL);
extern __declspec(dllexport)  size_t GetPhraseWeight(int id);
extern __declspec(dllexport)  void ClearPhrases(void);
extern __declspec(dllexport) BOOL FindRec(void);
extern __declspec(dllexport) void SetPlay(const void* lpBuff, DWORD dwSize);
extern __declspec(dllexport) void SetRecord();
extern __declspec(dllexport) bool GetRecord(void*& buff); // return false if was truncated
extern __declspec(dllexport) DWORD GetRecordSize(void*& buff, bool& isFit); // isFit=false if truncated
extern __declspec(dllexport) void NotifyHotkeyChange(BYTE key);

// Функции, необходимые для многопользовательской работы
extern __declspec(dllexport) UINT AddRegisterApplication(HWND pMainWnd);
extern __declspec(dllexport) bool SetPrgOwner(UINT pCode);
extern __declspec(dllexport) bool UnregisterApplication(UINT pCode);

extern __declspec(dllexport)  DWORD GetMode(void);

extern __declspec(dllexport) int GenStartKey(int key1, int key2);

#ifdef __cplusplus
}
#endif

//=====================================

#endif // __HDLCUR_INCLUDED__
