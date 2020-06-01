// Copyright 2019 Augmented Perception Corporation

#include "CameraClient.hpp"
#include "core_logger.hpp"

#include <iostream>
using namespace std;

using namespace core;

static logger::Channel Logger("CameraTester");

int main(int argc, const char* argv[])
{
	(void)argc;
	(void)argv;

	Logger.Info("Starting");

	CameraClient client;

	if (!client.Start()) {
		Logger.Error("Failed to start");
		return -1;
	}

	Logger.Info("Press any key to stop");
	string pause;
	cin >> pause;

	Logger.Info("Stopping");
	client.Stop();

	Logger.Info("Terminated");
	return 0;
}
