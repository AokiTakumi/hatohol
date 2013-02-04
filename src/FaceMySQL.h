/* Asura
   Copyright (C) 2013 MIRACLE LINUX CORPORATION
 
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef FaceMySQL_h
#define FaceMySQL_h

#include <vector>
#include <string>
using namespace std;

#include <gio/gio.h>
#include "Utils.h"
#include "FaceBase.h"

class FaceMySQL : public FaceBase {
	GSocket *m_socket;
	int      m_port;

	size_t parseCmdArgPort(CommandLineArg &cmdArg, size_t idx);
protected:
	// virtual methods
	gpointer mainThread(AsuraThreadArg *arg);

public:
	FaceMySQL(CommandLineArg &cmdArg);
	virtual ~FaceMySQL();
};

#endif // FaceMySQL_h
