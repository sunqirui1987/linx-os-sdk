// Mongoose configuration file
// This file contains configuration options for the Mongoose library

#ifndef MONGOOSE_CONFIG_H
#define MONGOOSE_CONFIG_H

// Include stddef.h for size_t definition
#include <stddef.h>

// Define MG_ARCH for the platform
#if defined(__APPLE__)
  #define MG_ARCH MG_ARCH_UNIX
#elif defined(ESP_PLATFORM)
  #define MG_ARCH MG_ARCH_ESP32
#elif defined(__linux__)
  #define MG_ARCH MG_ARCH_UNIX
#else
  #define MG_ARCH MG_ARCH_UNIX  // Default to UNIX architecture
#endif

// Enable WebSocket support
#define MG_ENABLE_WEBSOCKET 1

// Enable HTTP support
#define MG_ENABLE_HTTP 1

// Enable SSL/TLS support if needed
// #define MG_ENABLE_SSL 1

// Enable IPv6 support
#define MG_ENABLE_IPV6 1

// Enable debugging
#define MG_ENABLE_DEBUG 0

// Enable filesystem access
#define MG_ENABLE_FILESYSTEM 1

// Enable directory listing
#define MG_ENABLE_DIRECTORY_LISTING 1

// Enable HTTP digest authentication
#define MG_ENABLE_HTTP_DIGEST_AUTH 1

// Enable HTTP basic authentication
#define MG_ENABLE_HTTP_BASIC_AUTH 1

// Enable HTTP SSI (Server Side Includes)
#define MG_ENABLE_SSI 1

// Enable HTTP CGI (Common Gateway Interface)
#define MG_ENABLE_HTTP_CGI 1

// Enable HTTP WebDAV
#define MG_ENABLE_HTTP_WEBDAV 0

// Enable HTTP URL rewrites
#define MG_ENABLE_HTTP_URL_REWRITES 1

// Enable HTTP multipart
#define MG_ENABLE_HTTP_MULTIPART 1

// Enable HTTP streaming
#define MG_ENABLE_HTTP_STREAMING_MULTIPART 1

#endif // MONGOOSE_CONFIG_H