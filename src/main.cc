#include <cstdio>
#include <cstdlib>
#include <glib.h>

#include <string>
#include <vector>
using namespace std;

#include <Logger.h>
using namespace mlpl;

#include "Asura.h"
#include "Utils.h"
#include "FaceMySQL.h"
#include "ArmController.h"

int main(int argc, char *argv[])
{
	g_type_init();
	asuraInit();
	MLPL_INFO("started asura: ver. %s\n", PACKAGE_VERSION);

	CommandLineArg cmdArg;
	for (int i = 1; i < argc; i++)
		cmdArg.push_back(argv[i]);

	FaceMySQL face(cmdArg);
	face.start();

	ArmController armController(cmdArg);
	armController.start();

	GMainLoop *loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(loop);
	return EXIT_SUCCESS;
}
