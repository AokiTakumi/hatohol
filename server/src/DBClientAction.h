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

#ifndef DBClientAction_h
#define DBClientAction_h

#include <string>
#include "DBClientHatohol.h"
#include "DBClientConnectable.h"
#include "Params.h"

enum ActionType {
	ACTION_COMMAND,
	ACTION_RESIDENT,
};

enum ComparisonType {
	CMP_INVALID,
	CMP_EQ,
	CMP_EQ_GT,
};

enum ActionConditionEnableFlag {
	ACTCOND_SERVER_ID        = (1 << 0),
	ACTCOND_HOST_ID          = (1 << 1),
	ACTCOND_HOST_GROUP_ID    = (1 << 2),
	ACTCOND_TRIGGER_ID       = (1 << 3),
	ACTCOND_TRIGGER_STATUS   = (1 << 4),
	ACTCOND_TRIGGER_SEVERITY = (1 << 5),
};

struct ActionCondition {
	uint32_t enableBits;

	int      serverId;
	uint64_t hostId;
	uint64_t hostGroupId;
	uint64_t triggerId;
	int      triggerStatus;
	int      triggerSeverity;
	ComparisonType triggerSeverityCompType;

	// methods
	ActionCondition(void)
	: enableBits(0),
	  triggerSeverityCompType(CMP_INVALID)
	{
	}

	ActionCondition(uint32_t _enableBits, int _serverId, uint64_t _hostId,
	                uint64_t _hostGroupId, uint64_t _triggerId,
	                int _triggerStatus, int _triggerSeverity, 
	                ComparisonType _triggerSeverityCompType)
	: enableBits(_enableBits),
	  serverId(_serverId),
	  hostId(_hostId),
	  hostGroupId(_hostGroupId),
	  triggerId(_triggerId),
	  triggerStatus(_triggerStatus),
	  triggerSeverity(_triggerSeverity),
	  triggerSeverityCompType(_triggerSeverityCompType)
	{
	}

	void enable(int bit)
	{
		enableBits |= bit;
	}

	bool isEnable(int bit) const
	{
		return (enableBits & bit);
	}
};

struct ActionDef {
	int         id;
	ActionCondition condition;
	ActionType  type;
	std::string workingDir;
	std::string path;
	int         timeout; // in sec
};

typedef list<ActionDef>               ActionDefList;
typedef ActionDefList::iterator       ActionDefListIterator;
typedef ActionDefList::const_iterator ActionDefListConstIterator;

struct ExitChildInfo {
	pid_t pid;
	int   status;
	int   exitCode;
};

class DBClientAction :
   public DBClientConnectable<DB_DOMAIN_ID_ACTION>
{
public:
	static int ACTION_DB_VERSION;
	static const char *DEFAULT_DB_NAME;

	static void init(void);
	DBClientAction(void);
	virtual ~DBClientAction();
	int addAction(const ActionDef &actionDef);
	void getActionList(const EventInfo &eventInfo,
	                   ActionDefList &actionDefList);
	void logStartExecAction(const ActionDef &actionDef);
	void logEndExecAction(const ExitChildInfo &exitChildInfo);
	void logErrExecAction(const ActionDef &actionDef, const string &msg);

protected:
	uint64_t getNewActionLogId(void);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;

};

#endif // DBClientAction_h

