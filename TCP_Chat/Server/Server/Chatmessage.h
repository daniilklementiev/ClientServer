#pragma once
#pragma warning(disable : 4996)
#include <string>
#include <stdio.h>
#include <wchar.h>
#include <time.h>
#include <stdlib.h>
#include <Windows.h>

class ChatMessage {
private:
	char* nick;
	char* txt;
	time_t dt;
	char* _str;
	long long id;
public:
	std::list<ChatMessage>messages;
	ChatMessage() : nick{ NULL }, txt{ NULL }, dt{ time(NULL) }, _str{ NULL }, id{ NULL } {}
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

	long long getId() { return this->id; }
	void setId(long long id) {
		this->id = id;
	}


	bool  parseStringDT(char* str) {
		if (str == NULL) return false;

		// looking for TAB symbol
		int tabPos = -1;
		int len = strlen(str);
		int i = 0;
		while (str[i] != '\t' && i < len) ++i;

		if (i == len) return false;

		tabPos = i;

		// from 0 to TAB - text
		if (this->txt != NULL) delete[] this->txt;

		this->txt = new char[tabPos + 1];
		for (i = 0; i < tabPos; ++i)
			this->txt[i] = str[i];

		this->txt[tabPos] = '\0';
		++tabPos;

		// from TAB to next TAB - username

		while (str[tabPos] != '\t' && tabPos < len) ++tabPos;
		if (tabPos == len) return false;
		if (this->nick != NULL) delete[] this->nick;

		this->nick = new char[tabPos - i + 1];

		for (int j = i + 1; j < tabPos; ++j)
			this->nick[j - i - 1] = str[j];

		this->nick[tabPos - i - 1] = '\0';

		// from TAB to next TAB - dt
		i = tabPos;
		tabPos++;
		char timestamp[32];
		while (str[tabPos] != '\t' && tabPos < len) ++tabPos;
		if (tabPos == len) return false;
		for (int j = i + 1; j < tabPos; ++j)
			timestamp[j - i - 1] = str[j];
		timestamp[tabPos - i - 1] = '\0';
		dt = atoi(timestamp);
		// from TAB to END - id
		i = tabPos + 1;
		while (str[i] != '\0')
		{
			timestamp[i - tabPos - 1] = str[i];
			i++;
		}
		timestamp[i - tabPos - 1] = str[i];
		id = atoll(timestamp);
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
		// text \t nik \t dt \t id
		int text_len = strlen(this->txt);
		int nick_len = strlen(this->nick);

		if (_str) delete[] _str;
		int len = text_len + nick_len + 66;
		_str = new char[len];

		sprintf(_str, "%s\t%s\t%d\t%lld", this->txt, this->nick, dt, id);
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
			sprintf(_str, "[%.2d:%.2d:%.2d] %s -> %s", 1 + t->tm_hour, 1 + t->tm_min, 1 + t->tm_sec, this->nick, this->txt);
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