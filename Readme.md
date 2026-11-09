Web Server

A lightweight web server written in C, using OpenSSL for HTTPS and epoll for handling client connections.

Features
HTTPS support using OpenSSL
Non-blocking I/O with epoll
Dynamic plugin support
C-based API modules
Static file serving
Lightweight and fast
Requirements
Linux
GCC
Make
OpenSSL development libraries
Build

Clone the repository:

```bash
git clone https://github.com/TTboss90/web-server.git
cd web-server
```


Build the project:

```bash
make build
```
Run

Start the server:

```bash
make run
```

Clean build files:

```bash
make clean
```
Project Structure
src/ – Core web server source code
html/ – Website files and API modules
plugins/ – Server plugins
out_files/ – Additional output files
out/ – Build output