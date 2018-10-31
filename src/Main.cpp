// This file is part of NppLsp.
// 
// Copyright (C)2018 Justin Dailey <dail8859@yahoo.com>
// 
// NppLsp is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "AboutDialog.h"
#include "resource.h"
#include "npp\PluginInterface.h"
#include "ScintillaGateway.h"
#include "NppGateway.h"

#include "LspClient.h"

#include <algorithm>
#include <vector>
#include <unordered_map>


using namespace nlohmann;


static HANDLE _hModule;
static NppGateway npp;
static ScintillaGateway editor;

static void GotoDefiniton();
static void ShowAbout();

static FuncItem funcItem[] = {
	{ L"Goto Definiton", GotoDefiniton, 0, false, nullptr },
	{ L"", nullptr, 0, false, nullptr },
	{ L"About...", ShowAbout, 0, false, nullptr }
};

static const wchar_t *GetIniFilePath() {
	static wchar_t iniPath[MAX_PATH] = {0};

	if (iniPath[0] == 0) {
		npp.GetPluginsConfigDir(MAX_PATH, iniPath);
		wcscat_s(iniPath, MAX_PATH, L"\\NppLsp.ini");
	}

	return iniPath;
}



std::unordered_map<BufferID, LspClient*> clients;
LspClient *current_client = nullptr;

static void GotoDefiniton() {
	auto range = current_client->requestDefinition(editor.GetCurrentPos());

	int s = editor.PositionFromLine(range["start"]["line"]) + range["start"]["character"];
	int e = editor.PositionFromLine(range["end"]["line"]) + range["end"]["character"];

	editor.SetSel(s, e);
}

static void Autocompletion() {
	auto j = current_client->requestCompletion(editor.GetCurrentPos());
}

static void ShowAbout() {
	ShowAboutDialog((HINSTANCE)_hModule, MAKEINTRESOURCE(IDD_ABOUTDLG), npp.data._nppHandle);
}

BOOL APIENTRY DllMain(HANDLE hModule, DWORD reasonForCall, LPVOID lpReserved) {
	switch (reasonForCall) {
		case DLL_PROCESS_ATTACH:
			_hModule = hModule;
			break;
		case DLL_PROCESS_DETACH:
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
	}
	return TRUE;
}

extern "C" __declspec(dllexport) void setInfo(NppData notepadPlusData) {
	// Set these as early as possible so it is in a valid state
	npp.SetNppData(notepadPlusData);
	editor.SetScintillaInstance(notepadPlusData._scintillaMainHandle);
}

extern "C" __declspec(dllexport) const wchar_t *getName() {
	return L"NppLsp";
}

extern "C" __declspec(dllexport) FuncItem *getFuncsArray(int *nbF) {
	*nbF = sizeof(funcItem) / sizeof(funcItem[0]);
	return funcItem;
}

extern "C" __declspec(dllexport) void beNotified(SCNotification *notifyCode) {
	switch (notifyCode->nmhdr.code) {
		case SCN_DWELLSTART:
			if (current_client) {
				std::string contents = current_client->requestHover(notifyCode->position)["contents"].get<std::string>();
				if (contents.length() > 512) {
					contents.resize(contents.find('\n', 512));
					contents.append("\n...");
				}
				if (!contents.empty())
					editor.CallTipShow(notifyCode->position, contents.c_str());
			}
			break;
		case SCN_DWELLEND:
			editor.CallTipCancel();
			break;
		case NPPN_READY:
			editor.SetMouseDwellTime(100);
			break;
		case NPPN_SHUTDOWN:
			break;
		case NPPN_BUFFERACTIVATED:
			editor.SetScintillaInstance(npp.GetCurrentScintillaHwnd());

			if (clients.find(npp.GetCurrentBufferID()) == clients.end()) {
				if (editor.GetLexerLanguage() == "python") {
					clients[npp.GetCurrentBufferID()] = new LspClient(editor);
					current_client = clients[npp.GetCurrentBufferID()];
					current_client->notifyDidOpen();
				}
				else
					current_client = nullptr;
			}
			else {
				// Client exists
				current_client = clients[npp.GetCurrentBufferID()];
			}
			break;
		case NPPN_FILESAVED: {
			BufferID id = notifyCode->nmhdr.idFrom;
			if (clients.find(id) != clients.end()) {
				auto client = clients[id];
				client->notifyDidSave();
			}
			break;
		}
		case NPPN_FILECLOSED: {
			BufferID id = notifyCode->nmhdr.idFrom;
			if (clients.find(id) != clients.end()) {
				auto client = clients[id];
				client->requestShutdown();
				client->notifyExit();
				clients.erase(id);
			}
			break;
		}
	}

	return;
}

extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM wParam, LPARAM lParam) {
	return TRUE;
}

extern "C" __declspec(dllexport) BOOL isUnicode() {
	return TRUE;
}
