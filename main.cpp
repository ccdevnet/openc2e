/*
 *  main.cpp
 *  openc2e
 *
 *  Created by Alyssa Milburn on Wed 02 Jun 2004.
 *  Copyright (c) 2004-2006 Alyssa Milburn. All rights reserved.
 *  Copyright (c) 2005-2006 Bryan Donlan. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 */

#include "openc2e.h"
#include <iostream>
#include "Engine.h"
#include "World.h"
#include "SDLBackend.h"

#ifdef _MSC_VER
#undef main // because SDL is stupid
#endif

extern "C" int main(int argc, char *argv[]) {
	try {
		std::cout << "openc2e (development build), built " __DATE__ " " __TIME__ "\nCopyright (c) 2004-2006 Alyssa Milburn and others\n\n";

		if (!engine.parseCommandLine(argc, argv)) return 1;
		if (!engine.initialSetup(new SDLBackend())) return 0;
	
		// do a first-pass draw of the world. TODO: correct?
		world.drawWorld();

		while (!engine.done) {
			if (!engine.tick()) // if the engine didn't need an update..
				SDL_Delay(10); // .. delay for a short while
		} // main loop

		engine.backend->shutdown();
	} catch (std::exception &e) {
		std::cerr << "Fatal exception encountered: " << e.what() << "\n";
		return 1;
	}
	return 0;
}

/* vim: set noet: */
