// ========================== WebSocket ==========================
// ================================================================
//
// This file handles live status streaming to the web interface.
// WebSockets are used so the browser receives updates in real time.

#include <WebSocketsServer.h>

WebSocketsServer ws(81); // This is the WebSocket port.
static uint32_t ws_last_ms = 0; // Last broadcast timestamp used for rate limiting.
static int ws_clients = 0; // Count of currently connected WebSocket clients.
void buildStatusJson(JsonDocument& d); // This is a forward declaration for the status builder.

// Send one status payload to a single client.
void wsSendStatus(uint8_t clientNum){
  JsonDocument d; // Scratch JavaScript Object Notation document for one client update.
  buildStatusJson(d); // Populate document with current live status values.
  String out; serializeJson(d, out); // Serialize document to text payload.
  ws.sendTXT(clientNum, out); // Send payload to one WebSocket client.
}
// Broadcast the current status to all connected clients.
void wsBroadcastStatus(){
  if(ws_clients <= 0) return; // Skip work when no browser is connected.
  JsonDocument d; // Scratch JavaScript Object Notation document for broadcast update.
  buildStatusJson(d); // Populate document with current live status values.
  String out; serializeJson(d, out); // Serialize document to text payload.
  ws.broadcastTXT(out); // Broadcast payload to all connected WebSocket clients.
}
// WebSocket connection events (connect, disconnect, message).
void wsEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length){
  (void)payload; (void)length; // Message payload is unused because browser sends no control messages currently.
  if(type == WStype_CONNECTED){
    ws_clients++; // Increment live client count for status broadcast gate.
    wsSendStatus(num); // Push immediate status snapshot to newly connected browser.
  } else if(type == WStype_DISCONNECTED){
    if(ws_clients > 0) ws_clients--; // Decrement live client count without letting count go negative.
  } else if(type == WStype_TEXT){
    // Text messages are currently ignored because the user interface is status-read and config-write via HTTP.
    // Keeping this explicit avoids confusion when packet captures show inbound WebSocket frames.
  }
}
// Initialize the WebSocket server.
void wsBegin(){
  ws.begin(); // Start listening on the configured WebSocket port.
  ws.onEvent(wsEvent); // Register connect and disconnect callback handler.
}
// Poll WebSocket events and rate-limit broadcasts.
void wsLoopTick(){
  ws.loop(); // Service connect, disconnect, and protocol housekeeping.
  if(ws_clients <= 0) return; // Skip status build when no clients are connected.
  const uint32_t now = millis(); // Current time used for broadcast interval control.
  if(now - ws_last_ms < 100) return; // Limit status broadcast to ten frames per second.
  ws_last_ms = now; // Record last successful broadcast timestamp.
  wsBroadcastStatus(); // Push fresh live status to all connected browsers.
}
