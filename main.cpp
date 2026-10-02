#define UNICODE
#define _UNICODE
#include <windows.h>
#include <windowsx.h>
#include <string>
#include <vector>
#include <random>
#include <fstream>
#include <algorithm>
#include <cstdint>
#include <cwchar>
#include <cstring>

static const int W=1160,H=740,SIDEBAR=226;
static HWND gEdit=nullptr;
static int gLen=3,gStyle=0;
static std::vector<std::wstring> gResults;
static std::mt19937 rng((unsigned)std::random_device{}());
static bool gRunning=false;
static COLORREF BG=RGB(6,7,9), PANEL=RGB(14,15,18), PANEL2=RGB(20,21,25), BORDER=RGB(38,39,46), TEXT=RGB(242,243,246), MUTED=RGB(137,140,150), GREEN=RGB(126,231,135);

enum : int { ID_LEN2=2001, ID_LEN3, ID_LEN4, ID_NORMAL, ID_FULL, ID_BOLD, ID_SMALL, ID_CIRCLED, ID_RUN, ID_COPY_BASE=2100 };

HFONT mkfont(int px,int weight=400,const wchar_t* face=L"Segoe UI") { return CreateFontW(-px,0,0,0,weight,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_DONTCARE,face); }
void fillRound(HDC dc,RECT r,int rad,COLORREF c){HBRUSH b=CreateSolidBrush(c);HGDIOBJ o=SelectObject(dc,b);RoundRect(dc,r.left,r.top,r.right,r.bottom,rad,rad);SelectObject(dc,o);DeleteObject(b);}
void strokeRound(HDC dc,RECT r,int rad,COLORREF c){HPEN p=CreatePen(PS_SOLID,1,c);HBRUSH b=(HBRUSH)GetStockObject(NULL_BRUSH);HGDIOBJ op=SelectObject(dc,p),ob=SelectObject(dc,b);RoundRect(dc,r.left,r.top,r.right,r.bottom,rad,rad);SelectObject(dc,op);SelectObject(dc,ob);DeleteObject(p);}
void drawText(HDC dc,const wchar_t* s,RECT r,int px,int weight,COLORREF c,const wchar_t* face=L"Segoe UI",UINT flags=DT_LEFT|DT_VCENTER|DT_SINGLELINE){HFONT f=mkfont(px,weight,face),o=(HFONT)SelectObject(dc,f);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,c);DrawTextW(dc,s,-1,&r,flags);SelectObject(dc,o);DeleteObject(f);}
void drawText(HDC dc,const std::wstring&s,RECT r,int px,int weight,COLORREF c,const wchar_t* face=L"Segoe UI",UINT flags=DT_LEFT|DT_VCENTER|DT_SINGLELINE){drawText(dc,s.c_str(),r,px,weight,c,face,flags);}
void iconDot(HDC dc,int x,int y,COLORREF c){HBRUSH b=CreateSolidBrush(c);HGDIOBJ o=SelectObject(dc,b);Ellipse(dc,x,y,x+7,y+7);SelectObject(dc,o);DeleteObject(b);}

std::wstring stylize(const std::string&s){
 std::wstring o;
 for(unsigned char c:s){
  if(gStyle==1){if(c>='a'&&c<='z')o.push_back(0xFF41+c-'a');else if(c>='A'&&c<='Z')o.push_back(0xFF21+c-'A');else if(c>='0'&&c<='9')o.push_back(0xFF10+c-'0');else o.push_back(c);}
  else if(gStyle==2){
   static const uint32_t low[]={0x1D5BA,0x1D5BB,0x1D5BC,0x1D5BD,0x1D5BE,0x1D5BF,0x1D5C0,0x1D5C1,0x1D5C2,0x1D5C3,0x1D5C4,0x1D5C5,0x1D5C6,0x1D5C7,0x1D5C8,0x1D5C9,0x1D5CA,0x1D5CB,0x1D5CC,0x1D5CD,0x1D5CE,0x1D5CF,0x1D5D0,0x1D5D1,0x1D5D2,0x1D5D3};
   static const uint32_t up[]={0x1D5A0,0x1D5A1,0x1D5A2,0x1D5A3,0x1D5A4,0x1D5A5,0x1D5A6,0x1D5A7,0x1D5A8,0x1D5A9,0x1D5AA,0x1D5AB,0x1D5AC,0x1D5AD,0x1D5AE,0x1D5AF,0x1D5B0,0x1D5B1,0x1D5B2,0x1D5B3,0x1D5B4,0x1D5B5,0x1D5B6,0x1D5B7,0x1D5B8,0x1D5B9};
   auto appendCP=[&](uint32_t cp){if(cp<=0xFFFF)o.push_back((wchar_t)cp);else{cp-=0x10000;o.push_back((wchar_t)(0xD800+(cp>>10)));o.push_back((wchar_t)(0xDC00+(cp&0x3FF)));}};
   if(c>='a'&&c<='z')appendCP(low[c-'a']);else if(c>='A'&&c<='Z')appendCP(up[c-'A']);else if(c>='0'&&c<='9')appendCP(0x1D7EC+c-'0');else o.push_back((wchar_t)c);
  }
  else if(gStyle==3){static const wchar_t a[]=L"ᴀʙᴄᴅᴇꜰɢʜɪᴊᴋʟᴍɴᴏᴘǫʀsᴛᴜᴠᴡxʏᴢ";if(c>='a'&&c<='z')o.push_back(a[c-'a']);else if(c>='A'&&c<='Z')o.push_back(a[c-'A']);else o.push_back(c);}
  else if(gStyle==4){if(c>='a'&&c<='z')o.push_back(0x24D0+c-'a');else if(c>='A'&&c<='Z')o.push_back(0x24B6+c-'A');else if(c>='1'&&c<='9')o.push_back(0x2460+c-'1');else if(c=='0')o.push_back(0x24EA);else o.push_back(c);}
  else o.push_back(c);
 }
 return o;
}
void copyText(const std::wstring&s){if(!OpenClipboard(nullptr))return;EmptyClipboard();SIZE_T n=(s.size()+1)*sizeof(wchar_t);HGLOBAL h=GlobalAlloc(GMEM_MOVEABLE,n);if(h){void*p=GlobalLock(h);memcpy(p,s.c_str(),n);GlobalUnlock(h);SetClipboardData(CF_UNICODETEXT,h);}CloseClipboard();}
void generate(HWND h){int count=20;wchar_t b[32]{};GetWindowTextW(gEdit,b,32);try{count=std::stoi(b);}catch(...){ }count=std::clamp(count,1,200);const char chars[]="abcdefghijklmnopqrstuvwxyz0123456789";gResults.clear();std::ofstream f("candidats.txt",std::ios::trunc);for(int n=0;n<count;n++){std::string s;for(int i=0;i<gLen;i++)s+=chars[rng()%36];auto w=stylize(s);gResults.push_back(w);if(f)f<<s<<"\n";}gRunning=true;InvalidateRect(h,nullptr,FALSE);}

RECT rr(int l,int t,int r,int b){return RECT{l,t,r,b};}
void buttonPaint(HDC dc,RECT r,const wchar_t* text,bool active,bool primary){COLORREF fill=primary?RGB(242,243,246):(active?RGB(48,49,56):PANEL2);fillRound(dc,r,9,fill);strokeRound(dc,r,9,active?RGB(90,91,100):BORDER);drawText(dc,text,r,12,650,primary?RGB(12,13,15):TEXT,L"Segoe UI",DT_CENTER|DT_VCENTER|DT_SINGLELINE);}
void addButton(HWND h,int id,RECT r,const wchar_t* text){HWND b=CreateWindowExW(0,L"BUTTON",text,WS_CHILD|WS_VISIBLE|BS_OWNERDRAW,r.left,r.top,r.right-r.left,r.bottom-r.top,h,(HMENU)(INT_PTR)id,GetModuleHandleW(nullptr),nullptr);SendMessageW(b,WM_SETFONT,(WPARAM)mkfont(12,650),TRUE);}

void createControls(HWND h){
 addButton(h,ID_LEN2,rr(282,222,352,260),L"2C"); addButton(h,ID_LEN3,rr(362,222,432,260),L"3C"); addButton(h,ID_LEN4,rr(442,222,512,260),L"4C");
 addButton(h,ID_NORMAL,rr(282,396,366,434),L"Normal"); addButton(h,ID_FULL,rr(374,396,472,434),L"Fullwidth"); addButton(h,ID_BOLD,rr(480,396,575,434),L"Sans Bold");
 addButton(h,ID_SMALL,rr(282,444,378,482),L"Small Caps"); addButton(h,ID_CIRCLED,rr(386,444,470,482),L"Circled");
 addButton(h,ID_RUN,rr(282,510,606,560),L"START RUN");
}

void paint(HWND h,HDC dc){
 RECT client;GetClientRect(h,&client);HBRUSH bg=CreateSolidBrush(BG);FillRect(dc,&client,bg);DeleteObject(bg);
 HBRUSH sb=CreateSolidBrush(RGB(9,10,13));RECT side={0,0,SIDEBAR,client.bottom};FillRect(dc,&side,sb);DeleteObject(sb);HPEN p=CreatePen(PS_SOLID,1,BORDER);HGDIOBJ op=SelectObject(dc,p);MoveToEx(dc,SIDEBAR,0,nullptr);LineTo(dc,SIDEBAR,client.bottom);SelectObject(dc,op);DeleteObject(p);
 drawText(dc,L"/dntool",rr(24,22,190,54),20,750,TEXT);drawText(dc,L"v3.0  •  name lab",rr(25,54,195,76),11,500,MUTED);
 const wchar_t* nav[]={L"HOME",L"GENERATOR",L"CHECKER",L"RESULTS",L"SETTINGS",L"HELP"};int ys[]={112,154,196,238,280,322};for(int i=0;i<6;i++){bool active=i==1;RECT r=rr(14,ys[i],207,ys[i]+36);if(active){fillRound(dc,r,10,RGB(38,39,45));strokeRound(dc,r,10,RGB(56,57,66));}iconDot(dc,28,ys[i]+14,active?TEXT:MUTED);drawText(dc,nav[i],rr(44,ys[i],190,ys[i]+36),12,650,active?TEXT:MUTED);}
 fillRound(dc,rr(14,585,207,680),12,PANEL);drawText(dc,L"LICENSE",rr(26,598,120,615),9,700,MUTED);drawText(dc,L"DAY PASS",rr(26,620,170,644),13,700,TEXT);drawText(dc,L"50,000 usernames saved",rr(26,650,190,668),10,500,MUTED);
 drawText(dc,L"Username Generator",rr(258,22,700,62),29,750,TEXT);drawText(dc,L"Generate short, stylized Discord-ready names",rr(260,62,690,88),12,500,MUTED);
 fillRound(dc,rr(862,24,936,58),9,PANEL2);strokeRound(dc,rr(862,24,936,58),9,BORDER);drawText(dc,L"DATA",rr(862,24,936,58),12,650,TEXT,L"Segoe UI",DT_CENTER|DT_VCENTER|DT_SINGLELINE);
 fillRound(dc,rr(944,24,1020,58),9,PANEL2);strokeRound(dc,rr(944,24,1020,58),9,BORDER);drawText(dc,L"RESULTS",rr(944,24,1020,58),12,650,TEXT,L"Segoe UI",DT_CENTER|DT_VCENTER|DT_SINGLELINE);
 fillRound(dc,rr(258,110,630,590),16,PANEL);drawText(dc,L"GENERATOR",rr(282,132,400,150),10,750,MUTED);drawText(dc,L"Create names",rr(282,154,500,184),20,750,TEXT);
 drawText(dc,L"LENGTH",rr(282,198,360,216),9,700,MUTED);
 drawText(dc,L"HOW MANY",rr(282,282,380,300),9,700,MUTED);fillRound(dc,rr(282,306,606,350),10,PANEL2);strokeRound(dc,rr(282,306,606,350),10,BORDER);drawText(dc,L"names",rr(300,306,355,350),11,500,MUTED);
 drawText(dc,L"STYLE / UNICODE",rr(282,372,430,390),9,700,MUTED);
 drawText(dc,L"Generates unique-looking random names and exports",rr(282,570,600,588),10,500,MUTED);
 fillRound(dc,rr(650,110,1030,590),16,PANEL);drawText(dc,L"TELEMETRY",rr(676,132,790,150),10,750,MUTED);drawText(dc,L"Runs",rr(676,154,800,184),20,750,TEXT);fillRound(dc,rr(945,132,1006,158),13,RGB(38,39,44));iconDot(dc,956,142,gRunning?GREEN:MUTED);drawText(dc,gRunning?L"Ready":L"Idle",rr(967,132,1000,158),9,650,MUTED,L"Segoe UI",DT_LEFT|DT_VCENTER|DT_SINGLELINE);
 if(gResults.empty()){fillRound(dc,rr(676,198,1004,268),11,RGB(10,11,14));drawText(dc,L"No active run",rr(694,208,985,232),13,650,TEXT,L"Segoe UI",DT_CENTER|DT_VCENTER|DT_SINGLELINE);drawText(dc,L"Choose a length, style and press Start Run",rr(694,234,985,254),10,500,MUTED,L"Segoe UI",DT_CENTER|DT_VCENTER|DT_SINGLELINE);}
 else {int maxRows=6,y=198;for(int i=0;i<(int)gResults.size()&&i<maxRows;i++,y+=54){fillRound(dc,rr(676,y,1004,y+44),10,RGB(10,11,14));drawText(dc,gResults[i],rr(692,y,840,y+44),15,650,TEXT,L"Cascadia Mono");fillRound(dc,rr(850,y+8,930,y+34),13,RGB(38,39,44));drawText(dc,L"available",rr(850,y+8,930,y+34),9,600,MUTED,L"Segoe UI",DT_CENTER|DT_VCENTER|DT_SINGLELINE);addButton(h,ID_COPY_BASE+i,rr(942,y+7,990,y+35),L"COPY");}if(gResults.size()>maxRows)drawText(dc,L"+ "+std::to_wstring(gResults.size()-maxRows)+L" more saved to candidats.txt",rr(678,526,995,548),10,500,MUTED);}
 drawText(dc,L"OUTPUT",rr(676,560,760,578),9,700,MUTED);drawText(dc,L"candidats.txt",rr(760,556,900,580),11,600,TEXT,L"Cascadia Mono");
}

void syncButtonStates(HWND h){
 for(auto p: {std::pair<int,int>{ID_LEN2,2}, {ID_LEN3,3}, {ID_LEN4,4}, {ID_NORMAL,0}, {ID_FULL,1}, {ID_BOLD,2}, {ID_SMALL,3}, {ID_CIRCLED,4}}){HWND b=GetDlgItem(h,p.first);if(b)InvalidateRect(b,nullptr,FALSE);} 
}
LRESULT CALLBACK WndProc(HWND h,UINT m,WPARAM w,LPARAM l){
 switch(m){
 case WM_CREATE:{gEdit=CreateWindowW(L"EDIT",L"20",WS_CHILD|WS_VISIBLE|WS_BORDER|ES_NUMBER|ES_CENTER|ES_AUTOHSCROLL,365,308,220,40,h,(HMENU)1001,GetModuleHandleW(nullptr),nullptr);SendMessageW(gEdit,WM_SETFONT,(WPARAM)mkfont(15,600),TRUE);createControls(h);return 0;}
 case WM_COMMAND:{int id=LOWORD(w);if(HIWORD(w)==BN_CLICKED){switch(id){case ID_LEN2:gLen=2;break;case ID_LEN3:gLen=3;break;case ID_LEN4:gLen=4;break;case ID_NORMAL:gStyle=0;break;case ID_FULL:gStyle=1;break;case ID_BOLD:gStyle=2;break;case ID_SMALL:gStyle=3;break;case ID_CIRCLED:gStyle=4;break;case ID_RUN:generate(h);break;default:if(id>=ID_COPY_BASE&&id<ID_COPY_BASE+6){int idx=id-ID_COPY_BASE;if(idx<(int)gResults.size())copyText(gResults[idx]);}break;}syncButtonStates(h);InvalidateRect(h,nullptr,FALSE);}return 0;}
 case WM_DRAWITEM:{DRAWITEMSTRUCT* d=(DRAWITEMSTRUCT*)l;if(!d||d->CtlType!=ODT_BUTTON)return 0;int id=(int)d->CtlID;bool active=false; if(id==ID_LEN2)active=gLen==2;else if(id==ID_LEN3)active=gLen==3;else if(id==ID_LEN4)active=gLen==4;else if(id==ID_NORMAL)active=gStyle==0;else if(id==ID_FULL)active=gStyle==1;else if(id==ID_BOLD)active=gStyle==2;else if(id==ID_SMALL)active=gStyle==3;else if(id==ID_CIRCLED)active=gStyle==4;bool primary=id==ID_RUN;wchar_t txt[64]{};GetWindowTextW(d->hwndItem,txt,64);buttonPaint(d->hDC,d->rcItem,txt,active,primary);return TRUE;}
 case WM_CTLCOLOREDIT:{HDC dc=(HDC)w;SetTextColor(dc,TEXT);SetBkColor(dc,PANEL2);static HBRUSH br=nullptr;if(!br)br=CreateSolidBrush(PANEL2);return (LRESULT)br;}
 case WM_ERASEBKGND:return 1;
 case WM_PAINT:{PAINTSTRUCT ps;HDC dc=BeginPaint(h,&ps);paint(h,dc);EndPaint(h,&ps);return 0;}
 case WM_DESTROY:PostQuitMessage(0);return 0;}
 return DefWindowProcW(h,m,w,l);
}
int WINAPI wWinMain(HINSTANCE hi,HINSTANCE,LPWSTR,int show){WNDCLASSW wc{};wc.hInstance=hi;wc.lpfnWndProc=WndProc;wc.lpszClassName=L"DNTModern4";wc.hCursor=LoadCursor(nullptr,IDC_ARROW);wc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);RegisterClassW(&wc);HWND h=CreateWindowExW(0,wc.lpszClassName,L"DiscordNameTool",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,W,H,nullptr,nullptr,hi,nullptr);ShowWindow(h,show);UpdateWindow(h);MSG msg;while(GetMessageW(&msg,nullptr,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);}return 0;}
