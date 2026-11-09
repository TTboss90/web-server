CC = gcc
CFLAGS = -Wall -Wextra 
LDLIBS = -lssl -lcrypto
PICFLAGS = -fPIC -shared

T = out/webserver

SRC = $(wildcard src/*.c)
OBJ = $(patsubst src/%.c,out/%.o,$(SRC))

API_SRC = $(shell find html -type f -name '*.c')
API_LIB = $(patsubst html/%.c,out/html/%.so,$(API_SRC))

PLUGIN_SRC = $(shell find plugins -type f -name '*.c')
PLUGIN_LIB = $(patsubst plugins/%.c,out/plugins/%.so,$(PLUGIN_SRC))

build: $(T) build_site

$(T): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

out/%.o: src/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

out/html/%.so: html/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(PICFLAGS) -I./src -I./plugins $< -o $@ $(LDLIBS)

out/plugins/%.so: plugins/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(PICFLAGS) -I./plugins $< -o $@ $(LDLIBS)

build_site: $(API_LIB) html_folder build_plugin out_files

build_plugin: $(PLUGIN_LIB) plugin_folder

html_folder:
	mkdir -p out/html
	cp -r html/. out/html/
	find out/html -name '*.c' -delete

plugin_folder:
	mkdir -p out/plugins
	cp -r plugins/. out/plugins/
	find out/plugins -name '*.c' -delete

out_files:
	cp -r out_files/. out/

clean:
	rm -rf out

run: build
	cd out && ./webserver $(args)

run_site: build_site
	cd out && ./webserver $(args)

.PHONY: run clean plugin_folder html html_folder build build_plugin build_site run_site out_files