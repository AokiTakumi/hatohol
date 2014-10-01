/*
 * Copyright (C) 2014 Project Hatohol
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

#ifndef HapProcessStandard_h
#define HapProcessStandard_h

#include "HapProcess.h"
#include "HatoholArmPluginStandard.h"

class HapProcessStandard : public HapProcess, public HatoholArmPluginStandard {
public:
	HapProcessStandard(int argc, char *argv[]);
	virtual ~HapProcessStandard();
	int mainLoopRun(void);

protected:
	static gboolean acquisitionTimerCb(void *data);
	void startAcquisition(void);
	const MonitoringServerInfo &getMonitoringServerInfo(void) const;

	virtual HatoholError acquireData(void);
	virtual void onCompletedAcquistion(
	  const HatoholError &err, const HatoholArmPluginWatchType &watchType);
	virtual HatoholArmPluginWatchType getHapWatchType(
	  const HatoholError &err);

	virtual void onReady(const MonitoringServerInfo &serverInfo) override;
	virtual int onCaughtException(const std::exception &e) override;

	bool initHapPipe(const std::string &hapPipeName);
	void exitProcess(void);
	static gboolean pipeRdErrCb(
	  GIOChannel *source, GIOCondition condition, gpointer data);
	static gboolean pipeWrErrCb(
	  GIOChannel *source, GIOCondition condition, gpointer data);
private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

#endif /// HapProcessStandard_h

