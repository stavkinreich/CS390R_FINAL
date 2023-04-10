/* Copyright 2022 the SumatraPDF project authors (see AUTHORS file).
   License: GPLv3 */

#include "utils/BaseUtil.h"
#include "utils/ScopedWin.h"
#include "utils/FileUtil.h"
#include "utils/FileWatcher.h"
#include "utils/WinUtil.h"

#include "wingui/UIModels.h"

#include "Settings.h"
#include "DocController.h"
#include "EngineBase.h"
#include "EngineAll.h"
#include "GlobalPrefs.h"
#include "ChmModel.h"
#include "DisplayModel.h"
#include "SumatraPDF.h"
#include "MainWindow.h"
#include "WindowTab.h"
#include "Selection.h"
#include "Translations.h"
#include "EditAnnotations.h"

WindowTab::WindowTab(MainWindow* win) {
    this->win = win;
}

void WindowTab::SetFilePath(const char* path) {
    type = Type::Document;
    this->filePath.SetCopy(path);
}

bool WindowTab::IsAboutTab() const {
    CrashIf(type == WindowTab::Type::None);
    return type == WindowTab::Type::About;
}

WindowTab::~WindowTab() {
    FileWatcherUnsubscribe(watcher);
    if (AsChm()) {
        AsChm()->RemoveParentHwnd();
    }
    delete selectionOnPage;
    delete ctrl;
    CloseAndDeleteEditAnnotationsWindow(editAnnotsWindow);
}

bool WindowTab::IsDocLoaded() const {
    return ctrl != nullptr;
}

DisplayModel* WindowTab::AsFixed() const {
    return ctrl ? ctrl->AsFixed() : nullptr;
}

ChmModel* WindowTab::AsChm() const {
    return ctrl ? ctrl->AsChm() : nullptr;
}

Kind WindowTab::GetEngineType() const {
    if (ctrl && ctrl->AsFixed()) {
        return ctrl->AsFixed()->GetEngine()->kind;
    }
    return nullptr;
}

EngineBase* WindowTab::GetEngine() const {
    if (ctrl && ctrl->AsFixed()) {
        return ctrl->AsFixed()->GetEngine();
    }
    return nullptr;
}

// can be null for About tab
const char* WindowTab::GetPath() const {
    return this->filePath;
}

const char* WindowTab::GetTabTitle() const {
    if (gGlobalPrefs->fullPathInTitle) {
        return filePath;
    }
    return path::GetBaseNameTemp(filePath);
}

void WindowTab::MoveDocBy(int dx, int dy) const {
    if (!ctrl) {
        return;
    }
    DisplayModel* dm = ctrl->AsFixed();
    CrashIf(!dm);
    if (!dm) {
        return;
    }
    CrashIf(win->linkOnLastButtonDown);
    if (win->linkOnLastButtonDown) {
        return;
    }
    if (0 != dx) {
        dm->ScrollXBy(dx);
    }
    if (0 != dy) {
        dm->ScrollYBy(dy, false);
    }
}

void WindowTab::ToggleZoom() const {
    CrashIf(!ctrl);
    if (!IsDocLoaded()) {
        return;
    }
    // TODO: maybe move to DocController?
    float newZoom = kZoomFitPage;
    float currZoom = ctrl->GetZoomVirtual();
    if (kZoomFitPage == currZoom) {
        newZoom = kZoomFitWidth;
    } else if (kZoomFitWidth == currZoom) {
        newZoom = kZoomFitContent;
    }
    ctrl->SetZoomVirtual(newZoom, nullptr);
}

// https://github.com/sumatrapdfreader/sumatrapdf/issues/1336
#if 0
LinkSaver::LinkSaver(WindowTab* tab, HWND parentHwnd, const WCHAR* fileName) {
    this->tab = tab;
    this->parentHwnd = parentHwnd;
    this->fileName = fileName;
}
#endif

bool SaveDataToFile(HWND hwndParent, char* fileNameA, ByteSlice data) {
    if (!HasPermission(Perm::DiskAccess)) {
        return false;
    }

    WCHAR dstFileName[MAX_PATH] = {0};
    if (fileNameA) {
        str::BufSet(dstFileName, dimof(dstFileName), fileNameA);
    }
    // CrashIf(fileName && str::FindChar(fileName, '/'));

    // Prepare the file filters (use \1 instead of \0 so that the
    // double-zero terminated string isn't cut by the string handling
    // methods too early on)
    AutoFreeWstr fileFilter = str::Format(L"%s\1*.*\1", _TR("All files"));
    str::TransCharsInPlace(fileFilter, L"\1", L"\0");

    OPENFILENAME ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwndParent;
    ofn.lpstrFile = dstFileName;
    ofn.nMaxFile = dimof(dstFileName);
    ofn.lpstrFilter = fileFilter;
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;

    bool ok = GetSaveFileNameW(&ofn);
    if (!ok) {
        return false;
    }
    char* path = ToUtf8Temp(dstFileName);
    ok = file::WriteFile(path, data);
    // https://github.com/sumatrapdfreader/sumatrapdf/issues/1336
#if 0
    if (ok && tab && IsUntrustedFile(tab->filePath, gPluginURL)) {
        file::SetZoneIdentifier(dstFileName);
    }
#endif
    return ok;
}
