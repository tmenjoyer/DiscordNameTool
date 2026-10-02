#define UNICODE
#define _UNICODE
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#pragma comment(lib, "comctl32.lib")

#define ID_2C       101
#define ID_3C       102
#define ID_4C       103
#define ID_GENERATE 104
#define ID_COUNT    105
#define ID_LIST     106
#define ID_STATUS   107
#define ID_STYLE1   108
#define ID_STYLE2   109
#define ID_STYLE3   110
#define ID_STYLE4   111

static HWND hCount, hList, hStatus, hStyleInfo;
static int selectedLen = 3;
static int selectedStyle = 0;

static const char ALPHANUM[] = "abcdefghijklmnopqrstuvwxyz0123456789";

static HFONT make_font(int size, int weight, const wchar_t *face) {
    return CreateFontW(size, 0, 0, 0, weight, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, face);
}

static void set_font(HWND hwnd, HFONT font) {
    SendMessageW(hwnd, WM_SETFONT, (WPARAM)font, TRUE);
}

static void add_cp(wchar_t *out, int *p, unsigned int cp, int cap) {
    if (cp <= 0xFFFF) {
        if (*p < cap - 1) out[(*p)++] = (wchar_t)cp;
    } else if (cp <= 0x10FFFF && *p < cap - 2) {
        cp -= 0x10000;
        out[(*p)++] = (wchar_t)(0xD800 + (cp >> 10));
        out[(*p)++] = (wchar_t)(0xDC00 + (cp & 0x3FF));
    }
    out[*p] = L'\0';
}

static unsigned int math_sans_bold(unsigned int cp) {
    if (cp >= 'a' && cp <= 'z') return 0x1D5A0 + (cp - 'a');
    if (cp >= 'A' && cp <= 'Z') return 0x1D5D4 + (cp - 'A');
    if (cp >= '0' && cp <= '9') return 0x1D7EC + (cp - '0');
    return cp;
}

static void stylize_name(const char *name, int style, wchar_t *out, int cap) {
    int p = 0;
    out[0] = L'\0';
    for (int i = 0; name[i] && p < cap - 3; ++i) {
        unsigned char c = (unsigned char)name[i];
        if (style == 0) {
            add_cp(out, &p, c, cap);
        } else if (style == 1) {
            if (c >= 'a' && c <= 'z') add_cp(out, &p, 0xFF41 + (c - 'a'), cap);
            else if (c >= 'A' && c <= 'Z') add_cp(out, &p, 0xFF21 + (c - 'A'), cap);
            else if (c >= '0' && c <= '9') add_cp(out, &p, 0xFF10 + (c - '0'), cap);
            else add_cp(out, &p, c, cap);
        } else if (style == 2) {
            add_cp(out, &p, math_sans_bold(c), cap);
        } else if (style == 3) {
            /* Small-caps / TikTok-style look using Unicode small letters where available. */
            static const wchar_t small[] = L"ᴀʙᴄᴅᴇꜰɢʜɪᴊᴋʟᴍɴᴏᴘǫʀsᴛᴜᴠᴡxʏᴢ";
            if (c >= 'a' && c <= 'z') add_cp(out, &p, small[c - 'a'], cap);
            else if (c >= 'A' && c <= 'Z') add_cp(out, &p, small[c - 'A'], cap);
            else add_cp(out, &p, c, cap);
        } else {
            /* Circled letters/numbers. */
            if (c >= 'a' && c <= 'z') add_cp(out, &p, 0x24D0 + (c - 'a'), cap);
            else if (c >= 'A' && c <= 'Z') add_cp(out, &p, 0x24B6 + (c - 'A'), cap);
            else if (c >= '1' && c <= '9') add_cp(out, &p, 0x2460 + (c - '1'), cap);
            else if (c == '0') add_cp(out, &p, 0x24EA, cap);
            else add_cp(out, &p, c, cap);
        }
    }
}

static const wchar_t *style_name(void) {
    switch (selectedStyle) {
        case 1: return L"Fullwidth";
        case 2: return L"Sans bold";
        case 3: return L"Small caps";
        case 4: return L"Circled";
        default: return L"Normal";
    }
}

static void generate_names(int len, int amount) {
    FILE *f = fopen("candidats.txt", "w");
    if (!f) {
        MessageBoxW(NULL, L"Impossible de créer candidats.txt.", L"Erreur", MB_ICONERROR);
        return;
    }

    SendMessageW(hList, LB_RESETCONTENT, 0, 0);
    srand((unsigned int)time(NULL));

    for (int i = 0; i < amount; ++i) {
        char name[8] = {0};
        for (int j = 0; j < len; ++j)
            name[j] = ALPHANUM[rand() % (sizeof(ALPHANUM) - 1)];

        fprintf(f, "%s\n", name);

        wchar_t styled[64];
        stylize_name(name, selectedStyle, styled, 64);
        SendMessageW(hList, LB_ADDSTRING, 0, (LPARAM)styled);
    }

    fclose(f);

    wchar_t status[160];
    swprintf(status, 160, L"%d pseudos générés  •  %dC  •  style : %ls  •  candidats.txt",
             amount, len, style_name());
    SetWindowTextW(hStatus, status);
}

static void set_selected_button(HWND hwnd) {
    (void)hwnd;
    InvalidateRect(hwnd, NULL, TRUE);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HFONT fontNormal, fontSmall, fontTitle, fontHuge, fontButton, fontMono;

    switch (msg) {
    case WM_CREATE: {
        INITCOMMONCONTROLSEX ic = { sizeof(ic), ICC_STANDARD_CLASSES };
        InitCommonControlsEx(&ic);

        fontNormal = make_font(17, FW_NORMAL, L"Segoe UI");
        fontSmall  = make_font(14, FW_NORMAL, L"Segoe UI");
        fontTitle  = make_font(28, FW_SEMIBOLD, L"Segoe UI");
        fontHuge   = make_font(22, FW_SEMIBOLD, L"Segoe UI");
        fontButton = make_font(16, FW_SEMIBOLD, L"Segoe UI");
        fontMono   = make_font(17, FW_NORMAL, L"Cascadia Mono");

        /* Sidebar */
        HWND brand = CreateWindowW(L"STATIC", L"/dntool", WS_CHILD|WS_VISIBLE,
            24, 22, 170, 34, hwnd, NULL, NULL, NULL);
        set_font(brand, fontHuge);
        HWND version = CreateWindowW(L"STATIC", L"v2.0  •  Discord names", WS_CHILD|WS_VISIBLE,
            25, 55, 180, 22, hwnd, NULL, NULL, NULL);
        set_font(version, fontSmall);

        HWND nav1 = CreateWindowW(L"STATIC", L"⌂   Home", WS_CHILD|WS_VISIBLE,
            25, 115, 175, 34, hwnd, NULL, NULL, NULL); set_font(nav1, fontNormal);
        HWND nav2 = CreateWindowW(L"STATIC", L"✦   Generator", WS_CHILD|WS_VISIBLE,
            25, 157, 175, 34, hwnd, NULL, NULL, NULL); set_font(nav2, fontNormal);
        HWND nav3 = CreateWindowW(L"STATIC", L"▣   Styles", WS_CHILD|WS_VISIBLE,
            25, 199, 175, 34, hwnd, NULL, NULL, NULL); set_font(nav3, fontNormal);
        HWND nav4 = CreateWindowW(L"STATIC", L"☰   Results", WS_CHILD|WS_VISIBLE,
            25, 241, 175, 34, hwnd, NULL, NULL, NULL); set_font(nav4, fontNormal);

        HWND tip = CreateWindowW(L"STATIC", L"UNICODE FONTS\nTikTok-style names\n2C / 3C / 4C",
            WS_CHILD|WS_VISIBLE, 25, 505, 175, 70, hwnd, NULL, NULL, NULL);
        set_font(tip, fontSmall);

        /* Main header */
        HWND title = CreateWindowW(L"STATIC", L"Username Generator", WS_CHILD|WS_VISIBLE,
            235, 28, 500, 42, hwnd, NULL, NULL, NULL); set_font(title, fontTitle);
        HWND sub = CreateWindowW(L"STATIC", L"Génère des pseudos courts avec des écritures stylisées.",
            WS_CHILD|WS_VISIBLE, 237, 70, 600, 28, hwnd, NULL, NULL, NULL); set_font(sub, fontNormal);

        HWND card1 = CreateWindowW(L"STATIC", L"GENERATOR", WS_CHILD|WS_VISIBLE,
            235, 120, 400, 30, hwnd, NULL, NULL, NULL); set_font(card1, fontSmall);

        HWND len = CreateWindowW(L"STATIC", L"Longueur", WS_CHILD|WS_VISIBLE,
            235, 157, 120, 25, hwnd, NULL, NULL, NULL); set_font(len, fontSmall);

        HWND b2 = CreateWindowW(L"BUTTON", L"2C", WS_CHILD|WS_VISIBLE|BS_OWNERDRAW,
            235, 188, 90, 45, hwnd, (HMENU)ID_2C, NULL, NULL);
        HWND b3 = CreateWindowW(L"BUTTON", L"3C", WS_CHILD|WS_VISIBLE|BS_OWNERDRAW,
            335, 188, 90, 45, hwnd, (HMENU)ID_3C, NULL, NULL);
        HWND b4 = CreateWindowW(L"BUTTON", L"4C", WS_CHILD|WS_VISIBLE|BS_OWNERDRAW,
            435, 188, 90, 45, hwnd, (HMENU)ID_4C, NULL, NULL);
        set_font(b2, fontButton); set_font(b3, fontButton); set_font(b4, fontButton);

        HWND amount = CreateWindowW(L"STATIC", L"Nombre", WS_CHILD|WS_VISIBLE,
            235, 251, 120, 25, hwnd, NULL, NULL, NULL); set_font(amount, fontSmall);
        hCount = CreateWindowW(L"EDIT", L"100", WS_CHILD|WS_VISIBLE|WS_BORDER|ES_NUMBER,
            235, 280, 290, 42, hwnd, (HMENU)ID_COUNT, NULL, NULL); set_font(hCount, fontMono);

        HWND gen = CreateWindowW(L"BUTTON", L"Générer les pseudos  →",
            WS_CHILD|WS_VISIBLE|BS_OWNERDRAW, 235, 340, 290, 50, hwnd,
            (HMENU)ID_GENERATE, NULL, NULL); set_font(gen, fontButton);

        HWND style = CreateWindowW(L"STATIC", L"ÉCRITURE / STYLE", WS_CHILD|WS_VISIBLE,
            235, 410, 250, 25, hwnd, NULL, NULL); set_font(style, fontSmall);

        const wchar_t *labels[] = {L"Normal", L"Ｆｕｌｌｗｉｄｔｈ", L"𝙎𝙖𝙣𝙨 𝙗𝙤𝙡𝙙", L"ᴍɪɴɪ ᴄᴀᴘs", L"ⓒⓘⓡⓒⓛⓔⓓ"};
        int ids[] = {ID_STYLE1, ID_STYLE2, ID_STYLE3, ID_STYLE4};
        int xs[] = {235, 305, 395, 505};
        for (int i = 0; i < 4; ++i) {
            HWND b = CreateWindowW(L"BUTTON", labels[i+1], WS_CHILD|WS_VISIBLE|BS_OWNERDRAW,
                xs[i], 440, i == 0 ? 65 : 95, 38, hwnd, (HMENU)ids[i], NULL, NULL);
            set_font(b, fontSmall);
        }
        hStyleInfo = CreateWindowW(L"STATIC", L"Style : Normal", WS_CHILD|WS_VISIBLE,
            600, 440, 200, 30, hwnd, NULL, NULL, NULL); set_font(hStyleInfo, fontSmall);

        HWND result = CreateWindowW(L"STATIC", L"LIVE RESULTS", WS_CHILD|WS_VISIBLE,
            660, 120, 250, 30, hwnd, NULL, NULL, NULL); set_font(result, fontSmall);
        hList = CreateWindowW(L"LISTBOX", NULL,
            WS_CHILD|WS_VISIBLE|WS_BORDER|WS_VSCROLL|LBS_NOINTEGRALHEIGHT,
            660, 160, 300, 365, hwnd, (HMENU)ID_LIST, NULL, NULL);
        set_font(hList, fontMono);
        SendMessageW(hList, LB_SETHORIZONTALEXTENT, 500, 0);

        hStatus = CreateWindowW(L"STATIC", L"Prêt  •  sélection actuelle : 3C",
            WS_CHILD|WS_VISIBLE, 235, 535, 725, 28, hwnd, (HMENU)ID_STATUS, NULL, NULL);
        set_font(hStatus, fontSmall);

        return 0;
    }

    case WM_DRAWITEM: {
        DRAWITEMSTRUCT *d = (DRAWITEMSTRUCT*)lParam;
        if (!d) break;
        wchar_t text[128] = L"";
        GetWindowTextW(d->hwndItem, text, 128);
        HBRUSH bg = CreateSolidBrush(RGB(32,32,38));
        HBRUSH active = CreateSolidBrush(RGB(238,238,242));
        HBRUSH hover = CreateSolidBrush(RGB(48,48,56));
        BOOL isMain = (d->itemID == ID_GENERATE);
        BOOL selected = (d->hwndItem && ((GetDlgCtrlID(d->hwndItem) == ID_3C && selectedLen == 3) ||
                         (GetDlgCtrlID(d->hwndItem) == ID_2C && selectedLen == 2) ||
                         (GetDlgCtrlID(d->hwndItem) == ID_4C && selectedLen == 4) ||
                         (GetDlgCtrlID(d->hwndItem) >= ID_STYLE1 && GetDlgCtrlID(d->hwndItem) <= ID_STYLE4 &&
                          selectedStyle == GetDlgCtrlID(d->hwndItem)-ID_STYLE1+1))));
        FillRect(d->hDC, &d->rcItem, (isMain || selected) ? active : bg);
        SetBkMode(d->hDC, TRANSPARENT);
        SetTextColor(d->hDC, (isMain || selected) ? RGB(12,12,15) : RGB(235,235,240));
        DrawTextW(d->hDC, text, -1, &d->rcItem, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        DeleteObject(bg); DeleteObject(active); DeleteObject(hover);
        return TRUE;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_2C: selectedLen = 2; SetWindowTextW(hStatus, L"Longueur sélectionnée : 2C"); InvalidateRect(hwnd,NULL,TRUE); break;
        case ID_3C: selectedLen = 3; SetWindowTextW(hStatus, L"Longueur sélectionnée : 3C"); InvalidateRect(hwnd,NULL,TRUE); break;
        case ID_4C: selectedLen = 4; SetWindowTextW(hStatus, L"Longueur sélectionnée : 4C"); InvalidateRect(hwnd,NULL,TRUE); break;
        case ID_STYLE1: selectedStyle = 1; SetWindowTextW(hStyleInfo, L"Style : Fullwidth"); InvalidateRect(hwnd,NULL,TRUE); break;
        case ID_STYLE2: selectedStyle = 2; SetWindowTextW(hStyleInfo, L"Style : Sans bold"); InvalidateRect(hwnd,NULL,TRUE); break;
        case ID_STYLE3: selectedStyle = 3; SetWindowTextW(hStyleInfo, L"Style : Small caps"); InvalidateRect(hwnd,NULL,TRUE); break;
        case ID_STYLE4: selectedStyle = 4; SetWindowTextW(hStyleInfo, L"Style : Circled"); InvalidateRect(hwnd,NULL,TRUE); break;
        case ID_GENERATE: {
            wchar_t buffer[32]; GetWindowTextW(hCount, buffer, 32);
            int amount = _wtoi(buffer);
            if (amount < 1 || amount > 1000) {
                MessageBoxW(hwnd, L"Entre un nombre entre 1 et 1000.", L"Nombre invalide", MB_ICONWARNING);
                break;
            }
            generate_names(selectedLen, amount);
            break;
        }
        }
        return 0;

    case WM_CTLCOLORSTATIC:
        SetBkColor((HDC)wParam, RGB(14,14,17));
        SetTextColor((HDC)wParam, RGB(235,235,240));
        return (LRESULT)GetStockObject(NULL_BRUSH);

    case WM_CTLCOLOREDIT:
        SetBkColor((HDC)wParam, RGB(28,28,34));
        SetTextColor((HDC)wParam, RGB(245,245,250));
        return (LRESULT)GetStockObject(NULL_BRUSH);

    case WM_CTLCOLORLISTBOX:
        SetBkColor((HDC)wParam, RGB(22,22,27));
        SetTextColor((HDC)wParam, RGB(235,235,240));
        return (LRESULT)GetStockObject(NULL_BRUSH);

    case WM_ERASEBKGND: {
        RECT r; GetClientRect(hwnd, &r);
        HDC dc = (HDC)wParam;
        HBRUSH bg = CreateSolidBrush(RGB(14,14,17));
        FillRect(dc, &r, bg);
        DeleteObject(bg);
        RECT side = {0,0,215,r.bottom};
        HBRUSH sb = CreateSolidBrush(RGB(10,10,12));
        FillRect(dc, &side, sb);
        DeleteObject(sb);
        RECT line = {214,0,216,r.bottom};
        HBRUSH lb = CreateSolidBrush(RGB(38,38,44));
        FillRect(dc, &line, lb);
        DeleteObject(lb);
        return 1;
    }

    case WM_DESTROY:
        PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrev, PWSTR lpCmdLine, int nCmdShow) {
    (void)hPrev; (void)lpCmdLine;
    const wchar_t CLASS_NAME[] = L"DiscordNameToolWindow";
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = WndProc; wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME; wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(0, CLASS_NAME, L"OGTool • Username Generator",
        WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 1000, 640, NULL, NULL, hInstance, NULL);
    if (!hwnd) return 1;
    ShowWindow(hwnd, nCmdShow); UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg); DispatchMessageW(&msg);
    }
    return 0;
}
