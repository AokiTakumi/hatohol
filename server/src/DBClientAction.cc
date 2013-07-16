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

#include "DBClientAction.h"

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
DBClientAction::DBClientAction(void)
{
}

DBClientAction::~DBClientAction()
{
}

void DBClientAction::getActionList(const EventInfo &eventInfo,
                                   ActionDefList &actionDefList)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
}

void DBClientAction::logStartExecAction(const ActionDef &actionDef)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
}

void DBClientAction::logEndExecAction(const ExitChildInfo &exitChildInfo)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
}

void DBClientAction::logErrExecAction(const ActionDef &actionDef,
                                      const string &msg)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
}
