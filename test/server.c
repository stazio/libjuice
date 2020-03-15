/**
 * Copyright (c) 2020 Paul-Louis Ageneau
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "juice/juice.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
static void sleep(unsigned int secs) { Sleep(secs * 1000); }
#else
#include <unistd.h> // for sleep
#endif

static juice_server_t *server;

int test_server() {
	juice_set_log_level(JUICE_LOG_LEVEL_VERBOSE);

	// Create server
	juice_server_credentials_t credentials[1];
	memset(&credentials, 0, sizeof(credentials));
	credentials[0].username = "test";
	credentials[0].password = "123456";

	juice_server_config_t config;
	memset(&config, 0, sizeof(config));
	config.credentials = credentials;
	config.credentials_count = 1;
	config.max_allocations = 100;
	config.realm = "Juice test server";
	server = juice_server_create(6666, &config);

	sleep(10);

	// Destroy
	juice_server_destroy(server);

	// Sleep so we can check destruction went well
	sleep(2);

	return 0;
}

