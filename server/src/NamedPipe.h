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

#ifndef NamedPipe_h
#define NamedPipe_h

#include <string>
#include <glib.h>
#include "SmartBuffer.h"

class NamedPipe {
public:
	enum EndType {
		END_TYPE_MASTER_READ,
		END_TYPE_MASTER_WRITE,
		END_TYPE_SLAVE_READ,
		END_TYPE_SLAVE_WRITE,
	};

	static const char *BASE_DIR;

	NamedPipe(EndType endType);
	virtual ~NamedPipe();
	bool openPipe(const std::string &name);

	/**
	 * Create a GIOChannel object and register an event callback.
	 * openPipe() must be called before the call of this function.
	 *
	 * @parameter iochCb
	 * A callback handler on the pipe's event.
	 * If the instance type is END_TYPE_MASTER_READ or 
	 * END_TYPE_SLAVE_READ, it is called on one of the events:
	 * G_IO_IN, G_IO_PRI, G_IO_ERR, G_IO_HUP, and G_IO_NVAL.
	 * If the instance type is END_TYPE_MASTER_WRITE or 
	 * END_TYPE_SLAVE_WRITE, it is call on one of the events:
	 * G_IO_ERR, G_IO_HUP, and G_IO_NVAL.
	 *
	 * @parameter data
	 * An arbitray user data that is passed as an argument of the callback.
	 *
	 * @return
	 * If no error occurs, true is returned. Otherwise false.
	 */
	bool createGIOChannel(GIOFunc iochCb, gpointer data);

	int getFd(void) const;
	const std::string &getPath(void) const;

	/**
	 * Push the buffer to the internal queue. After the call,
	 * the content of the buffer is taken over (i.e. the buffer in
	 * 'buf' is moved).
	 * The function is asynchronus and returns without blocking.
	 *
	 * @param buf A SmartBuffer instance that has data to be written.
	 */
	void push(mlpl::SmartBuffer &buf);

protected:
	static gboolean writeCb(GIOChannel *source, GIOCondition condition,
	                        gpointer data);
	/**
	 * Write data to the pipe.
	 *
	 * @parameter buf
	 * A SmartBuffer instance that has data to be written.
	 *
	 * @parameter fullyWritten
	 * When the all data in \buf was written, this function set
	 * true to the parameter. Otherwise it sets false.
 	 *
	 * @return
	 * If no error occurs, true is returned. Otherwise false.
	 */
	bool writeBuf(mlpl::SmartBuffer &buf, bool &fullyWritten);

	void enableWriteCbIfNeeded(void);
	bool isExistingDir(const std::string &dirname, bool &hasError);
	bool makeBasedirIfNeeded(const std::string &baseDir);
	bool deleteFileIfExists(const std::string &path);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // NamedPipe_h
