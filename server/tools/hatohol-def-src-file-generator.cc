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
#include "DBClientHatohol.h"
#include "DBClientConfig.h"
#include "ActionManager.h"

using namespace std;

#define APPEND(CONTENT, FMT, ...) \
do { CONTENT += StringUtils::sprintf(FMT, ##__VA_ARGS__); } while(0)

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

static string makeJsDefSource(char *arg[])
{
	string s;
	s += GPL_V2_OR_LATER_HEADER_C_STYLE;
	APPEND(s, "\n");

	APPEND(s, "var TRIGGER_STATUS_OK      = %d\n",  TRIGGER_STATUS_OK);
	APPEND(s, "var TRIGGER_STATUS_PROBLEM = %d\n",  TRIGGER_STATUS_PROBLEM);
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
	} else {
		printf("Unknown command: %s\n", cmd.c_str());
		return EXIT_FAILURE;
	}
	printf("%s", content.c_str());

	return EXIT_SUCCESS;
}
