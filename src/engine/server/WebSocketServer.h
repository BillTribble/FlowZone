#pragma once

#include "civetweb.h"
#include <JuceHeader.h>
#include <string>
#include <vector>

class WebSocketServer {
public:
  WebSocketServer(int port = 50001);
  ~WebSocketServer();

  void start();
  void stop();

private:
  static int websocket_connect_handler(const struct mg_connection *conn,
                                       void *cbdata);
  static void websocket_ready_handler(struct mg_connection *conn, void *cbdata);
  static int websocket_data_handler(struct mg_connection *conn, int bits,
                                    char *data, size_t len, void *cbdata);
  static void websocket_close_handler(const struct mg_connection *conn,
                                      void *cbdata);

  struct mg_context *ctx = nullptr;
  int port;
};
