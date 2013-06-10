#include <windows.h>
#include <tchar.h>
#define _RICHEDIT_VER 0x0200
#include <richedit.h>
#include <commctrl.h>
#include <stdio.h>
#include <ctype.h>
#include <wchar.h>
#include "akelpad.h"
#include "edit.h"
#include "codepage.h"
#include "langpack.h"

extern int g_iCmdShow;
extern HWND hSyncWindow;

extern RECT MainWindowRect;

extern BOOL GlobalPrint;

BOOL append_header=TRUE; //whether header was in last translated text
BOOL refresh_data=TRUE;

BOOL unix_newline=FALSE; //Unix newline (0x0A) was detected when opening file
BOOL newline_detected=FALSE;

extern BOOL insertstate;

TCHAR recent_names[RECENTFILES][MAX_PATH+1]; //names of recent files
DWORD recent_positions[RECENTFILES]; //positions in recent files

OPENFILENAME ofn;
TCHAR file_buffer[MAX_PATH+1]; //placeholder for ofn.lpstrFile

unsigned char pcReadBuffer[READBUFFERSIZE];
unsigned char pcTranslateBuffer[TRANSLATEBUFFERSIZE];
BOOL firsttime;

CHOOSEFONT cf;
CHOOSECOLOR cc;
COLORREF crCustColors[16];
LOGFONT logfont;
LOGFONT logfont2;
HFONT hCurrentFont=NULL;

FINDTEXT ft;
DWORD ftflags=FR_DOWN;

HKL *hKl;
int layouts;
BOOL switched=FALSE;
BOOL usesystembckgcolor=TRUE;

char *pStreamInBuffer = NULL;
int iStreamInBufferSize;

PAGESETUPDLG psdPage;
PRINTDLG pd;

struct __replaceto {
    TCHAR *org_query;
    TCHAR *lpstrNewText;
} replaceto;

BOOL terror; //set if a syntax error was detected while translating escape-sequences in search string
TCHAR *org_query; //original search template (entered by user)
BOOL use_escape=FALSE;
BOOL wrap_on=FALSE; //wrap strings
BOOL keep_on=FALSE; //keep left space
BOOL need_autodetect=FALSE; //if charset should be auto-detected
BOOL replace_in_sel=FALSE; //if replacement should be done in selection instead of whole text
BOOL open_in_new_window=TRUE;
BOOL support_asian=FALSE;

LRESULT lang_settings;

void _SetLayout();
void _RestoreLayout();

void SaveDocument(HWND hWndEdit);
int _FindText(HWND hWndEdit);
void _ReplaceText(HWND hWndEdit, BOOL all);
BOOL TranslateBackslashes(TCHAR *newstr,TCHAR *string);
TCHAR TranslateHexadecimalDigit(TCHAR ch);

int TranslateNewlinesToUnix(wchar_t *buffer,int iBufSize);

BOOL AddLastFilePosition(HWND); //adds file to list of recent files
BOOL SaveLastFilePosition(HWND); //saves list of recent files
BOOL RestoreLastFilePosition(HWND); //reads list of recent files

BOOL CALLBACK FindDlgProc(HWND hDlg,UINT msg,WPARAM wParam,LPARAM lParam);
BOOL CALLBACK ReplaceDlgProc(HWND hDlg,UINT msg,WPARAM wParam,LPARAM lParam);
BOOL CALLBACK ConfirmDlgProc(HWND hDlg,UINT msg,WPARAM wParam,LPARAM lParam);
BOOL CALLBACK AboutDlgProc(HWND hDlg,UINT msg,WPARAM wParam,LPARAM lParam);
BOOL CALLBACK GoToLineDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

void InitEditor(HWND hWndEdit) {
    TCHAR pBuf[16];
    HMENU hMenu;
    SendMessage(hWndEdit,EM_SETTEXTMODE,TM_PLAINTEXT|TM_MULTILEVELUNDO|TM_MULTICODEPAGE,0);
    SendMessage(hWndEdit,EM_SETEVENTMASK,0,ENM_SELCHANGE|ENM_CHANGE|ENM_MOUSEEVENTS|ENM_KEYEVENTS);

    ofn.lpstrFile=file_buffer;

    ofn.lpstrFile[0]=_T('\0');
    ofn.lpstrFile[MAX_PATH]=_T('\0');

    ofn.lStructSize       = sizeof (OPENFILENAME);
    ofn.hwndOwner         = GetParent(hWndEdit);
    ofn.hInstance         = NULL;
    ofn.lpstrFilter       = STR_FILE_FILTER;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter    = 0;
    ofn.nFilterIndex      = 0;
    ofn.nMaxFile          = MAX_PATH;
    ofn.lpstrFileTitle    = NULL;
    ofn.nMaxFileTitle     = 0;
    ofn.lpstrInitialDir   = NULL;
    ofn.lpstrTitle        = NULL;
    ofn.nFileOffset       = 0;
    ofn.nFileExtension    = 0;
    ofn.lCustData         = 0;
    ofn.lpfnHook          = NULL;
    ofn.lpTemplateName    = NULL;

    cc.lStructSize=sizeof(CHOOSECOLOR);
    cc.hwndOwner=GetParent(hWndEdit);
    cc.hInstance=0;
    cc.rgbResult=RGB(0xFF,0xFF,0xFF);
    cc.lpCustColors=crCustColors;
    cc.Flags=CC_FULLOPEN|CC_RGBINIT;
    cc.lCustData=0;
    cc.lpfnHook=NULL;
    cc.lpTemplateName=NULL;
    ZeroMemory(crCustColors,16*sizeof(COLORREF));

    ft.lpstrText=NULL;
    org_query=NULL;
    replaceto.lpstrNewText=NULL;
    replaceto.org_query=NULL;

    cf.lpLogFont=&logfont;
    cf.rgbColors=0;
    GetObject(GetStockObject(SYSTEM_FONT),sizeof(LOGFONT),&logfont);
    SetChosenFont(hWndEdit);

    layouts=GetKeyboardLayoutList(0,NULL);
    hKl=(HKL *)HeapAlloc(GetProcessHeap(),0,layouts*sizeof(HKL));
    if(!hKl) {
        MessageBox(NULL,MSG_ERROR_NOT_ENOUGH_MEMORY,NULL,MB_OK|MB_ICONERROR);
        ExitProcess(1);
    }


    GetKeyboardLayoutList(layouts,hKl);

    RestoreLastFilePosition(hWndEdit);

    hMenu=GetMenu(GetParent(hWndEdit));

    ZeroMemory(&psdPage,sizeof(PAGESETUPDLG));
    psdPage.lStructSize=sizeof(PAGESETUPDLG);
    psdPage.Flags=PSD_MARGINS;

    if(GetLocaleInfo(LOCALE_USER_DEFAULT,LOCALE_IMEASURE,pBuf,16)) {
        if(*pBuf==_T('0')) {
            psdPage.rtMargin.left=1000;
            psdPage.rtMargin.right=1000;
            psdPage.rtMargin.top=1000;
            psdPage.rtMargin.bottom=1000;
            psdPage.Flags|=PSD_INHUNDREDTHSOFMILLIMETERS;
        }
        else {
            psdPage.rtMargin.left=500;
            psdPage.rtMargin.right=500;
            psdPage.rtMargin.top=500;
            psdPage.rtMargin.bottom=500;
            psdPage.Flags|=PSD_INTHOUSANDTHSOFINCHES;
        }
    }

    ZeroMemory(&pd,sizeof(PRINTDLG));
    pd.Flags=PD_ALLPAGES|PD_NOPAGENUMS|PD_RETURNDC|PD_USEDEVMODECOPIESANDCOLLATE;
    pd.nCopies=1;

    lang_settings=SendMessage(hWndEdit,EM_GETLANGOPTIONS,(WPARAM)0,(LPARAM)0);
    SendMessage(hWndEdit,EM_SETLANGOPTIONS,(WPARAM)0,(LPARAM)0);
}

int DoFileNew(HWND hWndEdit) {
    int choice;

    AddLastFilePosition(hWndEdit);

    if(GetModify()) {
        choice=MessageBox(GetParent(hWndEdit),MSG_DOCUMENT_CHANGED,APP_SHORT_TITLE,MB_YESNOCANCEL|MB_ICONEXCLAMATION);
        switch(choice) {
        case IDYES:
            if(!DoFileSave(hWndEdit)) break;
        case IDNO:
            unix_newline=FALSE;
            ChangeCodePage(WINDOWS_1251);
            HideCaret(NULL);
            SetWindowText(hWndEdit, _T(""));
            ShowCaret(NULL);
            SetWindowText(GetParent(hWndEdit),APP_SHORT_TITLE);
            ofn.lpstrFile[0]=_T('\0');
            SetModify(FALSE);
            break;
        }
        if(choice!=IDCANCEL) return -1;
        else return 0;
    }
    else {
        unix_newline=FALSE;
        ChangeCodePage(WINDOWS_1251);
        HideCaret(NULL);
        SetWindowText(hWndEdit, _T(""));
        ShowCaret(NULL);
        SetWindowText(GetParent(hWndEdit),APP_SHORT_TITLE);
        ofn.lpstrFile[0]=_T('\0');
        SetModify(FALSE);
        return -1;
    }
}

void DoFileNewWindow(HWND hWndEdit) {
    TCHAR szModuleFileName[MAX_PATH];
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    UNREFERENCED_PARAMETER(hWndEdit);

    ZeroMemory(&si,sizeof(STARTUPINFO));
    si.cb=sizeof(STARTUPINFO);

    if(!GetModuleFileName(GetModuleHandle(NULL),szModuleFileName,MAX_PATH)) return;

    CreateProcess(szModuleFileName,NULL,NULL,NULL,FALSE,0,NULL,NULL,&si,&pi);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

void DoFileOpen(HWND hWndEdit) {
    HWND hWndParent;
    int choice;

    hWndParent=GetParent(hWndEdit);

    ofn.Flags=OFN_CREATEPROMPT|OFN_LONGNAMES|OFN_HIDEREADONLY;
    ofn.lpstrDefExt=NULL;

    if(GetModify()) {
        choice=MessageBox(hWndParent,MSG_DOCUMENT_CHANGED,APP_SHORT_TITLE,MB_YESNOCANCEL|MB_ICONEXCLAMATION);
        if(choice==IDYES) {
            if(!DoFileSave(hWndEdit)) choice=IDCANCEL;
        }
    }
    else choice=IDNO;

    if(choice==IDCANCEL) return;

    AddLastFilePosition(hWndEdit);

    if(GetOpenFileName(&ofn)) {
        need_autodetect=TRUE;
        OpenDocument(hWndEdit,FALSE);
        need_autodetect=FALSE;
    }
}

void DoFileOpenHistory(HWND hWndEdit,int number) {
    HWND hWndParent;
    int choice;
    TCHAR name_buf[MAX_PATH+2];

    hWndParent=GetParent(hWndEdit);

    if(GetModify()) {
        choice=MessageBox(hWndParent,MSG_DOCUMENT_CHANGED,APP_SHORT_TITLE,MB_YESNOCANCEL|MB_ICONEXCLAMATION);
        if(choice==IDYES) {
            if(!DoFileSave(hWndEdit)) choice=IDCANCEL;
        }
    }
    else choice=IDNO;

    if(choice==IDCANCEL) return;

    _tcscpy(name_buf, recent_names[number]);

    AddLastFilePosition(hWndEdit);

    _tcscpy(ofn.lpstrFile,name_buf);

    need_autodetect=TRUE;
    OpenDocument(hWndEdit,FALSE);
    need_autodetect=FALSE;
}

void DoFileReopen(HWND hWndEdit) {
    if(!*ofn.lpstrFile) return;
    else if(!GetModify()) OpenDocument(hWndEdit,FALSE);
    else if(MessageBox(GetParent(hWndEdit),MSG_FILE_WILL_BE_REOPENED,APP_SHORT_TITLE,MB_OKCANCEL|MB_ICONEXCLAMATION)==IDOK) OpenDocument(hWndEdit,FALSE);
}

int DoFileSave(HWND hWndEdit) {
    if(!GetModify()&&ofn.lpstrFile[0]) return 0;
    if(file_buffer[0]==0) return DoFileSaveAs(hWndEdit);
    SaveDocument(hWndEdit);
    return -1;
}

int DoFileSaveAs(HWND hWndEdit) {

    ofn.Flags=OFN_OVERWRITEPROMPT|OFN_LONGNAMES|OFN_HIDEREADONLY;
    ofn.lpstrDefExt=_T("txt");

    if(GetSaveFileName(&ofn)) {
        SaveDocument(hWndEdit);
        return -1;
    }
    return 0;
}

void DoFilePageSetup(HWND hWndEdit) {
    psdPage.hwndOwner=GetParent(hWndEdit);

    PageSetupDlg(&psdPage);
}

void DoFilePrint(HWND hWndEdit) {
    FORMATRANGE fr;
    DOCINFO di;
    int nextch;
    int width,height;

    pd.lStructSize=sizeof(PRINTDLG);
    pd.hwndOwner=GetParent(hWndEdit);
    pd.hDevMode=psdPage.hDevMode;
    pd.hDevNames=psdPage.hDevNames;
    pd.Flags|=PD_NOPAGENUMS|PD_RETURNDC|PD_USEDEVMODECOPIESANDCOLLATE;

    ZeroMemory(&di,sizeof(DOCINFO));

    di.cbSize=sizeof(DOCINFO);
    di.lpszDocName=(*ofn.lpstrFile)?ofn.lpstrFile:STR_DOCNAME;

    if(!PrintDlg(&pd)) return;

    width=GetDeviceCaps(pd.hDC,HORZSIZE)*56; //about 56 twips per millimeter
    height=GetDeviceCaps(pd.hDC,VERTSIZE)*56;

    fr.hdc=pd.hDC;
    fr.hdcTarget=pd.hDC;

    if(psdPage.Flags&PSD_INHUNDREDTHSOFMILLIMETERS) {
        fr.rc.left=psdPage.rtMargin.left*567/1000;
        fr.rc.top=psdPage.rtMargin.top*567/1000;
        fr.rc.right=width-psdPage.rtMargin.right*567/1000;
        fr.rc.bottom=height-psdPage.rtMargin.bottom*567/1000;
    }
    else {
        fr.rc.left=psdPage.rtMargin.left*144/100;
        fr.rc.top=psdPage.rtMargin.top*144/100;
        fr.rc.right=width-psdPage.rtMargin.right*144/100;
        fr.rc.bottom=height-psdPage.rtMargin.bottom*144/100;
    }
    fr.rcPage.left=0;
    fr.rcPage.top=0;
    fr.rcPage.right=width;
    fr.rcPage.bottom=height;

    if(pd.Flags&PD_PRINTTOFILE) di.lpszOutput=_T("FILE:");

    if(StartDoc(pd.hDC,&di)<=0) {
        MessageBox(GetParent(hWndEdit),MSG_ERROR_PRINT,APP_SHORT_TITLE,MB_OK|MB_ICONERROR);
        return;
    }

    if(pd.Flags&PD_SELECTION) {
        SendMessage(hWndEdit,EM_EXGETSEL,0,(LPARAM)&fr.chrg);
    }
    else {
        fr.chrg.cpMin=0;
        fr.chrg.cpMax=-1;
    }

    do {
        nextch=SendMessage(hWndEdit,EM_FORMATRANGE,FALSE,(LPARAM)&fr);
        if(nextch==fr.chrg.cpMin) break;
        fr.chrg.cpMin=nextch;
        StartPage(pd.hDC);
        if(!SendMessage(hWndEdit,EM_DISPLAYBAND,0,(LPARAM)&fr.rc)) {
            EndPage(pd.hDC);
            MessageBox(GetParent(hWndEdit),MSG_ERROR_PRINT,APP_SHORT_TITLE,MB_OK|MB_ICONERROR);
            break;
        }
        EndPage(pd.hDC);
    } while(TRUE);

    EndDoc(pd.hDC);
    DeleteDC(pd.hDC);
}

void DoFileExit(HWND hWndEdit) {
    int choice;
    HWND hWndParent;

    AddLastFilePosition(hWndEdit);

    hWndParent=GetParent(hWndEdit);

    if(GetModify()) {
        choice=MessageBox(hWndParent,MSG_DOCUMENT_CHANGED,APP_SHORT_TITLE,MB_YESNOCANCEL|MB_ICONEXCLAMATION);
        if(choice==IDYES) {
            if(!DoFileSave(hWndEdit)) choice=IDCANCEL;
        }
    }
    else choice=IDNO;
    if(choice!=IDCANCEL) {
        DestroyWindow(hWndParent);
    }
}

void DoEditUndo(HWND hWndEdit) {
    SendMessage(hWndEdit,EM_UNDO,0,0);
}

void DoEditRedo(HWND hWndEdit) {
    SendMessage(hWndEdit,EM_REDO,0,0);
}

void DoEditCut(HWND hWndEdit) {
    SendMessage(hWndEdit,WM_CUT,0,0);
}

void DoEditCopy(HWND hWndEdit) {
    SendMessage(hWndEdit,WM_COPY,0,0);
}

void DoEditPaste(HWND hWndEdit) {
    //Includes workarounds of several MS bugs
    HANDLE hData;
    LPVOID pData;
    UINT format;

    if(!OpenClipboard(NULL)) {
        SendMessage(hWndEdit,WM_PASTE,0,0);
        return;
    }

    format=EnumClipboardFormats(0);
    if(format!=CF_TEXT) {
        CloseClipboard();
        SendMessage(hWndEdit,WM_PASTE,0,0);
        return;
    }

    hData=GetClipboardData(CF_TEXT);
    if(!hData) {
        CloseClipboard();
        SendMessage(hWndEdit,WM_PASTE,0,0);
        return;
    }

    pData=GlobalLock(hData);
    if(!pData) {
        CloseClipboard();
        SendMessage(hWndEdit,WM_PASTE,0,0);
        return;
    }

    SendMessage(hWndEdit,EM_REPLACESEL,TRUE,(LPARAM)pData);

    GlobalUnlock(hData);
    CloseClipboard();
}

void DoEditClear(HWND hWndEdit) {
    SendMessage(hWndEdit,EM_REPLACESEL,TRUE,(LPARAM)_T(""));
}

void DoEditSelectAll(HWND hWndEdit) {
    CHARRANGE chrg;
    chrg.cpMin=0;
    chrg.cpMax=-1;

    SendMessage(hWndEdit,EM_EXSETSEL,0,(LPARAM)&chrg);
}

void DoEditInsertChar(HWND hWndEdit) {
    ShellExecute(GetParent(hWndEdit),_T("open"),_T("charmap.exe"),NULL,NULL,SW_SHOWDEFAULT);
}

void DoEditWrap(HWND hWndEdit) {
    HMENU hMenu;
    NMHDR nmhdr;

    hMenu=GetMenu(GetParent(hWndEdit));
    wrap_on=!wrap_on;
    if(wrap_on) {
        SendMessage(hWndEdit,EM_SETTARGETDEVICE,(WPARAM)NULL,0);
        CheckMenuItem(hMenu,IDM_EDIT_WRAP,MF_CHECKED);
    }
    else {
        SendMessage(hWndEdit,EM_SETTARGETDEVICE,(WPARAM)NULL,1);
        CheckMenuItem(hMenu,IDM_EDIT_WRAP,MF_UNCHECKED);
    }

    nmhdr.hwndFrom=hWndEdit;
    nmhdr.idFrom=ID_EDIT;
    nmhdr.code=EN_SELCHANGE;

    SendMessage(GetParent(hWndEdit),WM_NOTIFY,ID_EDIT,(LPARAM)&nmhdr);
}

void DoEditKeepSpace(HWND hWndEdit) {
    HMENU hMenu;

    hMenu=GetMenu(GetParent(hWndEdit));
    keep_on=!keep_on;
    if(keep_on) {
        CheckMenuItem(hMenu,IDM_EDIT_KEEPSPACE,MF_CHECKED);
    }
    else {
        CheckMenuItem(hMenu,IDM_EDIT_KEEPSPACE,MF_UNCHECKED);
    }
}

void DoSearchFind(HWND hWndEdit) {
    int r;
    do {
        if(DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_FIND),GetParent(hWndEdit),(DLGPROC)FindDlgProc))
            r=_FindText(hWndEdit);
        else return;
    } while(r==-1);
}

void DoSearchFindNext(HWND hWndEdit) {
    int r;
    if(ft.lpstrText) {
        r=_FindText(hWndEdit);
        if(r==-1) DoSearchFind(hWndEdit);
    }
    else DoSearchFind(hWndEdit);
}

void DoSearchFindOther(HWND hWndEdit) {
    if(ft.lpstrText) {
        ftflags^=FR_DOWN;
        _FindText(hWndEdit);
        ftflags^=FR_DOWN;
    }
    else DoSearchFind(hWndEdit);
}

void DoSearchReplace(HWND hWndEdit) {
    int result;

    result=DialogBoxParam(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_REPLACE),GetParent(hWndEdit),(DLGPROC)ReplaceDlgProc,(LPARAM)hWndEdit);
    if(result) _ReplaceText(hWndEdit,(result==-1)?FALSE:TRUE);
}

void DoSearchGoToLine(HWND hWndEdit) {
    CHARRANGE chrg;
    int result;
    int line,column;
    int chars;

    SendMessage(hWndEdit,EM_EXGETSEL,0,(LPARAM)&chrg);
    line=SendMessage(hWndEdit,EM_EXLINEFROMCHAR,0,chrg.cpMin)+1;

    column=SendMessage(hWndEdit,EM_LINEINDEX,line-1,0); //column=index of first character in that string
    column=chrg.cpMin-column+1;

    result=DialogBoxParam(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_GOTOLINE),GetParent(hWndEdit),(DLGPROC)GoToLineDlgProc,(LPARAM)line);

    chrg.cpMin=SendMessage(hWndEdit,EM_LINEINDEX,result-1,0);
    if(chrg.cpMin==-1) {
        MessageBox(GetParent(hWndEdit),MSG_WRONG_STRING,APP_SHORT_TITLE,MB_OK|MB_ICONERROR);
        return;
    }

    chars=SendMessage(hWndEdit,EM_LINELENGTH,chrg.cpMin,0);

    if(column<=chars) chrg.cpMin+=column-1;
    else chrg.cpMin+=chars;
    chrg.cpMax=chrg.cpMin;

    SendMessage(hWndEdit,EM_EXSETSEL,0,(LPARAM)&chrg);
}

void DoSettingsFont(HWND hWndEdit) {
    cf.lStructSize    = sizeof (CHOOSEFONT);
    cf.hwndOwner      = GetParent(hWndEdit);
    cf.hDC            = NULL;
    cf.lpLogFont      = &logfont;
    cf.iPointSize     = 0;
    cf.Flags          = CF_INITTOLOGFONTSTRUCT|CF_SCREENFONTS|CF_EFFECTS|CF_FORCEFONTEXIST;
#ifndef FOREIGN_BUILD
    cf.Flags|=CF_SELECTSCRIPT;
#endif
    cf.lCustData      = 0;
    cf.lpfnHook       = NULL;
    cf.lpTemplateName = NULL;
    cf.hInstance      = NULL;
    cf.lpszStyle      = NULL;
    cf.nFontType      = 0;
    cf.nSizeMin       = 0;
    cf.nSizeMax       = 0;

    /*
    Allow user to select only Cyrillic fonts
    to avoid changing font applied only for Latin letters
    and subsequent problems with setting up font
    for entire text
    (courtesy of ;)) RichEdit)
    */

#ifndef FOREIGN_BUILD
    logfont.lfCharSet=RUSSIAN_CHARSET;
#endif

    memcpy(&logfont2,&logfont,sizeof(LOGFONT));
    if(ChooseFont(&cf)) {
        SetChosenFont(hWndEdit);
    }
    else {
        memcpy(&logfont,&logfont2,sizeof(LOGFONT));
    }
}

void DoSettingsBG(HWND hWndEdit) {
    COLORREF temp;
    temp=cc.rgbResult;
    if(ChooseColor(&cc)) {
        SendMessage(hWndEdit,EM_SETBKGNDCOLOR,0,cc.rgbResult);
        usesystembckgcolor=FALSE;
    }
    else cc.rgbResult=temp;
}

void DoSettingsNoMultiOpen(HWND hWndEdit) {
    HMENU hMenu;

    hMenu=GetMenu(GetParent(hWndEdit));
    open_in_new_window=!open_in_new_window;
    if(open_in_new_window) {
        CheckMenuItem(hMenu,IDM_SETTINGS_NOMULTIOPEN,MF_UNCHECKED);
    }
    else {
        CheckMenuItem(hMenu,IDM_SETTINGS_NOMULTIOPEN,MF_CHECKED);
    }
}

void DoSettingsAsian(HWND hWndEdit) {
    HMENU hMenu;

    hMenu=GetMenu(GetParent(hWndEdit));
    support_asian=!support_asian;
    if(!support_asian) {
        CheckMenuItem(hMenu,IDM_SETTINGS_ASIAN,MF_UNCHECKED);
        SendMessage(hWndEdit,EM_SETLANGOPTIONS,(WPARAM)0,(LPARAM)0);
    }
    else {
        CheckMenuItem(hMenu,IDM_SETTINGS_ASIAN,MF_CHECKED);
        SendMessage(hWndEdit,EM_SETLANGOPTIONS,(WPARAM)0,(LPARAM)lang_settings);
    }
}

void DoCodePageOpenAs(HWND hWndEdit) {
    HWND hWndParent;
    int choice;

    if(*ofn.lpstrFile) {
        if(!GetModify()) OpenDocument(hWndEdit,FALSE);
        else if(MessageBox(GetParent(hWndEdit),MSG_FILE_WILL_BE_REOPENED,APP_SHORT_TITLE,MB_OKCANCEL|MB_ICONEXCLAMATION)==IDOK) OpenDocument(hWndEdit,FALSE);
    }
    else {

        hWndParent=GetParent(hWndEdit);

        ofn.Flags=OFN_CREATEPROMPT|OFN_LONGNAMES|OFN_HIDEREADONLY;
        ofn.lpstrDefExt=NULL;

        if(GetModify()) {
            choice=MessageBox(hWndParent,MSG_DOCUMENT_CHANGED,APP_SHORT_TITLE,MB_YESNOCANCEL|MB_ICONEXCLAMATION);
            if(choice==IDYES) {
                if(!DoFileSave(hWndEdit)) choice=IDCANCEL;
            }
        }
        else choice=IDNO;

        if(choice==IDCANCEL) return;

        if(GetOpenFileName(&ofn)) {
            OpenDocument(hWndEdit,FALSE);
        }

    }
}

int DoCodePageSaveIn(HWND hWndEdit) {
    if(file_buffer[0]==0) return DoFileSaveAs(hWndEdit);
    SaveDocument(hWndEdit);
    return -1;
}

void DoHelpAbout(HWND hWndEdit) {
    DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_ABOUT),GetParent(hWndEdit),(DLGPROC)AboutDlgProc);
}

void DoNonMenuDelLine(HWND hWndEdit) {
    CHARRANGE chrg;
    int line,len,column;

    SendMessage(hWndEdit,EM_EXGETSEL,0,(LPARAM)&chrg);

    line=SendMessage(hWndEdit,EM_EXLINEFROMCHAR,0,chrg.cpMin)+1;
    column=SendMessage(hWndEdit,EM_LINEINDEX,line-1,0);
    column=chrg.cpMin-column+1;

    if(line+1==SendMessage(hWndEdit,EM_GETLINECOUNT,0,0)&&column==SendMessage(hWndEdit,EM_LINELENGTH,chrg.cpMin,0)+2)
        line++;

    chrg.cpMin=SendMessage(hWndEdit,EM_LINEINDEX,line-1,0);

    len=SendMessage(hWndEdit,EM_LINELENGTH,chrg.cpMin,0);
    chrg.cpMax=chrg.cpMin+len+1;

    SendMessage(hWndEdit,EM_EXSETSEL,0,(LPARAM)&chrg);
    SendMessage(hWndEdit,EM_REPLACESEL,TRUE,(LPARAM)_T(""));
}

void DoNonMenuTab(HWND hWndEdit, BOOL istab, BOOL add) {
    CHARRANGE chrg;
    TEXTRANGE tr;
    EDITSTREAM es;
    TCHAR szIndent[2]={_T('\t'),_T('\0')};
    TCHAR tmpbuf[16];
    int iFirstLine,iLastLine;
    int iCharacters,iLines;
    int iInputBufferSize;
    int iOutputBufferSize;
    unsigned int read;
    wchar_t *szInputBuffer;
    wchar_t *szOutputBuffer;
    int i, j=0, deleted=0;
    wchar_t IndentChar=0x0009;

    if(!istab) IndentChar=0x0020;

    SendMessage(hWndEdit,EM_EXGETSEL,0,(LPARAM)&chrg);
    iFirstLine=SendMessage(hWndEdit,EM_EXLINEFROMCHAR,0,chrg.cpMin);
    iLastLine=SendMessage(hWndEdit,EM_EXLINEFROMCHAR,0,chrg.cpMax);

    iCharacters=chrg.cpMax-chrg.cpMin;
    iLines=iLastLine-iFirstLine+1;

    if(chrg.cpMin==chrg.cpMax) {
        if(!istab) szIndent[0]=_T(' ');
        if(!insertstate) SendMessage(hWndEdit,EM_SETSEL,chrg.cpMin,chrg.cpMin+1);
        SendMessage(hWndEdit,EM_REPLACESEL,(WPARAM)TRUE,(LPARAM)szIndent);
        return;
    }

    tr.chrg.cpMin=chrg.cpMax?(chrg.cpMax-1):chrg.cpMax;
    tr.chrg.cpMax=chrg.cpMax;
    tr.lpstrText=tmpbuf;
    SendMessage(hWndEdit,EM_GETTEXTRANGE,(WPARAM)0,(LPARAM)&tr);
    if(tmpbuf[0]==_T('\x0D')) {
        chrg.cpMax--;
        SendMessage(hWndEdit,EM_EXSETSEL,(WPARAM)0,(LPARAM)&chrg);
        SendMessage(hWndEdit,EM_EXGETSEL,0,(LPARAM)&chrg);
        iFirstLine=SendMessage(hWndEdit,EM_EXLINEFROMCHAR,0,chrg.cpMin);
        iLastLine=SendMessage(hWndEdit,EM_EXLINEFROMCHAR,0,chrg.cpMax);

        iCharacters=chrg.cpMax-chrg.cpMin;
        iLines=iLastLine-iFirstLine+1;
    }

    if(iLines==1) {
        if(!istab) szIndent[0]=_T(' ');
        SendMessage(hWndEdit,EM_REPLACESEL,(WPARAM)TRUE,(LPARAM)szIndent);
    }
    else {
        iInputBufferSize=(iCharacters+1)*sizeof(wchar_t);
        szInputBuffer=(wchar_t *)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,iInputBufferSize);
        if(!szInputBuffer) return;
        iOutputBufferSize=(iCharacters+iLines+1)*sizeof(wchar_t);
        szOutputBuffer=(wchar_t *)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,iOutputBufferSize);
        if(!szOutputBuffer) {
            HeapFree(GetProcessHeap(),0,(LPVOID)szInputBuffer);
            return;
        }
        //  gt.cb=iInputBufferSize;
        //  gt.flags=2; //GT_SELECTION, from the latest Platform SDK
        //  gt.codepage=1200; //Unicode
        //  gt.lpDefaultChar=NULL;
        //  gt.lpUsedDefChar=NULL;
        //  read=SendMessage(hWndEdit,EM_GETTEXTEX,(WPARAM)&gt,(LPARAM)szInputBuffer);
        es.dwCookie=0;
        es.pfnCallback=ReadBufferCallback;
        pStreamInBuffer=(char *)szInputBuffer;
        iStreamInBufferSize=iInputBufferSize;
        read=SendMessage(hWndEdit,EM_STREAMOUT,(WPARAM)SF_TEXT|SFF_SELECTION|SF_UNICODE,(LPARAM)&es);

        //  read++;
        /*  if(read!=(unsigned int)iInputBufferSize) {

        HeapFree(GetProcessHeap(),0,(LPVOID)szInputBuffer);
        HeapFree(GetProcessHeap(),0,(LPVOID)szOutputBuffer);
        return;
        }*/
        // Begin transform
        if(add) {
            szOutputBuffer[j]=IndentChar;
            deleted++;
            j++;
            for(i=0;i<=iInputBufferSize/(int)sizeof(wchar_t)-2;i++) {
                szOutputBuffer[j]=szInputBuffer[i];
                j++;
                if(szInputBuffer[i]==0x000D) {
                    szOutputBuffer[j]=IndentChar;
                    j++;
                    deleted++;
                }
            }
        }
        else {
            if(szInputBuffer[0]==IndentChar) {
                i=1;
                deleted++;
            }
            else i=0;
            for(;i<=iInputBufferSize/(int)sizeof(wchar_t)-2;i++) {
                szOutputBuffer[j]=szInputBuffer[i];
                if(szInputBuffer[i]!=IndentChar||i<=0||szInputBuffer[i-1]!=0x000D) j++;
                else deleted++;
            }
        }
        // End transform
        //  SendMessageW(hWndEdit,EM_REPLACESEL,(WPARAM)TRUE,(LPARAM)szOutputBuffer);
        es.dwCookie=0;
        es.pfnCallback=BufferCallback;
        pStreamInBuffer=(char *)szOutputBuffer;
        iStreamInBufferSize=lstrlenW(szOutputBuffer)*2;
        SendMessage(hWndEdit,EM_STREAMIN,(WPARAM)SF_TEXT|SFF_SELECTION|SF_UNICODE,(LPARAM)&es);
        if(add) chrg.cpMax+=deleted;
        else chrg.cpMax-=deleted;
        SendMessage(hWndEdit,EM_EXSETSEL,(WPARAM)0,(LPARAM)&chrg);
        HeapFree(GetProcessHeap(),0,(LPVOID)szInputBuffer);
        HeapFree(GetProcessHeap(),0,(LPVOID)szOutputBuffer);
    }
}

DWORD CALLBACK BufferCallback(DWORD dwCookie,LPBYTE pbBuff,LONG cb,LONG FAR *pcb) {
    int iToWrite;
    UNREFERENCED_PARAMETER(dwCookie);
    iToWrite=(iStreamInBufferSize<cb)?iStreamInBufferSize:cb;
    memcpy(pbBuff,pStreamInBuffer,iToWrite);
    pStreamInBuffer+=iToWrite;
    iStreamInBufferSize-=iToWrite;
    *pcb=iToWrite;
    return 0;
}

DWORD CALLBACK ReadBufferCallback(DWORD dwCookie,LPBYTE pbBuff,LONG cb,LONG FAR *pcb) {
    int iToWrite;
    int written=0;
    int i;

    wchar_t *pFrom;
    wchar_t *pTo;

    UNREFERENCED_PARAMETER(dwCookie);

    iToWrite=cb;
    // memcpy(pStreamInBuffer,pbBuff,iToWrite);
    pFrom=(wchar_t *)pbBuff;
    pTo=(wchar_t *)pStreamInBuffer;
    for(i=0;i<=iToWrite/2-1;i++) {
        if(pFrom[i]!=0x000A) {
            if(written*2>=iStreamInBufferSize) break;
            pTo[written]=pFrom[i];
            written++;
        }
    }
    pStreamInBuffer+=written*2;
    iStreamInBufferSize-=written*2;
    *pcb=iToWrite;
    return 0;
}

void SetChosenFont(HWND hWndEdit) {
    HFONT hNewFont;
    RECT rEdit;
    CHARFORMAT cfmt;

    hNewFont=CreateFontIndirect(&logfont);
    SendMessage(hWndEdit,WM_SETFONT,(WPARAM)hNewFont,(LPARAM)TRUE);
    if(hCurrentFont) DeleteObject(hCurrentFont);
    hCurrentFont=hNewFont;
    SendMessage(hWndEdit,EM_SETMARGINS,EC_LEFTMARGIN|EC_RIGHTMARGIN,MAKELONG(4,4));
    if(GetClientRect(hWndEdit,&rEdit)) {
        InvalidateRect(hWndEdit,&rEdit,FALSE);
        UpdateWindow(hWndEdit);
    }

    cfmt.cbSize=sizeof(CHARFORMAT);
    cfmt.dwMask=CFM_COLOR;
    cfmt.dwEffects=0;
    cfmt.crTextColor=cf.rgbColors;
    cfmt.bPitchAndFamily=logfont.lfPitchAndFamily;

    SendMessage(hWndEdit,EM_SETCHARFORMAT,SCF_ALL,(LPARAM)&cfmt);
}

DWORD CALLBACK InputStreamCallback(DWORD dwCookie,LPBYTE pbBuff,LONG cb,LONG FAR *pcb) {
    /*
    * Some unmaintainable code in this function
    */
    int iBytesToRestore,iBytesWritten=0,iBytesRead,iTemp;
    unsigned char *endptr;
    static int iAddReadBytes,iTranslateBufferSize;
    wchar_t *win_ptr,*unix_ptr;

    if(firsttime) {
        iAddReadBytes=0;
        iTranslateBufferSize=0;
    }

    if(iTranslateBufferSize) {
        iBytesToRestore=(cb<iTranslateBufferSize)?cb:iTranslateBufferSize;
        iTranslateBufferSize-=iBytesToRestore;
        iBytesWritten+=iBytesToRestore;
        memcpy(pbBuff,pcTranslateBuffer,iBytesToRestore);
        if(iTranslateBufferSize) MoveMemory(pcTranslateBuffer,pcTranslateBuffer+iBytesToRestore,iTranslateBufferSize);
    }

    while(iBytesWritten<cb) {
        if(!ReadFile((HANDLE)dwCookie,pcReadBuffer+iAddReadBytes,READBUFFERSIZE-iAddReadBytes,(DWORD *)&iBytesRead,NULL)) {
            MessageBox(NULL,MSG_ERROR_IO,APP_SHORT_TITLE,MB_OK|MB_ICONERROR);
            return 0xFFFFFFFF;
        }

        if(firsttime) {
            if(iBytesRead<3) {
                if(!ReadFile((HANDLE)dwCookie,pcReadBuffer+iAddReadBytes+iBytesRead,1,(DWORD *)&iTemp,NULL)) {
                    MessageBox(NULL,MSG_ERROR_IO,APP_SHORT_TITLE,MB_OK|MB_ICONERROR);
                    return 0xFFFFFFFF;
                }
                if(iTemp) iBytesRead++;
                if(!ReadFile((HANDLE)dwCookie,pcReadBuffer+iAddReadBytes+iBytesRead+iTemp,1,(DWORD *)&iTemp,NULL)) {
                    MessageBox(NULL,MSG_ERROR_IO,APP_SHORT_TITLE,MB_OK|MB_ICONERROR);
                    return 0xFFFFFFFF;
                }
                if(iTemp) iBytesRead++;
            }
        }

        if(iBytesRead==0) break;

        iTranslateBufferSize=TranslateTextToRepresent(pcReadBuffer,iBytesRead+iAddReadBytes,pcTranslateBuffer,&endptr,firsttime);
        if(firsttime) firsttime=FALSE;
        if(!iTranslateBufferSize&&endptr==pcTranslateBuffer) {
            MessageBox(NULL,MSG_ERROR_CANNOT_TRANSLATE,APP_SHORT_TITLE,MB_OK|MB_ICONERROR);
            return 0xFFFFFFFF;
        }

        if(!newline_detected) {
            win_ptr=wmemchr((wchar_t*)pcTranslateBuffer,0x0D,iTranslateBufferSize/2);
            unix_ptr=wmemchr((wchar_t*)pcTranslateBuffer,0x0A,iTranslateBufferSize/2);
            if(win_ptr&&!unix_ptr) {
                newline_detected=TRUE;
            }
            else if(!win_ptr&&unix_ptr) {
                newline_detected=TRUE;
                unix_newline=TRUE;
            }
            else if(win_ptr&&unix_ptr) {
                if(unix_ptr<win_ptr) unix_newline=TRUE;
                newline_detected=TRUE;
            }
        }

        iBytesToRestore=((cb-iBytesWritten)<iTranslateBufferSize)?(cb-iBytesWritten):iTranslateBufferSize;
        if(iBytesToRestore) memcpy(pbBuff+iBytesWritten,pcTranslateBuffer,iBytesToRestore);
        iTranslateBufferSize-=iBytesToRestore;
        iBytesWritten+=iBytesToRestore;
        iAddReadBytes=iBytesRead+iAddReadBytes-(endptr-pcReadBuffer);
        if(iTranslateBufferSize) MoveMemory(pcTranslateBuffer,pcTranslateBuffer+iBytesToRestore,iTranslateBufferSize);
        if(iAddReadBytes) {
            MoveMemory(pcReadBuffer,endptr,iBytesRead-iAddReadBytes);
        }
    }

    *pcb=iBytesWritten;

    return 0;
}

DWORD CALLBACK OutputStreamCallback(DWORD dwCookie,LPBYTE pbBuff,LONG cb,LONG FAR *pcb) {
    /*
    * Some unmaintainable code in this function
    */
    char *pcReadingFrom;
    int iFormBuffer=0;
    int iBytesTranslated,iBytesWritten;
    DWORD iBytesWrittenNow;
    int iTemp;
    int iRealBytes;

    if(unix_newline) iRealBytes=TranslateNewlinesToUnix((wchar_t *)pbBuff,cb);
    else iRealBytes=cb;

    pcReadingFrom=(char *)pbBuff;

    do {
        iBytesWritten=0;
        while(iFormBuffer<READBUFFERSIZE&&(pcReadingFrom-(char *)pbBuff)<iRealBytes) { 
            iTemp=iRealBytes-(pcReadingFrom-(char *)pbBuff);
            if(READBUFFERSIZE-iFormBuffer<iTemp) iTemp=READBUFFERSIZE-iFormBuffer;
            memcpy(&pcReadBuffer[iFormBuffer],pcReadingFrom,iTemp);
            iFormBuffer+=iTemp;
            pcReadingFrom+=iTemp;
        }

        iBytesTranslated=TranslateTextFromRepresent(pcReadBuffer,iFormBuffer,pcTranslateBuffer,firsttime);
        if(!iBytesTranslated) {
            MessageBox(NULL,MSG_ERROR_CANNOT_TRANSLATE,APP_SHORT_TITLE,MB_OK|MB_ICONERROR);
            return 0xFFFFFFFF;
        }
        if(firsttime) firsttime=FALSE;

        do {
            if(!WriteFile((HANDLE)dwCookie,pcTranslateBuffer+iBytesWritten,iBytesTranslated-iBytesWritten,&iBytesWrittenNow,NULL)) {
                MessageBox(NULL,MSG_ERROR_IO,APP_SHORT_TITLE,MB_OK|MB_ICONERROR);
                return 0xFFFFFFFF;
            }
            iBytesWritten+=iBytesWrittenNow;
        }while(iBytesWritten<iBytesTranslated);

        iFormBuffer=0;
    }while((pcReadingFrom-(char *)pbBuff)<iRealBytes&&!((pcReadingFrom-(char *)pbBuff)%2));

    *pcb=cb;

    return 0;
}

DWORD CALLBACK DummyOutputStreamCallback(DWORD dwCookie,LPBYTE pbBuff,LONG cb,LONG FAR *pcb) {
    if(!WriteFile((HANDLE)dwCookie,pbBuff,(DWORD) cb,(DWORD *)pcb,NULL)) {
        MessageBox(NULL,MSG_ERROR_IO,APP_SHORT_TITLE,MB_OK|MB_ICONERROR);
        return 0xFFFFFFFF;
    }
    return 0;
}

DWORD CALLBACK DummyInputStreamCallback(DWORD dwCookie,LPBYTE pbBuff,LONG cb,LONG FAR *pcb) {
    char *win_ptr, *unix_ptr;

    if(!ReadFile((HANDLE)dwCookie,pbBuff,(DWORD) cb,(DWORD *)pcb,NULL)) {
        MessageBox(NULL,MSG_ERROR_IO,APP_SHORT_TITLE,MB_OK|MB_ICONERROR);
        return 0xFFFFFFFF;
    }
    if(!newline_detected) {
        win_ptr=(char *)memchr(pbBuff,0x0D,*pcb);
        unix_ptr=(char *)memchr(pbBuff,0x0A,*pcb);
        if(win_ptr&&!unix_ptr) {
            newline_detected=TRUE;
        }
        else if(!win_ptr&&unix_ptr) {
            newline_detected=TRUE;
            unix_newline=TRUE;
        }
        else if(win_ptr&&unix_ptr) {
            if(unix_ptr<win_ptr) unix_newline=TRUE;
            newline_detected=TRUE;
        }
    }
    return 0;
}

void OpenDocument(HWND hWndEdit, BOOL mustexist) {
    CHARRANGE chrg;
    HWND hWndParent;
    HANDLE hFile;
    EDITSTREAM es;
    TCHAR szWindowTitle[MAX_PATH+STD_LANGPACK_STRING_LENGTH+5+1];
    WIN32_FIND_DATA w32fdInfo;
    int i,cp;
    DWORD dwBytesRead;
    TCHAR szFullFileName[MAX_PATH];
    TCHAR *p;

    RenewHistoryMenu();

    unix_newline=FALSE;
    newline_detected=FALSE;

    firsttime=TRUE;

    hWndParent=GetParent(hWndEdit);

    hFile=CreateFile(ofn.lpstrFile,
        GENERIC_READ,
        FILE_SHARE_READ|FILE_SHARE_WRITE,
        NULL,
        mustexist?OPEN_EXISTING:OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,
        NULL);

    if(hFile==INVALID_HANDLE_VALUE) {
        file_buffer[0]=_T('\0');
        MessageBox(hWndParent,MSG_ERROR_CANNOT_OPEN_FILE,APP_SHORT_TITLE,MB_OK|MB_ICONERROR);
        return;
    }

    if(need_autodetect) {
        if(!ReadFile(hFile,pcReadBuffer,AUTODETECTSIZE,&dwBytesRead,NULL)) {
            MessageBox(hWndParent,MSG_ERROR_IO,APP_SHORT_TITLE,MB_OK|MB_ICONERROR);
            return;
        }
        AutodetectCodePage(pcReadBuffer,dwBytesRead);
        SetFilePointer(hFile,0,NULL,FILE_BEGIN);
        need_autodetect=FALSE;
    }

    cp=GetCodePage();

    es.dwCookie=(DWORD)hFile;
    es.pfnCallback=(cp==WINDOWS_1251)?DummyInputStreamCallback:InputStreamCallback;

    HideCaret(NULL);

    if(cp==WINDOWS_1251) SendMessage(hWndEdit,EM_STREAMIN,SF_TEXT,(LPARAM)&es);
    else SendMessage(hWndEdit,EM_STREAMIN,SF_TEXT|SF_UNICODE,(LPARAM)&es);
    SetModify(FALSE);

    ShowCaret(NULL);

    CloseHandle(hFile);

    //Set caption of synchronizing window
    if(GetFullPathName(ofn.lpstrFile,MAX_PATH,szFullFileName,&p)) {
        SetWindowText(hSyncWindow,szFullFileName);
    }

    hFile=FindFirstFile(ofn.lpstrFile,&w32fdInfo);
    FindClose(hFile);

    _stprintf(szWindowTitle,_T("%s - %s"),(hFile!=INVALID_HANDLE_VALUE)?w32fdInfo.cFileName:ofn.lpstrFile,APP_SHORT_TITLE);

    if(hFile!=INVALID_HANDLE_VALUE) {
        for(i=_tcslen(ofn.lpstrFile)-1;i>=0;i--) {
            if(ofn.lpstrFile[i]==_T('\\')) break;
        }
        if(ofn.lpstrFile[i]==_T('\\')) {
            i++;
            ofn.lpstrFile[i]=_T('\0');
            _tcsncat(ofn.lpstrFile,w32fdInfo.cFileName,MAX_PATH-_tcslen(ofn.lpstrFile));
        }
    }

    SetWindowText(hWndParent,szWindowTitle);

    if(GlobalPrint) {
        DoFilePrint(hWndEdit);
        DestroyWindow(GetParent(hWndEdit));
    }

    for(i=0;i<=RECENTFILES-1;i++) {
        if(!_tcscmp(ofn.lpstrFile,recent_names[i])) {
            chrg.cpMin=recent_positions[i];
            chrg.cpMax=recent_positions[i];
            SendMessage(hWndEdit,EM_EXSETSEL,0,(LPARAM)&chrg);
            break;
        }
    }
}

void SaveDocument(HWND hWndEdit) {
    DWORD dwAttr;
    HWND hWndParent;
    EDITSTREAM es;
    HANDLE hFile;
    WIN32_FIND_DATA w32fdInfo;
    TCHAR realname[MAX_PATH+1];
    TCHAR szWindowTitle[MAX_PATH+STD_LANGPACK_STRING_LENGTH+5+1];
    int i;
    int cp,choice;

    cp=GetCodePage();
    if((cp==CP_UNICODE_UCS2_LE||cp==CP_UNICODE_UCS2_BE||cp==CP_UNICODE_UTF8)&&!append_header) {
        choice=MessageBox(GetParent(hWndEdit),MSG_WHETHER_APPEND_HEADER,MSG_TITLE_SAVING_IN_UNICODE,MB_YESNOCANCEL|MB_ICONQUESTION);
        if(choice==IDCANCEL) return;
        else if(choice==IDYES) append_header=TRUE;
    }

    firsttime=TRUE;

    hWndParent=GetParent(hWndEdit);

    hFile=FindFirstFile(ofn.lpstrFile,&w32fdInfo);
    FindClose(hFile);

    _tcsncpy(realname,ofn.lpstrFile,MAX_PATH);

    if(hFile!=INVALID_HANDLE_VALUE) {
        for(i=_tcslen(realname)-1;i>=0;i--) {
            if(realname[i]==_T('\\')) break;
        }
        if(realname[i]==_T('\\')) {
            i++;
            realname[i]=_T('\0');
            _tcsncat(realname,w32fdInfo.cFileName,MAX_PATH-_tcslen(realname));
        }
        else _tcsncpy(realname,w32fdInfo.cFileName,MAX_PATH);
    }

    dwAttr=GetFileAttributes(realname);
    if(dwAttr!=0xFFFFFFFF) {
        if(dwAttr&FILE_ATTRIBUTE_READONLY||dwAttr&FILE_ATTRIBUTE_HIDDEN||dwAttr&FILE_ATTRIBUTE_SYSTEM) 
            SetFileAttributes(realname,(dwAttr&(!FILE_ATTRIBUTE_READONLY)&(!FILE_ATTRIBUTE_HIDDEN)&(!FILE_ATTRIBUTE_SYSTEM)));
    }

    hFile=CreateFile(realname,
        GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,
        NULL);

    if(hFile==INVALID_HANDLE_VALUE) {
        MessageBox(hWndParent,MSG_ERROR_CANNOT_OPEN_FILE,APP_SHORT_TITLE,MB_OK|MB_ICONERROR);
        if(dwAttr!=0xFFFFFFFF) SetFileAttributes(realname,dwAttr);
        return;
    }

    es.dwCookie=(DWORD)hFile;
    es.pfnCallback=(cp==WINDOWS_1251&&!unix_newline)?DummyOutputStreamCallback:OutputStreamCallback;

    if(cp==WINDOWS_1251&&!unix_newline) SendMessage(hWndEdit,EM_STREAMOUT,SF_TEXT,(LPARAM)&es);
    else SendMessage(hWndEdit,EM_STREAMOUT,SF_TEXT|SF_UNICODE,(LPARAM)&es);
    SetModify(FALSE);

    CloseHandle(hFile);

    hFile=FindFirstFile(ofn.lpstrFile,&w32fdInfo);
    FindClose(hFile);

    _stprintf(szWindowTitle,_T("%s - %s"),(hFile!=INVALID_HANDLE_VALUE)?w32fdInfo.cFileName:ofn.lpstrFile,APP_SHORT_TITLE);
    SetWindowText(hWndParent,szWindowTitle);

    if(dwAttr!=0xFFFFFFFF) SetFileAttributes(realname,dwAttr|FILE_ATTRIBUTE_ARCHIVE);
}

BOOL CALLBACK FindDlgProc(HWND hDlg,UINT msg,WPARAM wParam,LPARAM lParam) {
    static HWND hWndText;
    int characters;

    UNREFERENCED_PARAMETER(lParam);

    switch(msg) {
    case WM_INITDIALOG:
        SendMessage(hDlg,WM_SETICON,(WPARAM)ICON_BIG,(LPARAM)LoadIcon(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_ICON1)));
        hWndText=GetDlgItem(hDlg,IDC_TEXT);
        if(org_query) {
            SetWindowText(hWndText,org_query);
            SendMessage(hWndText,EM_SETSEL,0,_tcslen(org_query));
        }
        SendMessage(GetDlgItem(hDlg,IDC_BACKWARD),BM_SETCHECK,(int)(!(BOOL)(ftflags&FR_DOWN)),0);
        SendMessage(GetDlgItem(hDlg,IDC_MATCHCASE),BM_SETCHECK,(int)(BOOL)(ftflags&FR_MATCHCASE),0);
        SendMessage(GetDlgItem(hDlg,IDC_WHOLEWORD),BM_SETCHECK,(int)(BOOL)(ftflags&FR_WHOLEWORD),0);
        SendMessage(GetDlgItem(hDlg,IDC_ESCAPESEQ),BM_SETCHECK,(int)use_escape,0);
        return TRUE;

    case WM_COMMAND:
        switch(LOWORD(wParam)) {
        case IDC_TEXT:
        case IDC_ESCAPESEQ:
        case IDC_MATCHCASE:
        case IDC_WHOLEWORD:
        case IDC_BACKWARD:
            return TRUE;
        case IDCANCEL:
            EndDialog(hDlg,0);
            return TRUE;
        case IDOK:
            characters=GetWindowTextLength(hWndText);
            if(ft.lpstrText) HeapFree(GetProcessHeap(),0,(LPVOID)ft.lpstrText);
            if(org_query) HeapFree(GetProcessHeap(),0,(LPVOID)org_query);

            ft.lpstrText=(TCHAR *)HeapAlloc(GetProcessHeap(),0, (2*(characters+1))*sizeof(TCHAR));
            if(!ft.lpstrText) {
                MessageBox(NULL,MSG_ERROR_NOT_ENOUGH_MEMORY,NULL,MB_OK|MB_ICONERROR);
                ExitProcess(1);
            }

            org_query=(TCHAR *)HeapAlloc(GetProcessHeap(),0,(characters+1)*sizeof(TCHAR));
            if(!org_query) {
                MessageBox(NULL,MSG_ERROR_NOT_ENOUGH_MEMORY,NULL,MB_OK|MB_ICONERROR);
                ExitProcess(1);
            }

            GetWindowText(hWndText,org_query,characters+1);

            ftflags=0;
            if(!SendMessage(GetDlgItem(hDlg,IDC_BACKWARD),BM_GETCHECK,0,0)) ftflags|=FR_DOWN;
            if(SendMessage(GetDlgItem(hDlg,IDC_MATCHCASE),BM_GETCHECK,0,0)) ftflags|=FR_MATCHCASE;
            if(SendMessage(GetDlgItem(hDlg,IDC_WHOLEWORD),BM_GETCHECK,0,0)) ftflags|=FR_WHOLEWORD;
            if(SendMessage(GetDlgItem(hDlg,IDC_ESCAPESEQ),BM_GETCHECK,0,0)) {
                use_escape=TRUE;
                if(!TranslateBackslashes((TCHAR*)ft.lpstrText,org_query)) {
                    MessageBox(hDlg,MSG_ERROR_SYNTAX,APP_SHORT_TITLE,MB_OK|MB_ICONERROR);
                    return TRUE;
                }
            }
            else {
                use_escape=FALSE;
                _tcscpy((TCHAR*)ft.lpstrText,org_query);
            }
            EndDialog(hDlg,-1);
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}

BOOL CALLBACK ReplaceDlgProc(HWND hDlg,UINT msg,WPARAM wParam,LPARAM lParam) {
    static HWND hWndText, hWndNewText, hWndEdit;
    int characters;
    CHARRANGE cr;

    switch(msg) {
    case WM_INITDIALOG:
        hWndEdit=(HWND)lParam;
        SendMessage(hDlg,WM_SETICON,(WPARAM)ICON_BIG,(LPARAM)LoadIcon(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_ICON1)));
        hWndText=GetDlgItem(hDlg,IDC_TEXT);
        hWndNewText=GetDlgItem(hDlg,IDC_NEWTEXT);
        if(org_query) {
            SetWindowText(hWndText,org_query);
            SendMessage(hWndText,EM_SETSEL,0,_tcslen(org_query));
        }

        if(replaceto.org_query) {
            SetWindowText(hWndNewText,replaceto.org_query);
        }

        SendMessage(hWndEdit,EM_EXGETSEL,0,(LPARAM)&cr);
        if(cr.cpMin==cr.cpMax) {
            replace_in_sel=FALSE;
            EnableWindow(GetDlgItem(hDlg,IDC_INSEL),FALSE);
        }

        SendMessage(GetDlgItem(hDlg,IDC_BACKWARD),BM_SETCHECK,(int)(!(BOOL)(ftflags&FR_DOWN)),0);
        SendMessage(GetDlgItem(hDlg,IDC_MATCHCASE),BM_SETCHECK,(int)(BOOL)(ftflags&FR_MATCHCASE),0);
        SendMessage(GetDlgItem(hDlg,IDC_WHOLEWORD),BM_SETCHECK,(int)(BOOL)(ftflags&FR_WHOLEWORD),0);
        SendMessage(GetDlgItem(hDlg,IDC_ESCAPESEQ),BM_SETCHECK,(int)use_escape,0);
        SendMessage(GetDlgItem(hDlg,IDC_INSEL),BM_SETCHECK,(int)replace_in_sel,0);

        EnableWindow(GetDlgItem(hDlg,IDOK),replace_in_sel?FALSE:TRUE);
        return TRUE;

    case WM_COMMAND:
        switch(LOWORD(wParam)) {
        case IDC_TEXT:
        case IDC_NEWTEXT:
        case IDC_ESCAPESEQ:
        case IDC_MATCHCASE:
        case IDC_WHOLEWORD:
        case IDC_BACKWARD:
            return TRUE;
        case IDC_INSEL:
            if(SendMessage(GetDlgItem(hDlg,IDC_INSEL),BM_GETCHECK,0,0)) replace_in_sel=TRUE;
            else replace_in_sel=FALSE;
            EnableWindow(GetDlgItem(hDlg,IDOK),replace_in_sel?FALSE:TRUE);
            return TRUE;
        case IDCANCEL:
            EndDialog(hDlg,0);
            return TRUE;
        case IDOK:
            if(replace_in_sel) return TRUE;
        case IDC_ALL:
            if(ft.lpstrText) HeapFree(GetProcessHeap(),0,(LPVOID)ft.lpstrText);
            if(org_query) HeapFree(GetProcessHeap(),0,(LPVOID)org_query);

            if(replaceto.lpstrNewText) HeapFree(GetProcessHeap(),0,(LPVOID)replaceto.lpstrNewText);
            if(replaceto.org_query) HeapFree(GetProcessHeap(),0,(LPVOID)replaceto.org_query);

            characters=GetWindowTextLength(hWndText);
            ft.lpstrText=(TCHAR *)HeapAlloc(GetProcessHeap(),0,(2*(characters+1))*sizeof(TCHAR));
            if(!ft.lpstrText) {
                MessageBox(NULL,MSG_ERROR_NOT_ENOUGH_MEMORY,NULL,MB_OK|MB_ICONERROR);
                ExitProcess(1);
            }

            org_query=(TCHAR *)HeapAlloc(GetProcessHeap(),0,(characters+1)*sizeof(TCHAR));
            if(!org_query) {
                MessageBox(NULL,MSG_ERROR_NOT_ENOUGH_MEMORY,NULL,MB_OK|MB_ICONERROR);
                ExitProcess(1);
            }

            GetWindowText(hWndText,org_query,characters+1);

            characters=GetWindowTextLength(hWndNewText);
            replaceto.lpstrNewText=(TCHAR *)HeapAlloc(GetProcessHeap(),0,(2*(characters+1))*sizeof(TCHAR));
            if(!replaceto.lpstrNewText) {
                MessageBox(NULL,MSG_ERROR_NOT_ENOUGH_MEMORY,NULL,MB_OK|MB_ICONERROR);
                ExitProcess(1);
            }

            replaceto.org_query=(TCHAR *)HeapAlloc(GetProcessHeap(),0,(characters+1)*sizeof(TCHAR));
            if(!replaceto.org_query) {
                MessageBox(NULL,MSG_ERROR_NOT_ENOUGH_MEMORY,NULL,MB_OK|MB_ICONERROR);
                ExitProcess(1);
            }

            GetWindowText(hWndNewText,replaceto.org_query,characters+1);


            ftflags=0;
            if(!SendMessage(GetDlgItem(hDlg,IDC_BACKWARD),BM_GETCHECK,0,0)) ftflags|=FR_DOWN;
            if(SendMessage(GetDlgItem(hDlg,IDC_MATCHCASE),BM_GETCHECK,0,0)) ftflags|=FR_MATCHCASE;
            if(SendMessage(GetDlgItem(hDlg,IDC_WHOLEWORD),BM_GETCHECK,0,0)) ftflags|=FR_WHOLEWORD;
            if(SendMessage(GetDlgItem(hDlg,IDC_ESCAPESEQ),BM_GETCHECK,0,0)) {
                use_escape=TRUE;
                if(!TranslateBackslashes((TCHAR *)ft.lpstrText,org_query)||
                    !TranslateBackslashes(replaceto.lpstrNewText,replaceto.org_query))
                {
                    MessageBox(hDlg,MSG_ERROR_SYNTAX,APP_SHORT_TITLE,MB_OK|MB_ICONERROR);
                    return TRUE;
                }
            } else {
                use_escape=FALSE;
                _tcscpy((TCHAR *)ft.lpstrText,org_query);
                _tcscpy(replaceto.lpstrNewText,replaceto.org_query);
            }
            EndDialog(hDlg,LOWORD(wParam)==IDOK?-1:-2);
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}

BOOL CALLBACK GoToLineDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    static HWND hWndLine;
    int line;

    switch(message) {
    case WM_INITDIALOG:
        SendMessage(hDlg,WM_SETICON,(WPARAM)ICON_BIG,(LPARAM)LoadIcon(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_ICON1)));
        hWndLine=GetDlgItem(hDlg,IDC_GOTOLINE);

        line=(int)lParam;

        SetDlgItemInt(hDlg,IDC_GOTOLINE,line,FALSE);
        SendMessage(hWndLine,EM_SETSEL,0,GetWindowTextLength(hWndLine));
        return TRUE;
    case WM_COMMAND:
        switch(LOWORD(wParam)) {
        case IDC_GOTOLINE:
            return TRUE;
        case IDOK:
            line=GetDlgItemInt(hDlg,IDC_GOTOLINE,NULL,FALSE);
            if(!line) {
                MessageBox(hDlg,MSG_WRONG_STRING,APP_SHORT_TITLE,MB_OK|MB_ICONERROR);
            }
            else {
                EndDialog(hDlg,line);
            }
            return TRUE;
        case IDCANCEL:
            EndDialog(hDlg,0);
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}

BOOL CALLBACK ConfirmDlgProc(HWND hDlg,UINT msg,WPARAM wParam,LPARAM lParam) {
    UNREFERENCED_PARAMETER(lParam);
    switch(msg) {
    case WM_INITDIALOG:
        SendMessage(hDlg,WM_SETICON,(WPARAM)ICON_BIG,(LPARAM)LoadIcon(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_ICON1)));
        return TRUE;

    case WM_COMMAND:
        EndDialog(hDlg,LOWORD(wParam));
        return TRUE;
    }
    return FALSE;
}

int _FindText(HWND hWndEdit) {
    SendMessage(hWndEdit,EM_EXGETSEL,0,(LPARAM)&ft.chrg);

    _SetLayout();
    if(ftflags&FR_DOWN) {
        ft.chrg.cpMin=ft.chrg.cpMax;
        ft.chrg.cpMax=-1;
    }
    else ft.chrg.cpMax=0;

    ft.chrg.cpMin=SendMessage(hWndEdit,EM_FINDTEXT,ftflags,(LPARAM)&ft);

    if(ft.chrg.cpMin==-1);
    else {
        ft.chrg.cpMax=ft.chrg.cpMin+_tcslen(ft.lpstrText);
        SendMessage(hWndEdit,EM_EXSETSEL,0,(LPARAM)&ft.chrg);
    }
    _RestoreLayout();

    return ft.chrg.cpMin;
}

void _ReplaceText(HWND hWndEdit,BOOL all)
{
    int result;
    CHARRANGE crInitial = { 0 };
    BOOL firstpass=TRUE;

    _SetLayout();

    if(replace_in_sel) SendMessage(hWndEdit,EM_EXGETSEL,0,(LPARAM)&crInitial);

    while(TRUE) {
        SendMessage(hWndEdit,EM_EXGETSEL,0,(LPARAM)&ft.chrg);

        if(replace_in_sel) {
            if(ftflags&FR_DOWN) {
                if(!firstpass) ft.chrg.cpMin=ft.chrg.cpMax;
                ft.chrg.cpMax=crInitial.cpMax;
            }
            else {
                if(firstpass) ft.chrg.cpMin=ft.chrg.cpMax;
                ft.chrg.cpMax=crInitial.cpMin;
            }
        }
        else {
            if(ftflags&FR_DOWN) {
                ft.chrg.cpMin=ft.chrg.cpMax;
                ft.chrg.cpMax=-1;
            }
            else {
                ft.chrg.cpMax=0;
            }
        }

        firstpass=FALSE;

        ft.chrg.cpMin=SendMessage(hWndEdit,EM_FINDTEXT,ftflags,(LPARAM)&ft);

        if(ft.chrg.cpMin==-1) break;
        else {
            ft.chrg.cpMax=ft.chrg.cpMin+_tcslen(ft.lpstrText);
            SendMessage(hWndEdit,EM_EXSETSEL,0,(LPARAM)&ft.chrg);
            if(!all) result=DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_CONFIRM),GetParent(hWndEdit),(DLGPROC)ConfirmDlgProc);
            else result=IDOK;
            switch(result) {
            case IDC_ALL:
                all=TRUE;
            case IDOK:
                if((ft.lpstrText[lstrlen(ft.lpstrText)-1]==_T('\x0D'))&&(lstrlen(replaceto.lpstrNewText)==1)) {
                    SendMessage(hWndEdit,EM_REPLACESEL,TRUE,(LPARAM)_T(""));
                }
                SendMessage(hWndEdit,EM_REPLACESEL,TRUE,(LPARAM)replaceto.lpstrNewText);
                ft.chrg.cpMax=ft.chrg.cpMin-_tcslen(ft.lpstrText)+_tcslen(replaceto.lpstrNewText);
                crInitial.cpMax=crInitial.cpMax-_tcslen(ft.lpstrText)+_tcslen(replaceto.lpstrNewText);
                break;
            case IDCANCEL:
                return;
            }
        }

    }
    _RestoreLayout();
}

BOOL TranslateBackslashes(TCHAR *newstr,TCHAR *string) {
    TCHAR *op,*np;
    TCHAR ch;

    terror=FALSE;

    for(op=string,np=newstr;*op;op++,np++) {
        if(*op!=_T('\\')) *np=*op;
        else {
            op++;
            if(!*op) break;
            if(*op==_T('\\')) *np=_T('\\');
            else if(*op==_T('n')) {
                *np=_T('\x0D');
            }
            else if(*op==_T('t')) {
                *np=_T('\t');
            }
            else {
                ch=TranslateHexadecimalDigit(*op)*16;
                op++;
                if(!*op) return FALSE;
                ch+=TranslateHexadecimalDigit(*op);
                *np=ch;
            }
        }
    }
    *np=_T('\0');
    if(!terror) return TRUE;
    return FALSE;
}

TCHAR TranslateHexadecimalDigit(TCHAR ch) {
    ch=toupper(ch);
    if(ch>=_T('0') && ch<=_T('9')) return ch-_T('0');
    if(ch>=_T('A') && ch<=_T('F')) return ch-_T('A')+0x0A;
    terror=TRUE;
    return 0;
}

BOOL RestorePreOptionsFromRegistry() {
    HKEY hKey;
    DWORD dwType, dwSize=sizeof(int);

    if(RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\Akelsoft\\AkelPad"),
        0, KEY_ALL_ACCESS, &hKey)!=ERROR_SUCCESS) return FALSE;

    RegQueryValueEx(hKey, _T("window_left"), NULL, &dwType,
        (LPBYTE)&MainWindowRect.left, &dwSize);

    RegQueryValueEx(hKey, _T("window_top"), NULL, &dwType,
        (LPBYTE)&MainWindowRect.top, &dwSize);

    RegQueryValueEx(hKey, _T("window_right"), NULL, &dwType,
        (LPBYTE)&MainWindowRect.right, &dwSize);

    RegQueryValueEx(hKey, _T("window_bottom"), NULL, &dwType,
        (LPBYTE)&MainWindowRect.bottom, &dwSize);
    return 0;
}

BOOL RestoreOptionsFromRegistry(HWND hWndEdit) {
    HKEY hKey;
    DWORD dwType, dwValue, dwSize=sizeof(DWORD);

    if(RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\Akelsoft\\AkelPad"),
        0, KEY_ALL_ACCESS, &hKey)!=ERROR_SUCCESS) return FALSE;

    if(RegQueryValueEx(hKey, _T("Wrap"), NULL, &dwType,
        (LPBYTE)&dwValue, &dwSize)==ERROR_SUCCESS) 
    {
        if(dwType==REG_DWORD&&dwValue) DoEditWrap(hWndEdit);
    }

    if(RegQueryValueEx(hKey, _T("Use_escape_sequences"), NULL, &dwType,
        (LPBYTE)&dwValue, &dwSize)==ERROR_SUCCESS)
    {
        if(dwType==REG_DWORD&&dwValue) use_escape=TRUE;
    }

    if(RegQueryValueEx(hKey, _T("Open_in_new_window"), NULL, &dwType,
        (LPBYTE)&dwValue, &dwSize)==ERROR_SUCCESS)
    {
        if(dwType==REG_DWORD&&!dwValue) DoSettingsNoMultiOpen(hWndEdit);
    }

    if(RegQueryValueEx(hKey, _T("Support_asian_characters"), NULL, &dwType,
        (LPBYTE)&dwValue, &dwSize)==ERROR_SUCCESS)
    {
        if(dwType==REG_DWORD&&dwValue) DoSettingsAsian(hWndEdit);
    }

    if(RegQueryValueEx(hKey, _T("Replace_in_selection"), NULL, &dwType,
        (LPBYTE)&dwValue, &dwSize)==ERROR_SUCCESS)
    {
        if(dwType==REG_DWORD&&dwValue) replace_in_sel=TRUE;
    }


    if(RegQueryValueEx(hKey, _T("Search_options"), NULL, &dwType,
        (LPBYTE)&dwValue, &dwSize)==ERROR_SUCCESS)
    {
        if(dwType==REG_DWORD) ftflags=dwValue;
    }

    if(RegQueryValueEx(hKey, _T("Search_query"), NULL, &dwType,
        (LPBYTE)NULL, &dwSize)==ERROR_SUCCESS)
    {
        if(dwType==REG_SZ&&dwSize>1) {
            org_query=(TCHAR *)HeapAlloc(GetProcessHeap(),0,dwSize*sizeof(TCHAR));
            if(!org_query) {
                MessageBox(NULL,MSG_ERROR_NOT_ENOUGH_MEMORY,NULL,MB_OK|MB_ICONERROR);
                ExitProcess(1);
            }

            if(RegQueryValueEx(hKey, _T("Search_query"), NULL, &dwType,
                (LPBYTE)org_query, &dwSize)!=ERROR_SUCCESS)
            {
                HeapFree(GetProcessHeap(),0,(LPVOID)org_query);
                org_query=NULL;
            }
        }
    }

    if(RegQueryValueEx(hKey, _T("Replace_string"), NULL, &dwType,
        (LPBYTE)NULL, &dwSize)==ERROR_SUCCESS)
    {
        if(dwType==REG_SZ&&dwSize>1) {
            replaceto.org_query=(TCHAR *)HeapAlloc(GetProcessHeap(),0,dwSize*sizeof(TCHAR));
            if(!replaceto.org_query) {
                MessageBox(NULL,MSG_ERROR_NOT_ENOUGH_MEMORY,NULL,MB_OK|MB_ICONERROR);
                ExitProcess(1);
            }

            if(RegQueryValueEx(hKey, _T("Replace_string"), NULL, &dwType,
                (LPBYTE)replaceto.org_query, &dwSize)!=ERROR_SUCCESS)
            {
                HeapFree(GetProcessHeap(),0,(LPVOID)replaceto.org_query);
                replaceto.org_query=NULL;
            }
        }
    }

    dwSize=sizeof(DWORD);

    if(RegQueryValueEx(hKey, _T("Keepspaces"), NULL, &dwType,
        (LPBYTE)&dwValue, &dwSize)==ERROR_SUCCESS)
    {
        if(dwType==REG_DWORD&&dwValue) DoEditKeepSpace(hWndEdit);
    }

    RegQueryValueEx(hKey, _T("Textcolor"), NULL, &dwType,
        (LPBYTE)&cf.rgbColors, &dwSize);

    if(RegQueryValueEx(hKey, _T("Background"), NULL, &dwType,
        (LPBYTE)&cc.rgbResult, &dwSize)==ERROR_SUCCESS)
    {
        if(dwType==REG_DWORD) {
            SendMessage(hWndEdit,EM_SETBKGNDCOLOR,0,cc.rgbResult);
            usesystembckgcolor=FALSE;
        }
    }

    dwSize=sizeof(LOGFONT);

    if(RegQueryValueEx(hKey, _T("Font"), NULL, &dwType,
        (LPBYTE)&logfont, &dwSize)==ERROR_SUCCESS)
    {
        if(dwType==REG_BINARY&&dwSize==sizeof(LOGFONT)) {
            SetChosenFont(hWndEdit);
        }
    }

    dwSize=sizeof(RECT);

    RegQueryValueEx(hKey, _T("Margins"), NULL, &dwType,
        (LPBYTE)&psdPage.rtMargin, &dwSize);

    dwSize=sizeof(DWORD);

    if(RegQueryValueEx(hKey, _T("Style"), NULL, &dwType,
        (LPBYTE)&dwValue, &dwSize)!=ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return FALSE;
    }

    if(dwType!=REG_DWORD) {
        RegCloseKey(hKey);
        return FALSE;
    }

    g_iCmdShow=dwValue?SW_MAXIMIZE:SW_RESTORE;

    return TRUE;
}

BOOL SaveOptionsInRegistry(HWND hWnd) {
    HKEY hKey;
    DWORD dwDisp,dwOne=1,dwZero=0;

    GetWindowRect(hWnd,&MainWindowRect);
    MainWindowRect.right-=MainWindowRect.left;
    MainWindowRect.bottom-=MainWindowRect.top;

    if(RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\Akelsoft\\AkelPad"), 0,
        NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
        NULL, &hKey, &dwDisp)!=ERROR_SUCCESS)
    {
        return FALSE;
    }

    if(RegSetValueEx(hKey, _T("Margins"), 0, REG_BINARY,
        (LPBYTE)&psdPage.rtMargin, sizeof(RECT))!=ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return FALSE;
    }

    if(RegSetValueEx(hKey, _T("Search_query"), 0, REG_SZ,
        (LPBYTE)(org_query?org_query:_T("")), org_query?(( _tcslen(org_query)+1) *sizeof(TCHAR) ):1)!=ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return FALSE;
    }

    if(RegSetValueEx(hKey, _T("Replace_string"), 0, REG_SZ,
        (LPBYTE)(replaceto.org_query?replaceto.org_query:_T("")),
        replaceto.org_query?((_tcslen(replaceto.org_query)+1)*sizeof(TCHAR)):1)!=ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return FALSE;
    }

    if(RegSetValueEx(hKey, _T("Use_escape_sequences"), 0, REG_DWORD,
        (LPBYTE)(use_escape?&dwOne:&dwZero), sizeof(DWORD))!=ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return FALSE;
    }

    if(RegSetValueEx(hKey, _T("Open_in_new_window"), 0, REG_DWORD,
        (LPBYTE)(open_in_new_window?&dwOne:&dwZero), sizeof(DWORD))!=ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return FALSE;
    }

    if(RegSetValueEx(hKey, _T("Support_asian_characters"), 0, REG_DWORD,
        (LPBYTE)(support_asian?&dwOne:&dwZero), sizeof(DWORD))!=ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return FALSE;
    }

    if(RegSetValueEx(hKey, _T("Replace_in_selection"), 0, REG_DWORD,
        (LPBYTE)(replace_in_sel?&dwOne:&dwZero), sizeof(DWORD))!=ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return FALSE;
    }

    if(RegSetValueEx(hKey, _T("Search_options"), 0, REG_DWORD,
        (LPBYTE)&ftflags, sizeof(DWORD))!=ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return FALSE;
    }

    if(RegSetValueEx(hKey, _T("Wrap"), 0, REG_DWORD,
        (LPBYTE)(wrap_on?&dwOne:&dwZero), sizeof(DWORD))!=ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return FALSE;
    }

    if(RegSetValueEx(hKey, _T("Keepspaces"), 0, REG_DWORD,
        (LPBYTE)(keep_on?&dwOne:&dwZero), sizeof(DWORD))!=ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return FALSE;
    }

    if(RegSetValueEx(hKey, _T("Font"), 0, REG_BINARY,
        (LPBYTE)&logfont, sizeof(LOGFONT))!=ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return FALSE;
    }

    if (RegSetValueEx(hKey, _T("Textcolor"), 0, REG_DWORD, 
        (LPBYTE)&cf.rgbColors, sizeof(DWORD))!=ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return FALSE;
    }

    if(RegSetValueEx(hKey, _T("Style"), 0, REG_DWORD,
        (LPBYTE)((GetWindowLong(hWnd,GWL_STYLE)&WS_MAXIMIZE)?&dwOne:&dwZero),
        sizeof(DWORD))!=ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return FALSE;
    }

    if(!(GetWindowLong(hWnd,GWL_STYLE)&WS_MINIMIZE)) {

        if(!(GetWindowLong(hWnd,GWL_STYLE)&WS_MAXIMIZE)) {
            if(RegSetValueEx(hKey, _T("window_left"), 0, REG_DWORD,
                (LPBYTE)&MainWindowRect.left, sizeof(DWORD))!=ERROR_SUCCESS)
            {
                RegCloseKey(hKey);
                return FALSE;
            }
            if(RegSetValueEx(hKey, _T("window_top"), 0, REG_DWORD,
                (LPBYTE)&MainWindowRect.top, sizeof(DWORD))!=ERROR_SUCCESS)
            {
                RegCloseKey(hKey);
                return FALSE;
            }
            if(RegSetValueEx(hKey, _T("window_right"), 0, REG_DWORD,
                (LPBYTE)&MainWindowRect.right, sizeof(DWORD))!=ERROR_SUCCESS)
            {
                RegCloseKey(hKey);
                return FALSE;
            }
            if(RegSetValueEx(hKey, _T("window_bottom"), 0, REG_DWORD,
                (LPBYTE)&MainWindowRect.bottom, sizeof(DWORD))!=ERROR_SUCCESS)
            {
                RegCloseKey(hKey);
                return FALSE;
            }
        }
    }

    if(!usesystembckgcolor) {
        if(RegSetValueEx(hKey, _T("Background"), 0, REG_DWORD,
            (LPBYTE)&cc.rgbResult, sizeof(DWORD))!=ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return FALSE;
        }
    }

    RegCloseKey(hKey);
    return TRUE;
}

BOOL CALLBACK AboutDlgProc(HWND hDlg,UINT msg,WPARAM wParam,LPARAM lParam) {
    UNREFERENCED_PARAMETER(lParam);
    if(msg==WM_INITDIALOG) {
        SendMessage(hDlg, WM_SETICON, (WPARAM)ICON_BIG,
            (LPARAM)LoadIcon(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_ICON1)));
        SetDlgItemText(hDlg,IDC_ABOUT_EMAIL, _T("developer@akelpad.net.ru"));
        SetDlgItemText(hDlg,IDC_ABOUT_ICQ, _T("109590531"));
    }
    if(msg==WM_COMMAND) {
        if (LOWORD(wParam)==IDOK || LOWORD(wParam)==IDCANCEL) {
            EndDialog(hDlg,0);
            return TRUE;
        } else if (LOWORD(wParam)==IDC_HOMEPAGE) {
            ShellExecute(GetParent(hDlg),_T("open"),STR_HOMEPAGE,NULL,NULL,SW_MAXIMIZE);
        }
    }
    return FALSE;
}

void _SetLayout() {
    TCHAR name[KL_NAMELENGTH];

    switched=FALSE;
    //Switch keyboard layout if it is English
    GetKeyboardLayoutName(name);
    if(!memcmp(name+_tcslen(name)-2, _T("09"), 2*sizeof(TCHAR))) {
        ActivateKeyboardLayout((HKL)HKL_NEXT,0);
        switched=TRUE;
    }
}

void _RestoreLayout() {
    if(switched) {
        ActivateKeyboardLayout((HKL)HKL_PREV,0);
        switched=FALSE;
    }
}

BOOL AddLastFilePosition(HWND hWndEdit) {
    CHARRANGE chrg;
    int i,j;

    TCHAR name_buf[MAX_PATH+2];
    int pos_buf;

    TCHAR current_file_buf[MAX_PATH+2];
    TCHAR *p;

    if(!GetFullPathName(ofn.lpstrFile,MAX_PATH,current_file_buf,&p)) {
        lstrcpy(current_file_buf,ofn.lpstrFile);
    }

    if(!ofn.lpstrFile||!*ofn.lpstrFile) return FALSE;

    RestoreLastFilePosition(hWndEdit);
    SendMessage(hWndEdit,EM_EXGETSEL,0,(LPARAM)&chrg);

    for(i=0;i<=RECENTFILES-1;i++) {
        if(!lstrcmp(current_file_buf,recent_names[i])) {
            recent_positions[i]=chrg.cpMin;
            break;
        }
    }

    if(i==RECENTFILES) {
        for(i=RECENTFILES-1;i>=1;i--) {
            memcpy(recent_names[i],recent_names[i-1],MAX_PATH);
            recent_positions[i]=recent_positions[i-1];
        }

        _tcscpy(recent_names[0],current_file_buf);

        recent_positions[0]=chrg.cpMin;
    }
    else {
        memcpy(name_buf,recent_names[i],MAX_PATH);
        pos_buf=recent_positions[i];
        for(j=i;j>=1;j--) {
            memcpy(recent_names[j],recent_names[j-1],MAX_PATH);
            recent_positions[j]=recent_positions[j-1];
        }
        memcpy(recent_names[0],name_buf,MAX_PATH);
        recent_positions[0]=pos_buf;
    }

    SaveLastFilePosition(hWndEdit);

    return TRUE;
}

BOOL SaveLastFilePosition(HWND hWndEdit) {
    HKEY hKey;
    DWORD dwDisp;
    int i;
    TCHAR namebuf[256];

    UNREFERENCED_PARAMETER(hWndEdit);

    if(RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\Akelsoft\\AkelPad\\Recent"),
        0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
        &hKey, &dwDisp)!=ERROR_SUCCESS) return FALSE;

    for(i=0;i<=RECENTFILES-1;i++) {
        _stprintf(namebuf, _T("name%d"), i);
        if(RegSetValueEx(hKey, namebuf, 0, REG_SZ,
            (LPBYTE)(recent_names[i]?recent_names[i]:_T("")),
            recent_names[i]?((_tcslen(recent_names[i])+1)*sizeof(TCHAR)):1)!=ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return FALSE;
        }

        _stprintf(namebuf, _T("pos%d"), i);
        if(RegSetValueEx(hKey, namebuf, 0, REG_DWORD,
            (LPBYTE)&recent_positions[i], sizeof(DWORD))!=ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return FALSE;
        }
    }

    RegCloseKey(hKey);
    return TRUE;
}

BOOL RestoreLastFilePosition(HWND hWndEdit) {
    HKEY hKey;
    DWORD dwType, dwSize=sizeof(DWORD);
    int i;
    TCHAR namebuf[256];

    UNREFERENCED_PARAMETER(hWndEdit);

    ZeroMemory(recent_names,MAX_PATH*RECENTFILES);
    ZeroMemory(recent_positions,RECENTFILES*sizeof(int));

    if(RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\Akelsoft\\AkelPad\\Recent"),
        0, KEY_ALL_ACCESS, &hKey)!=ERROR_SUCCESS)
    {
        return FALSE;
    }

    for(i=0;i<=RECENTFILES-1;i++) {
        _stprintf(namebuf, _T("name%d"), i);
        if(RegQueryValueEx(hKey, namebuf, NULL, &dwType,
            (LPBYTE)NULL, &dwSize)!=ERROR_SUCCESS) continue;

        if(dwType!=REG_SZ||dwSize<=1) break;

        RegQueryValueEx(hKey, namebuf, NULL, &dwType,
            (LPBYTE)recent_names[i], &dwSize);

        dwSize=sizeof(DWORD);
        _stprintf(namebuf, _T("pos%d"), i);

        RegQueryValueEx(hKey, namebuf, NULL, &dwType,
            (LPBYTE)&recent_positions[i], &dwSize);
    }
    RegCloseKey(hKey);

    RenewHistoryMenu();

    return TRUE;
}

int TranslateNewlinesToUnix(wchar_t *buffer,int iBufSize) {
    int rindex,windex;

    rindex=0;windex=0;

    while(rindex<=(iBufSize/2)-1) {
        buffer[windex]=buffer[rindex];
        if(buffer[rindex]!=0x0D) {
            windex++;
        }
        rindex++;
    }

    return windex*2;
}
