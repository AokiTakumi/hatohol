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

#include "ActorCollector.h"
#include "DBClientAction.h"

struct ActorCollector::PrivateContext {
	DBClientAction dbAction;
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ActorCollector::ActorCollector(void)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

ActorCollector::~ActorCollector()
{
	if (m_ctx)
		delete m_ctx;
}

gpointer ActorCollector::mainThread(HatoholThreadArg *arg)
{
	MLPL_WARN("Not implemented: %s\n", __PRETTY_FUNCTION__);
	return NULL;
}
