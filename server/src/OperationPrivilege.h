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

#ifndef OperationPrivilege_h
#define OperationPrivilege_h

#include <string>
#include "Params.h"

enum OprationPrivilegeType
{
	OPPRVLG_CREATE_USER,
};

typedef uint64_t OperationPrivilegeFlag;

class OperationPrivilege {
public:
	OperationPrivilege(void);
	virtual ~OperationPrivilege();

	const OperationPrivilegeFlag &getFlags(void) const;

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // OperationPrivilege_h
