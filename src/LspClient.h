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

#include <windows.h>

#include "ScintillaGateway.h"
#include "json.hpp"

using namespace nlohmann;

class LspClient {
public:
	LspClient(ScintillaGateway &editor);
	~LspClient();

	json read();

	json request(const std::string &method, const json &params = json::object());
	void notify(const std::string &method, const json &params = json::object());

	// https://github.com/Microsoft/language-server-protocol/blob/master/versions/protocol-2-x.md

	// General
	json requestInitialize();
	json requestShutdown();
	void notifyExit();
	// $/cancelRequest

	// Window
	// window/showMessage
	// window/showMessageRequest
	// window/logMessage
	// telemetry/event

	// Workspace
	// workspace/didChangeConfiguration
	// workspace/didChangeWatchedFiles
	// workspace/symbol

	// Document
	// textDocument/publishDiagnostics
	void notifyDidChange();
	// textDocument/didClose
	void notifyDidOpen();
	void notifyDidSave();
	json requestCompletion(int line, int character);
	// completionItem/resolve
	json requestHover(int position);
	// textDocument/signatureHelp
	// textDocument/references
	// textDocument/documentHighlight
	// textDocument/documentSymbol
	// textDocument/formatting
	// textDocument/rangeFormatting
	// textDocument/onTypeFormatting
	json requestDefinition(int position);
	// textDocument/codeAction
	// textDocument/codeLens
	// codeLens/resolve
	// textDocument/documentLink
	// documentLink/resolve
	// textDocument/rename

private:
	int id = 0;
	HANDLE log_file;
	ScintillaGateway &editor;
	json capabilities;

	HANDLE g_hChildStd_IN_Rd;
	HANDLE g_hChildStd_IN_Wr;
	HANDLE g_hChildStd_OUT_Rd;
	HANDLE g_hChildStd_OUT_Wr;

	void handleNotification(const std::string &method, const json &params);

	void CreatePipes();
	void CreateChildProcess();

	void log(const std::string &s);
	void log(const json &j);
};

