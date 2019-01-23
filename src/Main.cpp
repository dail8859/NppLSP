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
#include <sstream>


using namespace nlohmann;


static HANDLE _hModule;
static NppGateway npp;
static ScintillaGateway editor;

static void GotoDefiniton();
static void Autocompletion();
static void ShowAbout();

ShortcutKey sk = { false, false, false, VK_F12 };
ShortcutKey sk2 = { true, false, false, VK_SPACE };
static FuncItem funcItem[] = {
	{ L"Goto Definiton", GotoDefiniton, 0, false, &sk },
	{ L"Autocompletion", Autocompletion, 0, false, &sk2 },
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

	int s = editor.PositionFromLine(range["start"]["line"].get<uint32_t>()) + range["start"]["character"].get<uint32_t>();
	int e = editor.PositionFromLine(range["end"]["line"].get<uint32_t>()) + range["end"]["character"].get<uint32_t>();

	editor.SetSel(s, e);
}

template <typename T, typename U>
static std::string join(const std::vector<T> &v, const U &delim) {
	std::stringstream ss;
	for (size_t i = 0; i < v.size(); ++i) {
		if (i != 0) ss << delim;
		ss << v[i];
	}
	return ss.str();
}

enum xpm_type {
	CLASS = 1,
	NAMESPACE = 2,
	METHOD = 3,
	SIGNAL = 4,
	SLOT = 5,
	VARIABLE = 6,
	STRUCT = 7,
	TYPEDEF = 8
};

std::vector<std::string> xpm_images = {
	"", // Offset due to copying this all from Lua
	"/* XPM */static char *class[] = {/* columns rows colors chars-per-pixel */\"16 16 10 1 \",\"  c #000000\",\". c #001CD0\",\"X c #008080\",\"o c #0080E8\",\"O c #00C0C0\",\"+ c #24D0FC\",\"@ c #00FFFF\",\"# c #A4E8FC\",\"$ c #C0FFFF\",\"% c None\",/* pixels */\"%%%%%  %%%%%%%%%\",\"%%%% ##  %%%%%%%\",\"%%% ###++ %%%%%%\",\"%% +++++.   %%%%\",\"%% oo++.. $$  %%\",\"%% ooo.. $$$@@ %\",\"%% ooo. @@@@@X %\",\"%%%   . OO@@XX %\",\"%%% ##  OOOXXX %\",\"%% ###++ OOXX %%\",\"% +++++.  OX %%%\",\"% oo++.. %  %%%%\",\"% ooo... %%%%%%%\",\"% ooo.. %%%%%%%%\",\"%%  o. %%%%%%%%%\",\"%%%%  %%%%%%%%%%\"};",
	"/* XPM */static char *namespace[] = {/* columns rows colors chars-per-pixel */\"16 16 7 1 \",\"  c #000000\",\". c #1D1D1D\",\"X c #393939\",\"o c #555555\",\"O c #A8A8A8\",\"+ c #AAAAAA\",\"@ c None\",/* pixels */\"@@@@@@@@@@@@@@@@\",\"@@@@+@@@@@@@@@@@\",\"@@@.o@@@@@@@@@@@\",\"@@@ +@@@@@@@@@@@\",\"@@@ +@@@@@@@@@@@\",\"@@+.@@@@@@@+@@@@\",\"@@+ @@@@@@@o.@@@\",\"@@@ +@@@@@@+ @@@\",\"@@@ +@@@@@@+ @@@\",\"@@@.X@@@@@@@.+@@\",\"@@@@+@@@@@@@ @@@\",\"@@@@@@@@@@@+ @@@\",\"@@@@@@@@@@@+ @@@\",\"@@@@@@@@@@@X.@@@\",\"@@@@@@@@@@@+@@@@\",\"@@@@@@@@@@@@@@@@\"};",
	"/* XPM */static char *method[] = {/* columns rows colors chars-per-pixel */\"16 16 5 1 \",\"  c #000000\",\". c #E0BC38\",\"X c #F0DC5C\",\"o c #FCFC80\",\"O c None\",/* pixels */\"OOOOOOOOOOOOOOOO\",\"OOOOOOOOOOOOOOOO\",\"OOOOOOOOOOOOOOOO\",\"OOOOOOOOOO  OOOO\",\"OOOOOOOOO oo  OO\",\"OOOOOOOO ooooo O\",\"OOOOOOO ooooo. O\",\"OOOO  O XXoo.. O\",\"OOO oo  XXX... O\",\"OO ooooo XX.. OO\",\"O ooooo.  X. OOO\",\"O XXoo.. O  OOOO\",\"O XXX... OOOOOOO\",\"O XXX.. OOOOOOOO\",\"OO  X. OOOOOOOOO\",\"OOOO  OOOOOOOOOO\"};",
	"/* XPM */static char *signal[] = {/* columns rows colors chars-per-pixel */\"16 16 6 1 \",\"  c #000000\",\". c #FF0000\",\"X c #E0BC38\",\"o c #F0DC5C\",\"O c #FCFC80\",\"+ c None\",/* pixels */\"++++++++++++++++\",\"++++++++++++++++\",\"++++++++++++++++\",\"++++++++++  ++++\",\"+++++++++ OO  ++\",\"++++++++ OOOOO +\",\"+++++++ OOOOOX +\",\"++++  + ooOOXX +\",\"+++ OO  oooXXX +\",\"++ OOOOO ooXX ++\",\"+ OOOOOX  oX +++\",\"+ ooOOXX +  ++++\",\"+ oooXXX +++++++\",\"+ oooXX +++++..+\",\"++  oX ++++++..+\",\"++++  ++++++++++\"};",
	"/* XPM */static char *slot[] = {/* columns rows colors chars-per-pixel */\"16 16 5 1 \",\"  c #000000\",\". c #E0BC38\",\"X c #F0DC5C\",\"o c #FCFC80\",\"O c None\",/* pixels */\"OOOOOOOOOOOOOOOO\",\"OOOOOOOOOOOOOOOO\",\"OOOOOOOOOOOOOOOO\",\"OOOOOOOOOO  OOOO\",\"OOOOOOOOO oo  OO\",\"OOOOOOOO ooooo O\",\"OOOOOOO ooooo. O\",\"OOOO  O XXoo.. O\",\"OOO oo  XXX... O\",\"OO ooooo XX.. OO\",\"O ooooo.  X. OOO\",\"O XXoo.. O  OOOO\",\"O XXX... OOOOOOO\",\"O XXX.. OOOOO   \",\"OO  X. OOOOOO O \",\"OOOO  OOOOOOO   \"};",
	"/* XPM */static char *variable[] = {/* columns rows colors chars-per-pixel */\"16 16 5 1 \",\"  c #000000\",\". c #8C748C\",\"X c #9C94A4\",\"o c #ACB4C0\",\"O c None\",/* pixels */\"OOOOOOOOOOOOOOOO\",\"OOOOOOOOOOOOOOOO\",\"OOOOOOOOOOOOOOOO\",\"OOOOOOOOOOOOOOOO\",\"OOOOOOOOOOOOOOOO\",\"OOOOOOOOOOOOOOOO\",\"OOOOOOOOO  OOOOO\",\"OOOOOOOO oo  OOO\",\"OOOOOOO ooooo OO\",\"OOOOOO ooooo. OO\",\"OOOOOO XXoo.. OO\",\"OOOOOO XXX... OO\",\"OOOOOO XXX.. OOO\",\"OOOOOOO  X. OOOO\",\"OOOOOOOOO  OOOOO\",\"OOOOOOOOOOOOOOOO\"};",
	"/* XPM */static char *struct[] = {/* columns rows colors chars-per-pixel */\"16 16 14 1 \",\"  c #000000\",\". c #008000\",\"X c #00C000\",\"o c #00FF00\",\"O c #808000\",\"+ c #C0C000\",\"@ c #FFFF00\",\"# c #008080\",\"$ c #00C0C0\",\"% c #00FFFF\",\"& c #C0FFC0\",\"* c #FFFFC0\",\"= c #C0FFFF\",\"- c None\",/* pixels */\"-----  ---------\",\"---- &&  -------\",\"--- &&&oo ------\",\"-- ooooo.   ----\",\"-- XXoo.. ==  --\",\"-- XXX.. ===%% -\",\"-- XXX. %%%%%# -\",\"---   . $$%%## -\",\"--- **  $$$### -\",\"-- ***@@ $$## --\",\"- @@@@@O  $# ---\",\"- ++@@OO -  ----\",\"- +++OOO -------\",\"- +++OO --------\",\"--  +O ---------\",\"----  ----------\"};",
	"/* XPM */static char *typedef[] = {/* columns rows colors chars-per-pixel */\"16 16 10 1 \",\"  c #000000\",\". c #404040\",\"X c #6D6D6D\",\"o c #777777\",\"O c #949494\",\"+ c #ACACAC\",\"@ c #BBBBBB\",\"# c #DBDBDB\",\"$ c #EEEEEE\",\"% c None\",/* pixels */\"%%%%%  %%%%%%%%%\",\"%%%% ##  %%%%%%%\",\"%%% ###++ %%%%%%\",\"%% +++++.   %%%%\",\"%% oo++.. $$  %%\",\"%% ooo.. $$$@@ %\",\"%% ooo. @@@@@X %\",\"%%%   . OO@@XX %\",\"%%% ##  OOOXXX %\",\"%% ###++ OOXX %%\",\"% +++++.  OX %%%\",\"% oo++.. %  %%%%\",\"% ooo... %%%%%%%\",\"% ooo.. %%%%%%%%\",\"%%  o. %%%%%%%%%\",\"%%%%  %%%%%%%%%%\"};",
};

std::vector<int> xpm_map = {
	0, //text
	METHOD, //method
	METHOD, //function
	SLOT, //constructor
	VARIABLE, //field
	VARIABLE, //variable
	CLASS, // class
	TYPEDEF, //interface
	NAMESPACE, //module
	VARIABLE, //property
	0, //unit
	0, //value
	TYPEDEF, // enum
	0, //keyword
	0, //snippet
	0, //color
	0, //file
	0, //reference
	0, //folder
	VARIABLE, // enum member
	VARIABLE, //constant
	STRUCT, // struct
	SIGNAL, //event
	0, // operator
	0, // type parameter
};

static void Autocompletion() {
	auto completions = current_client->requestCompletion(editor.LineFromPosition(editor.GetCurrentPos()), editor.GetColumn(editor.GetCurrentPos()));

	std::vector<std::string> autoc;
	for (const auto &c : completions["items"]) {
		auto s = c["insertText"].get<std::string>() + std::string("?") + std::to_string(xpm_map[c["kind"].get<int>()]);
		autoc.push_back(s);
	}

	editor.AutoCShow(0, join(autoc, ' '));
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
			if (current_client && notifyCode->position != -1) {
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
			for (size_t i = 0; i < xpm_images.size(); ++i) {
				if (xpm_map[i] != 0) {
					editor.RegisterImage(i, xpm_images[i]);
				}
			}
			editor.RegisterImage(1, xpm_images[3]);
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
