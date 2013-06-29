#ifndef _AUTOCOMPLETE_H_
#define _AUTOCOMPLETE_H_


//// Defines

#define STRID_HOTKEYS                    301
#define STRID_COMPLETEWITHLIST           302
#define STRID_COMPLETENEXT               303
#define STRID_COMPLETEPREV               304
#define STRID_AUTOLIST                   305
#define STRID_AFTERCHAR_PRE              306
#define STRID_AFTERCHAR_POST             307
#define STRID_DOCUMENT                   308
#define STRID_ADDDOCUMENTWORDS           309
#define STRID_COMPLETENONSYNTAXDOCUMENT  310
#define STRID_SAVETYPEDCASE              311
#define STRID_MAXDOCUMENT                312
#define STRID_CHARS                      313
#define STRID_ADDHIGHLIGHTWORDS          314
#define STRID_RIGHTDELIMITERS            315
#define STRID_SYNTAXDELIMITERS           316

#define DLLA_AUTOCOMPLETE_ADDWINDOW 50
#define DLLA_AUTOCOMPLETE_DELWINDOW 51

#define SIZE_MINX      30
#define SIZE_MINY      30

//CreateAutoCompleteWindow flags
#define CAW_COMPLETEONE    0x1
#define CAW_COMPLETEEXACT  0x2
#define CAW_COMPLETEWINDOW 0x4
#define CAW_AUTOLIST       0x8

//CreateAutoCompleteWindow return value
#define CAWE_SUCCESS        0
#define CAWE_NOEDITWINDOW   1
#define CAWE_DOCUMENTLIMIT  2
#define CAWE_GETTITLEPART   3
#define CAWE_AUTOLIST       4
#define CAWE_GETBLOCK       5
#define CAWE_OTHER          6

//BLOCKINFO structure types
#define BIT_BLOCK          0x1
#define BIT_DOCWORD        0x2
#define BIT_NOSYNTAXFILE   0x4
#define BIT_HIGHLIGHT      0x8

#define AUTOCOMPLETEA   "AutoComplete"
#define AUTOCOMPLETEW  L"AutoComplete"
#define IDC_LIST        1001

#ifndef WM_MOUSEWHEEL
  #define WM_MOUSEWHEEL 0x020A
#endif


//// Structures

typedef struct _BLOCKINFO {
  struct _BLOCKINFO *next;
  struct _BLOCKINFO *prev;
  DWORD dwStructType;
  wchar_t wchFirstLowerChar;
  wchar_t *wpTitle;
  int nTitleLen;
  BOOL bExactTitle;

  struct _BLOCKINFO *master;
  INT_PTR *firstHandle;
  INT_PTR *lastHandle;
  wchar_t *wpBlock;
  int nBlockLen;
  int nLinesInBlock;
  HSTACK hHotSpotStack;
  INT_PTR nHotSpotBlockBegin;
} BLOCKINFO;

typedef struct _BLOCKINFOHANDLE {
  struct _BLOCKINFOHANDLE *next;
  struct _BLOCKINFOHANDLE *prev;
  BLOCKINFO *lpBlockInfo;
} BLOCKINFOHANDLE;

typedef struct _HOTSPOT {
  struct _HOTSPOT *next;
  struct _HOTSPOT *prev;
  int nHotSpotInitPos;
  int nHotSpotInitLen;
  int nHotSpotPos;
  int nHotSpotLen;
} HOTSPOT;

typedef struct _DOCWORDINFO {
  struct _DOCWORDINFO *next;
  struct _DOCWORDINFO *prev;
  DWORD dwStructType;
  wchar_t wchFirstLowerChar;
  wchar_t *wpDocWord;
  int nDocWordLen;
} DOCWORDINFO;

typedef struct {
  DOCWORDINFO *first;
  DOCWORDINFO *last;
  INT_PTR lpSorted[FIRST_NONLATIN + 1];
} HDOCWORDS;


//// Prototypes

BOOL CALLBACK AutoCompleteSetupDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK AutoCompleteParentMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *lResult);
BOOL CALLBACK AutoCompleteEditMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *lResult);
LRESULT CALLBACK GetMsgProc(int code, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK CompleteWithListProc(void *lpParameter, LPARAM lParam, DWORD dwSupport);
BOOL CALLBACK CompleteNextProc(void *lpParameter, LPARAM lParam, DWORD dwSupport);
BOOL CALLBACK CompletePrevProc(void *lpParameter, LPARAM lParam, DWORD dwSupport);
LRESULT CALLBACK AutoCompleteWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK NewListboxProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

//AutoComplete window
DWORD CreateAutoCompleteWindow(SYNTAXFILE *lpTheme, DWORD dwFlags);
BOOL MoveAutoCompleteWindow();
void CloseAutoCompleteWindow();

//AutoComplete window listbox
void FillListbox(SYNTAXFILE *lpSyntaxFile, HDOCWORDS *hDocWordsStack, const wchar_t *wpTitlePart);
void SetSelListbox(int nIndex);
BLOCKINFO* GetBlockListbox();

//Scheme
int ParseBlock(SYNTAXFILE *lpScheme, HSTACK *hHotSpotStack, const wchar_t *wpInput, int nInputLen, wchar_t *wszOutput, int *nOutputLines);

//Title
TITLEINFO* StackInsertTitle(STACKTITLE *hStack);
void StackFreeTitle(STACKTITLE *hStack);

//Title part
BOOL GetEditTitlePart(STACKDELIM *hDelimiterStack, wchar_t *wszTitle, int nTitleMax, INT_PTR *nMin, INT_PTR *nMax);
void CompleteTitlePart(BLOCKINFO *lpBlockInfo, INT_PTR nMin, INT_PTR nMax);

//Block
BLOCKINFO* StackInsertBlock(STACKBLOCK *hBlockStack);
BLOCKINFO* StackInsertAndSortBlock(STACKBLOCK *hBlockStack, wchar_t *wpTitle, int nTitleLen);
BLOCKINFO* StackGetExactBlock(SYNTAXFILE *lpSyntaxFile, AECHARINDEX *ciCaret, INT_PTR nCaretOffset, INT_PTR *nMin, INT_PTR *nMax);
BLOCKINFO* StackGetBlock(SYNTAXFILE *lpSyntaxFile, HDOCWORDS *hDocWordsStack, const wchar_t *wpTitlePart, int nTitlePartLen, BOOL *bOnlyOne);
void StackFreeBlock(STACKBLOCK *hBlockStack);

//Hot spot
HOTSPOT* StackInsertHotSpot(HSTACK *hStack, int nHotSpotPos);
HOTSPOT* GetHotSpot(BLOCKINFO *lpBlockInfo, HWND hWnd, CHARRANGE64 *cr);
BOOL NextHotSpot(BLOCKINFO *lpBlockInfo, HWND hWnd, BOOL bPrev);
void StackResetHotSpot(BLOCKINFO *lpBlockInfo);
void StackFreeHotSpot(HSTACK *hStack);

//Document words
void StackFillDocWord(SYNTAXFILE *lpSyntaxFile, HDOCWORDS *hDocWordsStack, const wchar_t *wpTitlePart, int nTitlePartLen);
DOCWORDINFO* StackInsertDocWord(HDOCWORDS *hStack, wchar_t *wpWordDoc, int nWordDocLen);
DOCWORDINFO* StackGetDocWord(HDOCWORDS *hStack, const wchar_t *wpDocWord, int nDocWordLen);
void StackFreeDocWord(HDOCWORDS *hStack);

//Options
void ReadAutoCompleteOptions(HANDLE hOptions);
void SaveAutoCompleteOptions(HANDLE hOptions, DWORD dwFlags);
void InitAutoComplete();
void UninitAutoComplete();


//// Global variables

extern SYNTAXFILE *lpSyntaxFileAutoComplete;

#endif
