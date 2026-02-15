#pragma once

#include "civetweb.h"
#include <JuceHeader.h>
#include <string>
#include <vector>

#include <functional>
#include <mutex>
#include <set>

class WebSocketServer {
public:
  WebSocketServer(int port);
  ~WebSocketServer();

  void start();
  void stop();

  // Send a message to all connected clients
  void broadcast(const std::string &message);

  // Func returning the initial state as JSON string
  void setInitialStateCallback(std::function<std::string()> callback);

  // Func handling received messages
  void setOnMessageCallback(std::function<void(const std::string &)> callback);

private:
  int port;
  struct mg_context *ctx = nullptr;

  std::mutex connectionsMutex;
  std::set<struct mg_connection *> connections;

  std::function<std::string()> getInitialState;
  std::function<void(const std::string &)> onMessage;

  // Static handlers that forward to instance methods
  static int websocket_connect_handler(const struct mg_connection *conn,
                                       void *cbdata);
  static void websocket_ready_handler(struct mg_connection *conn, void *cbdata);
  static int websocket_data_handler(struct mg_connection *conn, int bits,
                                    char *data, size_t len, void *cbdata);
  static void websocket_close_handler(const struct mg_connection *conn,
                                      void *cbdata);

  // Instance handlers
  int onConnect(const struct mg_connection *conn);
  void onReady(struct mg_connection *conn);
  int onData(struct mg_connection *conn, int bits, char *data, size_t len);
  void onClose(const struct mg_connection *conn);
};
