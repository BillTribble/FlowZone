#include "WebSocketServer.h"

WebSocketServer::WebSocketServer(int p) : port(p) {}

WebSocketServer::~WebSocketServer() { stop(); }

void WebSocketServer::start() {
  if (ctx != nullptr)
    return;

  const char *options[] = {"listening_ports", std::to_string(port).c_str(),
                           "websocket_timeout_ms", "3600000", nullptr};

  struct mg_callbacks callbacks;
  memset(&callbacks, 0, sizeof(callbacks));

  ctx = mg_start(&callbacks, this, options);

  // Set WebSocket handlers for root path "/"
  mg_set_websocket_handler(ctx, "/", websocket_connect_handler,
                           websocket_ready_handler, websocket_data_handler,
                           websocket_close_handler, this);
}

void WebSocketServer::stop() {
  if (ctx) {
    mg_stop(ctx);
    ctx = nullptr;
  }
}

int WebSocketServer::websocket_connect_handler(const struct mg_connection *conn,
                                               void *cbdata) {
  juce::ignoreUnused(conn, cbdata);
  return 0; // Return 0 to accept connection
}

void WebSocketServer::websocket_ready_handler(struct mg_connection *conn,
                                              void *cbdata) {
  juce::ignoreUnused(cbdata);
  const char *msg = "Hello from FlowZone Engine";
  mg_websocket_write(conn, MG_WEBSOCKET_OPCODE_TEXT, msg, strlen(msg));
}

int WebSocketServer::websocket_data_handler(struct mg_connection *conn,
                                            int bits, char *data, size_t len,
                                            void *cbdata) {
  juce::ignoreUnused(conn, bits, data, len, cbdata);
  return 1; // Keep connection open
}

void WebSocketServer::websocket_close_handler(const struct mg_connection *conn,
                                              void *cbdata) {
  juce::ignoreUnused(conn, cbdata);
}
