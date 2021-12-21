#pragma once
#pragma warning(disable : 4996)
#include <string>
#include <stdio.h>
#include <wchar.h>
#include <time.h>
#include <stdlib.h>
#include <Windows.h>
#include <list>

std::string* splitString(std::string str, char sym) {
	if (str.size() == 0) {
		return NULL;
	}
	size_t pos = 0;
	int parts = 1;

	while ((pos = str.find(sym, pos + 1)) != std::string::npos) {
		parts++;
	}

	std::string* res = new std::string[parts];
	pos = 0;
	size_t pos2;

	for (int i = 0; i < parts - 1; i++) {
		pos2 = str.find(sym, pos + 1);
		res[i] = str.substr(pos, pos2 - pos);
		pos = pos2;
	}

	res[parts - 1] = str.substr(pos + 1);

	if (parts == 1) {
	}

	return res;
}

class ChatMessage {
private:
	char* nick;
	char* txt;
	time_t dt;
	char* _str;
public:
	std::list<ChatMessage>messages;
	ChatMessage() : nick{ NULL }, txt{ NULL }, dt{ time(NULL) }, _str{ NULL } {}
	ChatMessage(char* nick, char* txt) :ChatMessage() {
		setNick(nick);
		setTxt(txt);
	}
	~ChatMessage() {
		if (_str) delete[] _str;
	}

	char* getNick() {
		return nick;
	}
	char* getTxt() {
		return txt;
	}
	void setNick(const char* nick) {
		if (!nick) {
			return;
		}
		if (this->nick) {
			delete this->nick;
		}
		this->nick = new char[strlen(nick)];
		strcpy(this->nick, nick);
	}
	void setTxt(const char* txt) {
		if (!txt) {
			return;
		}
		if (this->txt) {
			delete this->txt;
		}
		this->txt = new char[strlen(txt)];
		strcpy(this->txt, txt);
	}
	time_t getDt() { return this->dt; }
	void setDt(time_t dt) { this->dt = dt; }
	
	bool parseStringDT(char* str) {
		if (str == NULL) return false;

		// looking for TAB symbol
		int tabPosition = -1;
		int len = strlen(str);
		int i = 0;
		while (str[i] != '\t' && i < len) ++i;
		if (i == len) return false;
		tabPosition = i;

		// from 0 to TAB - text
		if (this->txt != NULL) delete[] this->txt;
		this->txt = new char[tabPosition + 1];
		for (i = 0; i < tabPosition; ++i) this->txt[i] = str[i];
		this->txt[tabPosition] = '\0';
		tabPosition++;
		// from TAB to len - name
		while (str[tabPosition] != '\t'
			&& tabPosition < len) ++tabPosition;
		if (tabPosition == len) return false;

		if (this->nick != NULL) delete[] this->nick;
		this->nick = new char[tabPosition - i + 1];
		for (int j = i + 1; j < tabPosition; ++j) this->nick[j - i - 1] = str[j];
		this->nick[tabPosition - i - 1] = '\0';
		
		// from TAB to END - dt
		char timestamp[16];
		i = tabPosition + 1;
		while(str[i] != '\0') {
			timestamp[i - tabPosition - 1] = str[i];
			i++;
		}
		timestamp[i - tabPosition - 1] = str[i];
		dt = atoi(timestamp);
		return true;
	}

	bool parseString(char* str) {
		if (str == NULL) return false;

		// looking for TAB symbol
		int tabPosition = -1;
		int len = strlen(str);
		int i = 0;
		while (str[i] != '\t' && i < len) ++i;
		if (i == len) return false;
		tabPosition = i;

		// from 0 to TAB - text
		if (this->txt != NULL) delete[] this->txt;
		this->txt = new char[tabPosition + 1];
		for (i = 0; i < tabPosition; ++i) this->txt[i] = str[i];
		this->txt[tabPosition] = '\0';

		// from TAB to TAB 
		if (this->nick != NULL) delete[] this->nick;
		this->nick = new char[tabPosition - i + 1];
		for (int j = i + 1; j < len; ++j)
			this->nick[j - i - 1] = str[j];
		this->nick[len - i - 1] = '\0';

		return true;
	}



	char* toString() {
		// text \t nik \t dt
		int text_len = strlen(this->txt);
		int nick_len = strlen(this->nick);
		char timestamp[16];
		itoa(this->dt, timestamp, 10);
		int dt_len = strlen(timestamp);
		
		if (_str) delete[] _str;
		_str = new char[text_len + 1 /*\t*/ + nick_len + 1 /*\t*/ + dt_len + 1 /*\0*/];
		sprintf(_str, "%s\t%s\t%s", this->txt, this->nick, timestamp);
		return _str;
	}

	char* toClientString() {
		if (this->txt == NULL || this->nick == NULL)
			return NULL;
		time_t now_t = time(NULL);
		tm* now = localtime(&now_t);
		tm* t = new tm;
		localtime_s(t, &this->dt);
		int text_len = strlen(this->txt);
		int nick_len = strlen(this->nick);
		if (_str) delete[] _str;
		_str = new char[text_len + nick_len + 32];
		if (now->tm_mday == t->tm_mday)
		{
			sprintf(_str, "[%d:%d:%.2d] %s -> %s", 1 + t->tm_hour, 1 + t->tm_min, 1 + t->tm_sec, this->nick, this->txt);
		}
		else if (now->tm_mday - 1 == t->tm_mday)
		{
			sprintf(_str, "[Yesterday] %s -> %s", this->nick, this->txt);
		}
		else if (t->tm_year == now->tm_year && t->tm_mon == now->tm_mon) {

			sprintf(_str, "%d days ago at %.2d:%.2d %s:%s", now->tm_mday - t->tm_mday, t->tm_hour, t->tm_min, getNick(), getTxt());

		}
		else {
			sprintf(_str, "[%d.%d.%d] %s -> %s", 1 + t->tm_mday, 1 + t->tm_mon, 1900 + t->tm_year, this->nick, this->txt);
		}
		
		
		return _str;
	}

};
