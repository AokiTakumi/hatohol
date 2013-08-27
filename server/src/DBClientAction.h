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

const static uint64_t INVALID_ACTION_LOG_ID = -1;

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

enum {
	ACTLOG_FLAG_QUEUING_TIME = (1 << 0),
	ACTLOG_FLAG_START_TIME   = (1 << 1),
	ACTLOG_FLAG_END_TIME     = (1 << 2),
	ACTLOG_FLAG_EXIT_CODE    = (1 << 3),
};

struct ActionLog {
	uint64_t id;
	int      actionId;
	int      status;
	int      starterId;
	int      queuingTime;
	int      startTime;
	int      endTime;
	int      failureCode;
	int      exitCode;
	uint32_t nullFlags;
};

struct ExitChildInfo {
	pid_t pid;
	int   status;
	int   exitCode;
};

enum {
	IDX_ACTION_LOGS_ACTION_LOG_ID,
	IDX_ACTION_LOGS_ACTION_ID,
	IDX_ACTION_LOGS_STATUS, 
	IDX_ACTION_LOGS_STARTER_ID,
	IDX_ACTION_LOGS_QUEUING_TIME,
	IDX_ACTION_LOGS_START_TIME,
	IDX_ACTION_LOGS_END_TIME,
	IDX_ACTION_LOGS_EXEC_FAILURE_CODE,
	IDX_ACTION_LOGS_EXIT_CODE,
	NUM_IDX_ACTION_LOGS,
};

class DBClientAction :
   public DBClientConnectable<DB_DOMAIN_ID_ACTION>
{
public:
	enum ActionLogStatus {
		// Hatohol limits the number of actions running
		// at the same time. If it excceds the limit,
		// a new action is registered as ACTLOG_STAT_QUEUING.
		ACTLOG_STAT_QUEUING,

		ACTLOG_STAT_STARTED,
		ACTLOG_STAT_SUCCEEDED,
		ACTLOG_STAT_FAILED,

		// For resident mode action, following staus is logged
		// until the launching and setup of hatohol-resident-yard
		// is completed.
		ACTLOG_STAT_LAUNCHING_RESIDENT,
	};

	enum ActionLogExecFailureCode {
		ACTLOG_EXECFAIL_NONE,
		ACTLOG_EXECFAIL_EXEC_FAILURE,
		ACTLOG_EXECFAIL_ENTRY_NOT_FOUND,
		ACTLOG_EXECFAIL_KILLED_TIMEOUT,
		ACTLOG_EXECFAIL_PIPE_READ_HUP,
		ACTLOG_EXECFAIL_PIPE_READ_ERR,
		ACTLOG_EXECFAIL_PIPE_WRITE_ERR,
		ACTLOG_EXECFAIL_UNEXPECTED_EXIT,
	};

	struct LogEndExecActionArg {
		uint64_t logId;
		ActionLogStatus status;
		int   exitCode;
	};

	static int ACTION_DB_VERSION;
	static const char *DEFAULT_DB_NAME;

	static void init(void);
	static const char *getTableNameActions(void);
	static const char *getTableNameActionLogs(void);

	DBClientAction(void);
	virtual ~DBClientAction();
	void addAction(ActionDef &actionDef);
	void getActionList(const EventInfo &eventInfo,
	                   ActionDefList &actionDefList);
	uint64_t logStartExecAction
	  (const ActionDef &actionDef,
	   ActionLogExecFailureCode failureCode = ACTLOG_EXECFAIL_NONE);
	void logEndExecAction(const LogEndExecActionArg &logArg);

	/**
	 * Get the action log.
	 * @param actionLog
	 * The returned values are filled in this instance.
	 * @param logId
	 * The log ID to be searched.
	 *
	 * @return true if the log is found. Otherwise false.
	 */
	bool getLog(ActionLog &actionLog, uint64_t logId);

protected:
	ItemDataNullFlagType getNullFlag(const ActionDef &actionDef,
	                                 ActionConditionEnableFlag enableFlag);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;

};

#endif // DBClientAction_h

