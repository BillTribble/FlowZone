#include "WebSocketServer.h"
#include "../FileLogger.h"

WebSocketServer::WebSocketServer(int p) : port(p) {}

WebSocketServer::~WebSocketServer() { stop(); }

void WebSocketServer::start() {
  if (ctx != nullptr)
    return;

  std::string portStr = std::to_string(port);
  std::vector<const char *> options;
  options.push_back("listening_ports");
  options.push_back(portStr.c_str());
  options.push_back("websocket_timeout_ms");
  options.push_back("3600000");

  if (!documentRoot.empty()) {
    options.push_back("document_root");
    options.push_back(documentRoot.c_str());
  }

  options.push_back(nullptr);

  struct mg_callbacks callbacks;
  memset(&callbacks, 0, sizeof(callbacks));

  ctx = mg_start(&callbacks, this, options.data());

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

void WebSocketServer::broadcast(const std::string &message) {
  static int broadcastCounter = 0;
  std::lock_guard<std::mutex> lock(connectionsMutex);
  flowzone::FileLogger::instance().logSampled(
      flowzone::FileLogger::Category::WebSocket,
      "BROADCAST to " + std::to_string(connections.size()) +
          " clients, " + std::to_string(message.length()) + " bytes",
      broadcastCounter, 60);
  for (auto *conn : connections) {
    mg_websocket_write(conn, MG_WEBSOCKET_OPCODE_TEXT, message.c_str(),
                       message.length());
  }
}

void WebSocketServer::setDocumentRoot(const std::string &path) {
  documentRoot = path;
}

void WebSocketServer::setInitialStateCallback(
    std::function<std::string()> callback) {
  getInitialState = callback;
}

void WebSocketServer::setOnMessageCallback(
    std::function<void(const std::string &)> callback) {
  onMessage = callback;
}

// Static Handlers
int WebSocketServer::websocket_connect_handler(const struct mg_connection *conn,
                                               void *cbdata) {
  auto *server = static_cast<WebSocketServer *>(cbdata);
  return server->onConnect(conn);
}

void WebSocketServer::websocket_ready_handler(struct mg_connection *conn,
                                              void *cbdata) {
  auto *server = static_cast<WebSocketServer *>(cbdata);
  server->onReady(conn);
}

int WebSocketServer::websocket_data_handler(struct mg_connection *conn,
                                            int bits, char *data, size_t len,
                                            void *cbdata) {
  auto *server = static_cast<WebSocketServer *>(cbdata);
  return server->onData(conn, bits, data, len);
}

void WebSocketServer::websocket_close_handler(const struct mg_connection *conn,
                                              void *cbdata) {
  auto *server = static_cast<WebSocketServer *>(cbdata);
  server->onClose(conn);
}

// Instance Handlers
int WebSocketServer::onConnect(const struct mg_connection *conn) {
  juce::ignoreUnused(conn);
  return 0; // Accept
}

void WebSocketServer::onReady(struct mg_connection *conn) {
  {
    std::lock_guard<std::mutex> lock(connectionsMutex);
    connections.insert(conn);
  }

  flowzone::FileLogger::instance().log(
      flowzone::FileLogger::Category::WebSocket,
      "CLIENT CONNECTED, total=" + std::to_string(connections.size()));

  if (getInitialState) {
    std::string stateJson = getInitialState();
    flowzone::FileLogger::instance().log(
        flowzone::FileLogger::Category::WebSocket,
        "SENDING INITIAL STATE, " + std::to_string(stateJson.length()) + " bytes");
    mg_websocket_write(conn, MG_WEBSOCKET_OPCODE_TEXT, stateJson.c_str(),
                       stateJson.length());
  } else {
    flowzone::FileLogger::instance().log(
        flowzone::FileLogger::Category::WebSocket,
        "WARNING: No initial state callback set!");
    const char *msg = "{\"error\": \"No state callback set\"}";
    mg_websocket_write(conn, MG_WEBSOCKET_OPCODE_TEXT, msg, strlen(msg));
  }
}

int WebSocketServer::onData(struct mg_connection *conn, int bits, char *data,
                            size_t len) {
  juce::ignoreUnused(conn, bits);
  if (onMessage) {
    std::string msg(data, len);
    flowzone::FileLogger::instance().log(
        flowzone::FileLogger::Category::WebSocket,
        "RECEIVED CMD: " + msg.substr(0, 120));
    onMessage(msg);
  }
  return 1; // Keep open
}

void WebSocketServer::onClose(const struct mg_connection *conn) {
  std::lock_guard<std::mutex> lock(connectionsMutex);
  connections.erase(const_cast<struct mg_connection *>(conn));
  flowzone::FileLogger::instance().log(
      flowzone::FileLogger::Category::WebSocket,
      "CLIENT DISCONNECTED, remaining=" + std::to_string(connections.size()));
}
