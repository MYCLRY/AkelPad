#ifndef _HIGHLIGHT_H_
#define _HIGHLIGHT_H_


//// Defines

#define STRID_IGNORE_FONTSTYLE_GROUP 101
#define STRID_NORMAL                 102
#define STRID_BOLD                   103
#define STRID_ITALIC                 104
#define STRID_AUTOMARK_GROUP         105
#define STRID_SYMBOLS                106
#define STRID_WORDS                  107
#define STRID_TEXTCOLOR              108
#define STRID_BGCOLOR                109

#define DLLA_HIGHLIGHT_MARK                2
#define DLLA_HIGHLIGHT_UNMARK              3
#define DLLA_HIGHLIGHT_FINDMARK            4
#define DLLA_HIGHLIGHT_CHECKMARK           11
#define DLLA_HIGHLIGHT_GETMARKSTACK        12
#define DLLA_HIGHLIGHT_ADDWINDOW           50
#define DLLA_HIGHLIGHT_DELWINDOW           51

#define FF_FONTSTYLE 0x1
#define FF_FONTSIZE  0x2
#define FF_FONTFACE  0x4

#define MARKID_AUTOMIN    1000001
#define MARKID_AUTOASSIGN (DWORD)-1
#define MARKID_SELECTION  (DWORD)-2

#define MARKAUTO_NONE    0
#define MARKAUTO_SYMBOLS 1
#define MARKAUTO_WORDS   2

#define MARKMAX_WORD   1024


//// Structures

typedef struct _WORDINFO {
  struct _WORDINFO *next;
  struct _WORDINFO *prev;
  wchar_t *wpWord;
  int nWordLen;
  DWORD dwFlags;
  DWORD dwFontStyle;
  DWORD dwColor1;
  DWORD dwColor2;
} WORDINFO;

typedef struct _QUOTEINFO {
  struct _QUOTEINFO *next;
  struct _QUOTEINFO *prev;
  wchar_t *wpQuoteStart;
  int nQuoteStartLen;
  wchar_t *wpQuoteEnd;
  int nQuoteEndLen;
  wchar_t wchEscape;
  wchar_t *wpQuoteInclude;
  int nQuoteIncludeLen;
  wchar_t *wpQuoteExclude;
  int nQuoteExcludeLen;
  DWORD dwFlags;
  DWORD dwFontStyle;
  DWORD dwColor1;
  DWORD dwColor2;
} QUOTEINFO;

typedef struct _MARKTEXT {
  struct _MARKTEXT *next;
  struct _MARKTEXT *prev;
  AEHMARKTEXT hMarkTextHandle;
  DWORD dwMarkID;
} MARKTEXT;

typedef struct {
  MARKTEXT *first;
  MARKTEXT *last;
} STACKMARKTEXT;

typedef struct _HIGHLIGHTWINDOW {
  struct _HIGHLIGHTWINDOW *next;
  struct _HIGHLIGHTWINDOW *prev;
  FRAMEDATA *lpFrame;
  HWND hWndEdit;
  AEHDOC hDocEdit;
  BOOL bOldFont;
  DWORD dwFontFlags;
  LOGFONTW lfFontNew;
  LOGFONTW lfFontOld;
  BOOL bOldColors;
  AECOLORS aecNew;
  AECOLORS aecOld;
  BOOL bOldBkImage;
  wchar_t wszBkImageFileOld[MAX_PATH];
  wchar_t wszBkImageFileNew[MAX_PATH];
  int nBkImageAlphaOld;
  int nBkImageAlphaNew;
  STACKMARKTEXT hMarkTextsStack;

  //User window
  MANUALSET *lpUser;
} HIGHLIGHTWINDOW;

typedef struct {
  HIGHLIGHTWINDOW *first;
  HIGHLIGHTWINDOW *last;
} STACKHIGHLIGHTWINDOW;


//// Prototypes

BOOL CALLBACK HighLightSetupDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK HighLightParentMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *lResult);
BOOL CALLBACK HighLightEditMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *lResult);
void PaintRichEditA(SYNTAXFILE *lpSyntaxFile, HWND hWnd, RECT *rcUpdateRect);
void PaintRichEditW(SYNTAXFILE *lpSyntaxFile, HWND hWnd, RECT *rcUpdateRect);
WORDINFO* StackInsertWord(STACKWORD *hStack, int nWordLen);
WORDINFO* StackGetWord(STACKWORD *hStack, wchar_t *wpWord, int nWordLen);
void StackFreeWord(STACKWORD *hStack);
QUOTEINFO* StackInsertQuote(STACKQUOTE *hStack, int nQuoteStartLen);
void StackFreeQuote(STACKQUOTE *hStack);
HIGHLIGHTWINDOW* StackInsertHighLightWindow(STACKHIGHLIGHTWINDOW *hStack);
HIGHLIGHTWINDOW* StackGetHighLightWindow(STACKHIGHLIGHTWINDOW *hStack, HWND hWndMaster, AEHDOC hDocMaster);
void StackDeleteHighLightWindow(STACKHIGHLIGHTWINDOW *hStack, HIGHLIGHTWINDOW *lpHighlightWindow);
void StackFreeHighLightWindow(STACKHIGHLIGHTWINDOW *hStack);
MARKTEXT* StackInsertMark(HIGHLIGHTWINDOW *lpHighlightWindow);
MARKTEXT* StackGetMarkByColorID(HIGHLIGHTWINDOW *lpHighlightWindow, DWORD dwMarkID, DWORD dwColorText, DWORD dwColorBk);
MARKTEXT* StackGetMarkByText(HIGHLIGHTWINDOW *lpHighlightWindow, const wchar_t *wpText, int nTextLen);
DWORD StackAssignMarkID(HIGHLIGHTWINDOW *lpHighlightWindow);
BOOL StackIsAnyMark(HIGHLIGHTWINDOW *lpHighlightWindow);
void StackDeleteMark(HIGHLIGHTWINDOW *lpHighlightWindow, MARKTEXT *lpMarkText);
BOOL StackFreeMark(HIGHLIGHTWINDOW *lpHighlightWindow);
BOOL MarkSelection(HIGHLIGHTWINDOW *lpHighlightWindow, const wchar_t *wpText, int nTextLen, DWORD dwColorText, DWORD dwColorBk, BOOL bMatchCase, DWORD dwFontStyle, DWORD dwMarkID);
BOOL UnmarkSelection(HIGHLIGHTWINDOW *lpHighlightWindow, DWORD dwMarkID, DWORD dwColorText, DWORD dwColorBk);
BOOL FindMark(HIGHLIGHTWINDOW *lpHighlightWindow, DWORD dwMarkID, DWORD dwColorText, DWORD dwColorBk, BOOL bFindUp);
void CreateEditTheme(SYNTAXFILE *lpSyntaxFile, HWND hWnd);
void GetFontAndColors(SYNTAXFILE *lpSyntaxFile, HIGHLIGHTWINDOW *lpHighlightWindow);
void RestoreFontAndColors(HIGHLIGHTWINDOW *lpHighlightWindow);
DWORD GetColorsToRestore(HIGHLIGHTWINDOW *lpHighlightWindow, AECOLORS *aecHighlight);
void UnassignTheme(HWND hWnd);
void UpdateHighLight(HWND hWnd);
void UpdateHighLightAll();
COLORREF GetColorValueFromStrA(char *pColor);
COLORREF GetColorValueFromStrW(wchar_t *wpColor);
char* GetColorStrFromValueA(COLORREF crColor, char *szColor);
wchar_t* GetColorStrFromValueW(COLORREF crColor, wchar_t *wszColor);
BOOL SetFrameInfo(FRAMEDATA *lpFrame, int nType, UINT_PTR dwData);
void ReadHighLightOptions(HANDLE hOptions);
void SaveHighLightOptions(HANDLE hOptions, DWORD dwFlags);
void InitHighLight();
void UninitHighLight();


//// Global variables

extern STACKHIGHLIGHTWINDOW hHighLightWindowsStack;
extern DWORD dwIgnoreFontStyle;
extern DWORD dwAutoMarkFlags;
extern DWORD dwAutoMarkFontStyle;
extern DWORD dwAutoMarkTextColor;
extern DWORD dwAutoMarkBkColor;
extern BOOL bUpdateTheme;

#endif
