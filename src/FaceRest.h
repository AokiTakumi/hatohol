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

#ifndef FaceRest_h
#define FaceRest_h

#include <libsoup/soup.h>
#include "FaceBase.h"
#include "JsonBuilderAgent.h"

class FaceRest : public FaceBase {
public:
	static void init(void);
	FaceRest(CommandLineArg &cmdArg);
	virtual ~FaceRest();
	virtual void stop(void);

protected:
        struct HandlerArg;

	// virtual methods
	gpointer mainThread(AsuraThreadArg *arg);

	// generic sub routines
	size_t parseCmdArgPort(CommandLineArg &cmdArg, size_t idx);
	static void replyError(SoupMessage *msg, const string &errorMessage);
	static string getJsonpCallbackName(GHashTable *query, HandlerArg *arg);
	static string wrapForJsonp(const string &jsonBody,
                                   const string &callbackName);
	static void replyJsonData(JsonBuilderAgent &agent, SoupMessage *msg,
	                          const string &jsonpCallbackName,
	                          HandlerArg *arg);

	// handlers
	static void
	  handlerDefault(SoupServer *server, SoupMessage *msg,
	                 const char *path, GHashTable *query,
	                 SoupClientContext *client, gpointer user_data);
	static void launchHandlerInTryBlock
	  (SoupServer *server, SoupMessage *msg, const char *path,
	   GHashTable *query, SoupClientContext *client, gpointer user_data);

	static void handlerHelloPage
	  (SoupServer *server, SoupMessage *msg, const char *path,
	   GHashTable *query, SoupClientContext *client, HandlerArg *arg);
	static void handlerGetServers
	  (SoupServer *server, SoupMessage *msg, const char *path,
	   GHashTable *query, SoupClientContext *client, HandlerArg *arg);
	static void handlerGetTriggers
	  (SoupServer *server, SoupMessage *msg, const char *path,
	   GHashTable *query, SoupClientContext *client, HandlerArg *arg);
	static void handlerGetEvents
	  (SoupServer *server, SoupMessage *msg, const char *path,
	   GHashTable *query, SoupClientContext *client, HandlerArg *arg);

private:
	static const char *pathForGetServers;
	static const char *pathForGetTriggers;
	static const char *pathForGetEvents;

	guint       m_port;
	SoupServer *m_soupServer;
};

#endif // FaceRest_h
