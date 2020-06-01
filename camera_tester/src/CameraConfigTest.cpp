// Copyright 2020 Augmented Perception Corporation

#include <windows.h>
#include <Hidsdi.h> // DDK

#include "core_logger.hpp"

using namespace core;

static logger::Channel Logger("Config");


class HidDevice
{
public:
    bool Open();
};



int main(int argc, const char* argv[])
{
	(void)argc;
	(void)argv;

	Logger.Info("Starting");

	return 0;
}
