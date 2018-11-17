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

#include "LspClient.h"

#include <string>
#include <windows.h>

using namespace nlohmann;

enum CompletionItemKind {
	Text = 1,
	Method = 2,
	Function = 3,
	Constructor = 4,
	Field = 5,
	Variable = 6,
	Class = 7,
	Interface = 8,
	Module = 9,
	Property = 10,
	Unit = 11,
	Value = 12,
	Enum = 13,
	Keyword = 14,
	Snippet = 15,
	Color = 16,
	File = 17,
	Reference = 18
};


LspClient::LspClient(ScintillaGateway &editor) : editor(editor) {
	log_file = CreateFile(L"C:\\lsp.log", GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	CreatePipes();
	CreateChildProcess();

	capabilities = requestInitialize();
}

LspClient::~LspClient() {
	if (log_file) {
		CloseHandle(log_file);
	}

	CloseHandle(g_hChildStd_IN_Rd);
	CloseHandle(g_hChildStd_IN_Wr);
	CloseHandle(g_hChildStd_OUT_Rd);
	CloseHandle(g_hChildStd_OUT_Wr);
}

json LspClient::read() {
	DWORD dwRead;
	std::string buffer;

	buffer.resize(128);
	ReadFile(g_hChildStd_OUT_Rd, (void*)buffer.data(), 128, &dwRead, NULL);

	// Find the lenght .e.g "Content-Length: XXX\r\n"
	int length = std::stoi(buffer.substr(buffer.find_first_of("123456789")));

	// Skip all the headers since they end with two new lines
	size_t start = buffer.find("\r\n\r\n") + 4;

	// For sanity erase everything up until the start of the json
	buffer.erase(0, start);

	// Probably need to grab more
	if (buffer.length() < (size_t)length) {
		// Resize it enough to fit the rest of the data
		buffer.resize(length);

		ReadFile(g_hChildStd_OUT_Rd, (void*)&buffer[dwRead - start], length - (dwRead - start), &dwRead, NULL);
	}

	json result = json::parse(buffer);

	log(result);

	return result;
}

json LspClient::request(const std::string &method, const json &params) {
	int request_id = this->id;
	this->id++;

	json j = {
		{ "jsonrpc", "2.0" },
		{ "id", request_id },
		{ "method", method },
		{ "params", params }
	};

	std::string s = j.dump();
	std::string message = "Content-Length: ";
	message += std::to_string(s.length());
	message += "\r\n\r\n";
	message += s;

	log(j);

	DWORD at;
	WriteFile(g_hChildStd_IN_Wr, message.c_str(), message.length(), &at, NULL);

	// Read incoming JSON messages until the proper response is found.
	do {
		json message = read();
		if (message.find("id") == message.end()) {
			handleNotification(message["method"], message["params"]);
		}
		else if (message["id"].get<int>() == request_id) {
			return message["result"];
		}
		else {
			int i = message["id"].get<int>();
		}
	} while (true);

	return json::object({});
}

void LspClient::notify(const std::string &method, const json &params) {
	json j = {
		{ "jsonrpc", "2.0" },
		{ "method", method },
		{ "params", params }
	};

	std::string s = j.dump();
	std::string message = "Content-Length: ";
	message += std::to_string(s.length());
	message += "\r\n\r\n";
	message += s;

	log(j);

	DWORD at;
	WriteFile(g_hChildStd_IN_Wr, message.c_str(), message.length(), &at, NULL);
}


json LspClient::requestInitialize() {
	std::string content = R"(
  {
    "processId":null,
    "capabilities":{
      "textDocument":{
        "signatureHelp":{
          "signatureInformation":{
            "documentationFormat":[
              "plaintext"
            ]
          }
        },
        "hover":{
          "contentFormat":[
            "plaintext"
          ]
        },
        "documentSymbol":{
          "symbolKind":{
            "valueSet":[
              1,
              2,
              3,
              4,
              5,
              6,
              7,
              8,
              9,
              10,
              11,
              12,
              13,
              14,
              15,
              16,
              17,
              18,
              19,
              20,
              21,
              22,
              23,
              24,
              25,
              26
            ]
          }
        },
        "completion":{
          "completionItem":{
            "preselectSupport":true,
            "documentationFormat":[
              "plaintext"
            ]
          },
          "completionItemKind":{
            "valueSet":[
              1,
              2,
              3,
              4,
              5,
              6,
              7,
              8,
              9,
              10,
              11,
              12,
              13,
              14,
              15,
              16,
              17,
              18,
              19,
              20,
              21,
              22,
              23,
              24,
              25
            ]
          }
        }
      }
    },
    "rootUri":"file:///Users/Justin/Desktop"
  }
)";

	return request("initialize", json::parse(content));
}

json LspClient::requestShutdown() {
	return request("shutdown");
}

void LspClient::notifyExit() {
	notify("exit");
}



void LspClient::notifyDidChange() {
	static int i = 0;

	notify("textDocument/didChange", json({
		{ "textDocument",{
			{ "uri", "file:///Users/Justin/Desktop/lsp.py" },
			{ "version", ++i } // TODO: incrementing version number?
		} },
		{ "contentChanges", {
			{{"text", editor.GetText().c_str()}},
		} }
	}));
}

void LspClient::notifyDidOpen() {
	notify("textDocument/didOpen", json({
		{ "textDocument",{
			{ "uri", "file:///Users/Justin/Desktop/lsp.py" },
			{ "languageId", "python" },
			{ "version", 0 },
			{ "text", editor.GetText().c_str() }
		} }
	}));
}

void LspClient::notifyDidSave() {
	notifyDidChange();

	notify("textDocument/didSave", json({
		{ "textDocument",{
			{ "uri", "file:///Users/Justin/Desktop/lsp.py" }
		} }
	}));
}

json LspClient::requestCompletion(int line, int character) {
	notifyDidChange();

	return request("textDocument/completion", json({
		{ "textDocument",{
			{ "uri", "file:///Users/Justin/Desktop/lsp.py" }
		} },
		{ "position",{
			{ "line", line },
			{ "character", character }
		} }
	}));
}

json LspClient::requestHover(int position) {
	notifyDidChange();

	return request("textDocument/hover", json({
		{ "textDocument",{
			{ "uri", "file:///Users/Justin/Desktop/lsp.py" }
		} },
		{ "position",{
			{ "line", editor.LineFromPosition(position) },
			{ "character", editor.GetColumn(position) }
		} }
	}));

}

json LspClient::requestDefinition(int position) {
	notifyDidChange();

	auto locations = request("textDocument/definition", json({
		{ "textDocument",{
			{ "uri", "file:///Users/Justin/Desktop/lsp.py" }
		} },
		{ "position",{
			{ "line", editor.LineFromPosition(position) },
			{ "character", editor.GetColumn(position) }
		} }
	}));

	// TODO: len == 0 or len > 1?
	return locations[0]["range"];
}

void LspClient::handleNotification(const std::string &method, const json &params) {
	if (method == "textDocument/publishDiagnostics") {
		return;
	}
	else {
		return;
	}
}


void LspClient::CreatePipes() {
	SECURITY_ATTRIBUTES saAttr;

	// Set the bInheritHandle flag so pipe handles are inherited.
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;

	// Create a pipe for the child process's STDOUT
	if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0))
		MessageBox(NULL, L"StdoutRd CreatePipe", L"Oh Noes", MB_OK);

	// Ensure the read handle to the pipe for STDOUT is not inherited
	if (!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))
		MessageBox(NULL, L"Stdout SetHandleInformation", L"Oh Noes", MB_OK);

	// Create a pipe for the child process's STDIN. 
	if (!CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0))
		MessageBox(NULL, L"Stdin CreatePipe", L"Oh Noes", MB_OK);

	// Ensure the write handle to the pipe for STDIN is not inherited. 
	if (!SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0))
		MessageBox(NULL, L"Stdin SetHandleInformation", L"Oh Noes", MB_OK);
}

void LspClient::CreateChildProcess() {
	wchar_t szCmdline[] = TEXT("child");
	PROCESS_INFORMATION piProcInfo;
	STARTUPINFO siStartInfo;
	BOOL bSuccess = FALSE;

	// Set up members of the PROCESS_INFORMATION structure. 

	ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

	// Set up members of the STARTUPINFO structure. 
	// This structure specifies the STDIN and STDOUT handles for redirection.

	ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
	siStartInfo.cb = sizeof(STARTUPINFO);
	siStartInfo.hStdError = g_hChildStd_OUT_Wr;
	siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
	siStartInfo.hStdInput = g_hChildStd_IN_Rd;
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

	wchar_t argv[256];
	wcscpy(argv, L"pyls -vvvv --log-file \"C:\\pickles.txt\"");

	// Create the child process
	bSuccess = CreateProcessW(NULL,
		argv,          // command line 
		NULL,          // process security attributes 
		NULL,          // primary thread security attributes 
		TRUE,          // handles are inherited 
		0,             // creation flags 
		NULL,          // use parent's environment 
		NULL,          // use parent's current directory 
		&siStartInfo,  // STARTUPINFO pointer 
		&piProcInfo);  // receives PROCESS_INFORMATION 

					   // If an error occurs, exit the application
	if (!bSuccess)
		MessageBox(NULL, L"CreateProcess", L"Oh Noes", MB_OK);
	else {
		// Close handles to the child process and its primary thread.
		// Some applications might keep these handles to monitor the status
		// of the child process, for example. 

		CloseHandle(piProcInfo.hProcess);
		CloseHandle(piProcInfo.hThread);
	}
}

void LspClient::log(const std::string &s) {
	DWORD at;

	if (log_file) {
		WriteFile(log_file, "---------\r\n", 11, &at, NULL);
		WriteFile(log_file, s.c_str(), s.length(), &at, NULL);
		WriteFile(log_file, "\r\n\r\n", 4, &at, NULL);
		FlushFileBuffers(log_file);
	}
}

void LspClient::log(const json &j) {
	DWORD at;

	if (log_file) {
		auto message = j.dump(1, '\t');
		WriteFile(log_file, "---------\r\n\r\n", 13, &at, NULL);
		WriteFile(log_file, message.c_str(), message.length(), &at, NULL);
		WriteFile(log_file, "\r\n\r\n", 4, &at, NULL);
		FlushFileBuffers(log_file);
	}
}
