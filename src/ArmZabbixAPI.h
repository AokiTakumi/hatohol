#ifndef ArmZabbixAPI_h
#define ArmZabbixAPI_h

#include "ArmBase.h"

class ArmZabbixAPI : public ArmBase
{
public:
	ArmZabbixAPI(CommandLineArg &cmdArg);

protected:
	// virtual methods
	gpointer mainThread(AsuraThreadArg *arg);

private:
	string m_server;
	int    m_server_port;
};

#endif // ArmZabbixAPI_h

