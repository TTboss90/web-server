#define header_funcs
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <errno.h>
#include <fcntl.h>
#include "socket.h"
#include "server.h"
#include "response.h"
#include "header.h"
#include <dlfcn.h>
#include <dirent.h>
#include <limits.h> 
#include <signal.h>
#include "plugins.h"

#define MAX_PLUGINS 64

void *plugins[MAX_PLUGINS] = { 0 };
int plugin_count = 0;

void load_plugins()
{
    DIR *dir = opendir("./plugins");
    if (!dir) {
        perror("plugins folder");
        return 1;
    }

	struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        const char *name = ent->d_name;
        size_t len = strlen(name);
		if (len < 3 || strcmp(name + len - 3, ".so") != 0)
        	continue;
		if (plugin_count >= MAX_PLUGINS) {
       	 	fprintf(stderr, "Too many plugins (max %d)\nstoped at: %s", MAX_PLUGINS, name);
        	break;
    	}
		
		char path[PATH_MAX];
    	snprintf(path, sizeof(path), "./plugins/%s", name);

		plugins[plugin_count] = dlopen(path, RTLD_LAZY | RTLD_GLOBAL);
		if (!plugins[plugin_count]) {
			fprintf(stderr, "Can't load plugin %s: %s\n", path, dlerror());
			exit(1);
		}
		dlerror();

		int (*func)(void) = (int (*)(void)) dlsym(plugins[plugin_count], "_pinit");

		char *err = dlerror();
		if (err) {
			fprintf(stderr, "Plugin %d: %s\n", plugin_count, err);
			exit(1);
		}

		if(func()){
			fprintf(stderr, "Plugin %d error\n", plugin_count);
			exit(1);
		}

        plugin_count++;
    }
}

void close_plugins()
{
    for (int i = 0; i < plugin_count; i++) {
		int (*func)(void) = (int (*)(void)) dlsym(plugins[i], "_pclose");

		char *err = dlerror();
		if (err) {
			fprintf(stderr, "Plugin %d: %s\n", i, err);
			continue;
		}

		if(func()){
			fprintf(stderr, "Plugin %d error\n", i);
			continue;
		}
	}

	for (int i = 0; i < plugin_count; i++) {
		dlclose(plugins[i]);
	}
}