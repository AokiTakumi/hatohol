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

enum OperationPrivilegeType
{
	OPPRVLG_CREATE_USER,
	OPPRVLG_UPDATE_USER,
	OPPRVLG_DELETE_USER,
	OPPRVLG_GET_ALL_USERS,
	OPPRVLG_CREATE_SERVERS,
	OPPRVLG_UPDATE_SERVERS,
	OPPRVLG_UPDATE_ALL_SERVERS,
	OPPRVLG_DELETE_SERVERS,
	OPPRVLG_DELETE_ALL_SERVERS,
	OPPRVLG_GET_ALL_SERVERS,
	NUM_OPPRVLG,
};

typedef uint64_t OperationPrivilegeFlag;
const static OperationPrivilegeFlag NONE_PRIVILEGE = 0;
#define FMT_OPPRVLG PRIu64

class OperationPrivilege {
public:
	OperationPrivilege(const OperationPrivilegeFlag flags = 0);
	OperationPrivilege(const OperationPrivilege &src);
	virtual ~OperationPrivilege();

	const OperationPrivilegeFlag &getFlags(void) const;
	void setFlags(const OperationPrivilegeFlag flags);
	static const OperationPrivilegeFlag 
	  makeFlag(OperationPrivilegeType type);
	const bool has(OperationPrivilegeType type) const;

	bool operator==(const OperationPrivilege &rhs);

	void setUserId(UserIdType userId);
	UserIdType getUserId(void) const;

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

const static OperationPrivilegeFlag ALL_PRIVILEGES = 
  OperationPrivilege::makeFlag(NUM_OPPRVLG) - 1;

#endif // OperationPrivilege_h
