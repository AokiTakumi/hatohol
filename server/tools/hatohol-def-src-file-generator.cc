/*
 * Copyright (C) 2013 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Hatohol. If not, see <http://www.gnu.org/licenses/>.
 */

#include <cstdio>
#include <cstdlib>
#include <string>
#include <sstream>
#include "DBClientHatohol.h"
#include "DBClientConfig.h"
#include "ActionManager.h"
#include "HatoholError.h"
#include "FaceRest.h"
#include "SessionManager.h"
using namespace std;
using namespace mlpl;

enum LanguageType {
	JAVASCRIPT,
	PYTHON,
};

#define APPEND(CONTENT, FMT, ...) \
do { CONTENT += StringUtils::sprintf(FMT, ##__VA_ARGS__); } while(0)

#define ADD_LINE(SOURCE, LANG_TYPE, VAL) \
SOURCE += makeLine(LANG_TYPE, #VAL, toString(VAL))

#define DEF_LINE(SOURCE, LANG_TYPE, VAL, TYPE, ACTUAL) \
do { \
	TYPE VAL = ACTUAL; \
	ADD_LINE(SOURCE, LANG_TYPE, VAL); \
} while (0)

static const char *GPL_V2_OR_LATER_HEADER_C_STYLE =
"/*\n"
" * Copyright (C) 2013 Project Hatohol\n"
" *\n"
" * This file is part of Hatohol.\n"
" *\n"
" * Hatohol is free software: you can redistribute it and/or modify\n"
" * it under the terms of the GNU General Public License as published by\n"
" * the Free Software Foundation, either version 2 of the License, or\n"
" * (at your option) any later version.\n"
" *\n"
" * Hatohol is distributed in the hope that it will be useful,\n"
" * but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
" * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the\n"
" * GNU General Public License for more details.\n"
" *\n"
" * You should have received a copy of the GNU General Public License\n"
" * along with Hatohol. If not, see <http://www.gnu.org/licenses/>.\n"
" */\n";

static const char *GPL_V2_OR_LATER_HEADER_PLAIN =
"  Copyright (C) 2013 Project Hatohol\n"
"\n"
"  This file is part of Hatohol.\n"
"\n"
"  Hatohol is free software: you can redistribute it and/or modify\n"
"  it under the terms of the GNU General Public License as published by\n"
"  the Free Software Foundation, either version 2 of the License, or\n"
"  (at your option) any later version.\n"
"\n"
"  Hatohol is distributed in the hope that it will be useful,\n"
"  but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the\n"
"  GNU General Public License for more details.\n"
"\n"
"  You should have received a copy of the GNU General Public License\n"
"  along with Hatohol. If not, see <http://www.gnu.org/licenses/>.\n";

static string toString(const int value)
{
	return StringUtils::sprintf("%d", value);
}

static string toString(const string &value)
{
	return StringUtils::sprintf("'%s'", value.c_str());
}

static string makeLine(LanguageType langType,
                       const string &varName, const string value)
{
	string line;
	switch(langType) {
	case JAVASCRIPT:
		line = StringUtils::sprintf("  %s: %s,",
		                            varName.c_str(), value.c_str());
		break;
	case PYTHON:
		line = StringUtils::sprintf("%s = %s",
		                            varName.c_str(), value.c_str());
		break;
	default:
		THROW_HATOHOL_EXCEPTION("Unknown language type: %d\n",
		                        langType);
	}
	line += "\n";
	return line;
}

static void makeDefSourceValues(string &s, LanguageType langType)
{
	ADD_LINE(s, langType, TRIGGER_STATUS_OK);
	ADD_LINE(s, langType, TRIGGER_STATUS_PROBLEM);
	APPEND(s, "\n");

	ADD_LINE(s, langType, EVENT_TYPE_GOOD);
	ADD_LINE(s, langType, EVENT_TYPE_BAD);
	APPEND(s, "\n");

	ADD_LINE(s, langType, TRIGGER_SEVERITY_UNKNOWN);
	ADD_LINE(s, langType, TRIGGER_SEVERITY_INFO);
	ADD_LINE(s, langType, TRIGGER_SEVERITY_WARNING);
	ADD_LINE(s, langType, TRIGGER_SEVERITY_ERROR);
	ADD_LINE(s, langType, TRIGGER_SEVERITY_CRITICAL);
	ADD_LINE(s, langType, TRIGGER_SEVERITY_EMERGENCY);
	APPEND(s, "\n");

	ADD_LINE(s, langType, ACTION_COMMAND);
	ADD_LINE(s, langType, ACTION_RESIDENT);
	APPEND(s, "\n");

	ADD_LINE(s, langType, CMP_INVALID);
	ADD_LINE(s, langType, CMP_EQ);
	ADD_LINE(s, langType, CMP_EQ_GT);
	APPEND(s, "\n");

	ADD_LINE(s, langType, MONITORING_SYSTEM_ZABBIX);
	ADD_LINE(s, langType, MONITORING_SYSTEM_NAGIOS);
	APPEND(s, "\n");

	//
	// HaotholError
	//
	ADD_LINE(s, langType, HTERR_OK);
	ADD_LINE(s, langType, HTERR_UNINITIALIZED);
	ADD_LINE(s, langType, HTERR_UNKNOWN_REASON);
	ADD_LINE(s, langType, HTERR_NOT_IMPLEMENTED);
	ADD_LINE(s, langType, HTERR_GOT_EXCEPTION);
	ADD_LINE(s, langType, HTERR_INVALID_USER);
	APPEND(s, "\n");

	// DBClientConfig
	ADD_LINE(s, langType, HTERR_INVALID_MONITORING_SYSTEM_TYPE);
	ADD_LINE(s, langType, HTERR_INVALID_PORT_NUMBER);
	ADD_LINE(s, langType, HTERR_INVALID_IP_ADDRESS);
	ADD_LINE(s, langType, HTERR_INVALID_HOST_NAME);
	ADD_LINE(s, langType, HTERR_NO_IP_ADDRESS_AND_HOST_NAME);
	ADD_LINE(s, langType, HTERR_NOT_FOUND_SERVER_ID);

	// DBClientUser
	ADD_LINE(s, langType, HTERR_EMPTY_USER_NAME);
	ADD_LINE(s, langType, HTERR_TOO_LONG_USER_NAME);
	ADD_LINE(s, langType, HTERR_INVALID_CHAR);
	ADD_LINE(s, langType, HTERR_EMPTY_PASSWORD);
	ADD_LINE(s, langType, HTERR_TOO_LONG_PASSWORD);
	ADD_LINE(s, langType, HTERR_USER_NAME_EXIST);
	ADD_LINE(s, langType, HTERR_NO_PRIVILEGE);
	ADD_LINE(s, langType, HTERR_INVALID_USER_FLAGS);
	ADD_LINE(s, langType, HTERR_NOT_FOUND_USER_ID);
	ADD_LINE(s, langType, HTERR_EMPTY_USER_ROLE_NAME);
	ADD_LINE(s, langType, HTERR_TOO_LONG_USER_ROLE_NAME);
	ADD_LINE(s, langType, HTERR_USER_ROLE_NAME_OR_FLAGS_EXIST);
	ADD_LINE(s, langType, HTERR_NOT_FOUND_USER_ROLE_ID);
	APPEND(s, "\n");

	// DBClientHatohol
	ADD_LINE(s, langType, HTERR_NOT_FOUND_SORT_ORDER);
	APPEND(s, "\n");

	// DBClientAction
	ADD_LINE(s, langType, HTERR_DELETE_INCOMPLETE);
	APPEND(s, "\n");

	// FaceRest
	ADD_LINE(s, langType, HTERR_UNSUPORTED_FORMAT);
	ADD_LINE(s, langType, HTERR_NOT_FOUND_SESSION_ID);
	ADD_LINE(s, langType, HTERR_NOT_FOUND_ID_IN_URL);
	ADD_LINE(s, langType, HTERR_NOT_FOUND_PARAMETER);
	ADD_LINE(s, langType, HTERR_INVALID_PARAMETER);
	ADD_LINE(s, langType, HTERR_AUTH_FAILED);
	APPEND(s, "\n");

	//
	// FaceRest
	//
	DEF_LINE(s, langType, FACE_REST_API_VERSION,
	         int, FaceRest::API_VERSION);
	DEF_LINE(s, langType, FACE_REST_SESSION_ID_HEADER_NAME,
	         string, FaceRest::SESSION_ID_HEADER_NAME);
	APPEND(s, "\n");

	//
	// DataQueryOption
	//
	DEF_LINE(s, langType, DATA_QUERY_OPTION_SORT_DONT_CARE,
	         DataQueryOption::SortOrder, DataQueryOption::SORT_DONT_CARE);
	DEF_LINE(s, langType, DATA_QUERY_OPTION_SORT_ASCENDING,
	         DataQueryOption::SortOrder, DataQueryOption::SORT_ASCENDING);
	DEF_LINE(s, langType, DATA_QUERY_OPTION_SORT_DESCENDING,
	         DataQueryOption::SortOrder, DataQueryOption::SORT_DESCENDING);
	APPEND(s, "\n");

	//
	// OperationPrivilege
	//
	ADD_LINE(s, langType, ALL_PRIVILEGES);
	ADD_LINE(s, langType, NONE_PRIVILEGE);

	ADD_LINE(s, langType, OPPRVLG_CREATE_USER);
	ADD_LINE(s, langType, OPPRVLG_UPDATE_USER);
	ADD_LINE(s, langType, OPPRVLG_DELETE_USER);
	ADD_LINE(s, langType, OPPRVLG_GET_ALL_USER);

	ADD_LINE(s, langType, OPPRVLG_CREATE_SERVER);
	ADD_LINE(s, langType, OPPRVLG_UPDATE_SERVER);
	ADD_LINE(s, langType, OPPRVLG_UPDATE_ALL_SERVER);
	ADD_LINE(s, langType, OPPRVLG_DELETE_SERVER);
	ADD_LINE(s, langType, OPPRVLG_DELETE_ALL_SERVER);
	ADD_LINE(s, langType, OPPRVLG_GET_ALL_SERVER);

	ADD_LINE(s, langType, OPPRVLG_CREATE_ACTION);
	ADD_LINE(s, langType, OPPRVLG_UPDATE_ACTION);
	ADD_LINE(s, langType, OPPRVLG_UPDATE_ALL_ACTION);
	ADD_LINE(s, langType, OPPRVLG_DELETE_ACTION);
	ADD_LINE(s, langType, OPPRVLG_DELETE_ALL_ACTION);
	ADD_LINE(s, langType, OPPRVLG_GET_ALL_ACTION);

	ADD_LINE(s, langType, OPPRVLG_CREATE_USER_ROLE);
	ADD_LINE(s, langType, OPPRVLG_UPDATE_ALL_USER_ROLE);
	ADD_LINE(s, langType, OPPRVLG_DELETE_ALL_USER_ROLE);

	ADD_LINE(s, langType, NUM_OPPRVLG);
	APPEND(s, "\n");

	// Session
	DEF_LINE(s, langType, SESSION_ID_LEN,
	         size_t, SessionManager::SESSION_ID_LEN);
	APPEND(s, "\n");

	DEF_LINE(s, langType, ENV_NAME_SESSION_ID,
	         string, ActionManager::ENV_NAME_SESSION_ID);
	APPEND(s, "\n");

	//
	// Other
	//
	ADD_LINE(s, langType, HTERR_ERROR_TEST);
	APPEND(s, "\n");
}

static string makeDefSource(LanguageType langType)
{
	string s;
	switch (langType) {
	case JAVASCRIPT:
		s += GPL_V2_OR_LATER_HEADER_C_STYLE;
		APPEND(s, "\n");
		APPEND(s, "var hatohol = {\n");
		makeDefSourceValues(s, langType);
		APPEND(s, "};\n");
		APPEND(s, "\n");
		break;
	case PYTHON:
		APPEND(s, "\"\"\"\n");
		s += GPL_V2_OR_LATER_HEADER_PLAIN;
		APPEND(s, "\"\"\"\n");
		APPEND(s, "\n");
		makeDefSourceValues(s, langType);
		break;
	default:
		THROW_HATOHOL_EXCEPTION("Unknown language type: %d\n",
		                        langType);
	}
	return s;
}

static string makeJsDefSource(char *arg[])
{
	string s;
	s += GPL_V2_OR_LATER_HEADER_C_STYLE;
	APPEND(s, "\n");

	APPEND(s, "var TRIGGER_STATUS_OK      = %d\n",  TRIGGER_STATUS_OK);
	APPEND(s, "var TRIGGER_STATUS_PROBLEM = %d\n",  TRIGGER_STATUS_PROBLEM);
	APPEND(s, "\n");

	APPEND(s, "var EVENT_TYPE_GOOD = %d\n", EVENT_TYPE_GOOD);
	APPEND(s, "var EVENT_TYPE_BAD  = %d\n", EVENT_TYPE_BAD);
	APPEND(s, "\n");

	APPEND(s,
	  "var TRIGGER_SEVERITY_UNKNOWN   = %d\n",  TRIGGER_SEVERITY_UNKNOWN);
	APPEND(s,
	  "var TRIGGER_SEVERITY_INFO      = %d\n",  TRIGGER_SEVERITY_INFO);
	APPEND(s,
	  "var TRIGGER_SEVERITY_WARNING   = %d\n",  TRIGGER_SEVERITY_WARNING);
	APPEND(s,
	  "var TRIGGER_SEVERITY_ERROR     = %d\n",  TRIGGER_SEVERITY_ERROR);
	APPEND(s,
	  "var TRIGGER_SEVERITY_CRITICAL  = %d\n",  TRIGGER_SEVERITY_CRITICAL);
	APPEND(s,
	  "var TRIGGER_SEVERITY_EMERGENCY = %d\n",  TRIGGER_SEVERITY_EMERGENCY);
	APPEND(s, "\n");

	APPEND(s, "var ACTION_COMMAND  = %d\n",  ACTION_COMMAND);
	APPEND(s, "var ACTION_RESIDENT = %d\n",  ACTION_RESIDENT);
	APPEND(s, "\n");

	APPEND(s, "var CMP_INVALID = %d\n",  CMP_INVALID);
	APPEND(s, "var CMP_EQ      = %d\n",  CMP_EQ);
	APPEND(s, "var CMP_EQ_GT   = %d\n",  CMP_EQ_GT);
	APPEND(s, "\n");

	APPEND(s,
	  "var MONITORING_SYSTEM_ZABBIX = %d\n", MONITORING_SYSTEM_ZABBIX);
	APPEND(s,
	  "var MONITORING_SYSTEM_NAGIOS = %d\n", MONITORING_SYSTEM_NAGIOS);

	return s;
}

static void printUsage(void)
{
	printf("Usage:\n");
	printf("\n");
	printf("  $ hatohol-def-src-file-generator command\n");
	printf("\n");
	printf("command:\n");
	printf("  js      : JavaScript\n");
	printf("  py      : Python\n");
	printf("\n");
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printUsage();
		return EXIT_FAILURE;
	}
	string cmd(argv[1]);
	string content;
	if (cmd == "js") {
		content = makeJsDefSource(&argv[2]);
		content += "\n";
		content += makeDefSource(JAVASCRIPT);
	} else if (cmd == "py") {
		content = makeDefSource(PYTHON);
	} else {
		printf("Unknown command: %s\n", cmd.c_str());
		return EXIT_FAILURE;
	}
	printf("%s", content.c_str());

	return EXIT_SUCCESS;
}
