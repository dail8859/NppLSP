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

#pragma once

#include "npp\PluginInterface.h"

#include <string>

typedef uptr_t BufferID;

class NppGateway final {
private:

	template<typename T>
	static inline void trim(std::basic_string<T> &s) {
		while (s.length() > 0 && s.back() == 0) s.pop_back();
	}

	template<typename T, typename U>
	inline sptr_t Call(unsigned int message, T wParam = 0, U lParam = 0) const {
		sptr_t retVal = SendMessage(data._nppHandle, message, (uptr_t)wParam, (sptr_t)lParam);
		return retVal;
	}

public:
	NppData data;

	NppGateway() {}

	explicit NppGateway(NppData data) {
		SetNppData(data);
	}

	void SetNppData(NppData data) {
		this->data = data;
	}

	int GetCurrentScintilla() const {
		int id;
		Call(NPPM_GETCURRENTSCINTILLA, 0, &id);
		return id;
	}

	HWND GetCurrentScintillaHwnd() const {
		auto id = GetCurrentScintilla();
		return (&data._scintillaMainHandle)[id];
	}

	//NPPM_GETCURRENTLANGTYPE

	void SetCurrentLangType(LangType lang) const {
		Call(NPPM_SETCURRENTLANGTYPE, 0, lang);
	}
	
	//NPPM_GETNBOPENFILES
	//NPPM_GETOPENFILENAMES
	//NPPM_MODELESSDIALOG
	//NPPM_GETNBSESSIONFILES
	//NPPM_GETSESSIONFILES
	//NPPM_SAVESESSION
	//NPPM_SAVECURRENTSESSION
	//NPPM_GETOPENFILENAMESPRIMARY
	//NPPM_GETOPENFILENAMESSECOND
	//NPPM_CREATESCINTILLAHANDLE
	//NPPM_DESTROYSCINTILLAHANDLE
	//NPPM_GETNBUSERLANG
	//NPPM_GETCURRENTDOCINDEX

	void SetStatusBar(int section, const wchar_t *status) const {
		Call(NPPM_SETSTATUSBAR, section, status);
	}

	void SetStatusBar(int section, const std::wstring &status) const {
		Call(NPPM_SETSTATUSBAR, section, status.c_str());
	}

	//NPPM_GETMENUHANDLE
	//NPPM_ENCODESCI
	//NPPM_DECODESCI
	//NPPM_ACTIVATEDOC
	//NPPM_LAUNCHFINDINFILESDLG
	//NPPM_DMMSHOW
	//NPPM_DMMHIDE
	//NPPM_DMMUPDATEDISPINFO
	//NPPM_DMMREGASDCKDLG
	//NPPM_LOADSESSION
	//NPPM_DMMVIEWOTHERTAB
	//NPPM_RELOADFILE
	//NPPM_SWITCHTOFILE
	//NPPM_SAVECURRENTFILE
	//NPPM_SAVEALLFILES

	void SetMenuItemCheck(int cmdID, bool check) const {
		Call(NPPM_SETMENUITEMCHECK, cmdID, check ? 1 : 0);
	}

	//NPPM_ADDTOOLBARICON
	//NPPM_GETWINDOWSVERSION
	//NPPM_DMMGETPLUGINHWNDBYNAME
	//NPPM_MAKECURRENTBUFFERDIRTY
	//NPPM_GETENABLETHEMETEXTUREFUNC

	void GetPluginsConfigDir(int size, wchar_t *configDir) const {
		Call(NPPM_GETPLUGINSCONFIGDIR, size, configDir);
	}

	std::wstring GetPluginsConfigDir() const {
		std::wstring text(MAX_PATH, '\0');
		Call(NPPM_GETPLUGINSCONFIGDIR, text.length(), &text[0]);
		trim(text);
		return text;
	}

	//NPPM_MSGTOPLUGIN

	void MenuCommand(int command) const {
		Call(NPPM_MENUCOMMAND, 0, command);
	}

	//NPPM_TRIGGERTABBARCONTEXTMENU
	//NPPM_GETNPPVERSION
	//NPPM_HIDETABBAR
	//NPPM_ISTABBARHIDDEN
	//NPPM_GETPOSFROMBUFFERID

	int GetFullPathFromBufferID(BufferID id, wchar_t *fullFilePath) const {
		auto ret = Call(NPPM_GETFULLPATHFROMBUFFERID, id, fullFilePath);
		return static_cast<int>(ret);
	}

	std::wstring GetFullPathFromBufferID(BufferID id) const {
		auto len = Call(NPPM_GETFULLPATHFROMBUFFERID, id, NULL);

		// Notepad++ documenation says it will return -1 for unknown ids
		if (len == -1)
			return std::wstring();

		std::wstring text(len + 1, '\0');
		Call(NPPM_GETFULLPATHFROMBUFFERID, id, &text[0]);
		trim(text);
		return text;
	}

	//NPPM_GETBUFFERIDFROMPOS

	BufferID GetCurrentBufferID() const {
		return Call(NPPM_GETCURRENTBUFFERID, 0, 0);
	}

	//NPPM_RELOADBUFFERID
	//NPPM_GETBUFFERLANGTYPE
	//NPPM_SETBUFFERLANGTYPE
	//NPPM_GETBUFFERENCODING
	//NPPM_SETBUFFERENCODING
	//NPPM_GETBUFFERFORMAT
	//NPPM_SETBUFFERFORMAT
	//NPPM_ADDREBAR
	//NPPM_HIDETOOLBAR
	//NPPM_ISTOOLBARHIDDEN
	//NPPM_HIDEMENU
	//NPPM_ISMENUHIDDEN
	//NPPM_HIDESTATUSBAR
	//NPPM_ISSTATUSBARHIDDEN
	//NPPM_GETSHORTCUTBYCMDID

	bool DoOpen(const wchar_t *fullPathName2Open) const {
		return Call(NPPM_DOOPEN, 0, fullPathName2Open) == 1;
	}

	bool DoOpen(const std::wstring &fullPathName2Open) const {
		return Call(NPPM_DOOPEN, 0, fullPathName2Open.c_str()) == 1;
	}

	//NPPM_SAVECURRENTFILEAS
	//NPPM_GETCURRENTNATIVELANGENCODING
	//NPPM_ALLOCATESUPPORTED
	//NPPM_ALLOCATECMDID
	//NPPM_ALLOCATEMARKER
	//NPPM_GETLANGUAGENAME
	//NPPM_GETLANGUAGEDESC
	//NPPM_SHOWDOCSWITCHER
	//NPPM_ISDOCSWITCHERSHOWN
	//NPPM_GETAPPDATAPLUGINSALLOWED
	//NPPM_GETCURRENTVIEW
	//NPPM_DOCSWITCHERDISABLECOLUMN
	//NPPM_GETEDITORDEFAULTFOREGROUNDCOLOR
	//NPPM_GETEDITORDEFAULTBACKGROUNDCOLOR
	//NPPM_SETSMOOTHFONT
	//NPPM_SETEDITORBORDEREDGE
	//NPPM_SAVEFILE
	//NPPM_DISABLEAUTOUPDATE
	//NPPM_GETFULLCURRENTPATH
	//NPPM_GETCURRENTDIRECTORY

	void GetFileName(int size, wchar_t *fileName) const {
		Call(NPPM_GETFILENAME, size, fileName);
	}

	std::wstring GetFileName() const {
		std::wstring text(MAX_PATH, '\0');
		Call(NPPM_GETFILENAME, text.length(), &text[0]);
		trim(text);
		return text;
	}

	//NPPM_GETNAMEPART
	//NPPM_GETEXTPART
	//NPPM_GETCURRENTWORD
	//NPPM_GETNPPDIRECTORY

	int GetCurrentLine() const {
		auto ret = Call(NPPM_GETCURRENTLINE, 0, 0);
		return static_cast<int>(ret);
	}

	//NPPM_GETCURRENTCOLUMN
};
