#pragma once

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <atomic>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "core/enums.hpp"
#include "players/human_player.hpp"
#include "players/player.hpp"

// Dead-simple text protocol (one line per message):
//   Client -> server: JOIN <room>
//   Client -> server: MOVE <turn> <row> <col>
//   Client -> server: BREAK <turn> <row> <col>
//
//   Server -> client: WELCOME <1|2>
//   Server -> client: WAITING
//   Server -> client: START
//   Server -> client: MOVE <turn> <row> <col>
//   Server -> client: BREAK <turn> <row> <col>
//   Server -> client: INFO <free text>
//   Server -> client: ERROR <free text>
//
// Keep it boring. Every packet is human-readable in tcpdump/netcat logs.

namespace netplay {

struct NetAction {
  enum class Kind { Move, Break };

  Kind kind = Kind::Move;
  int turn = 0;
  Coord coord{};
};

class NetworkLink {
 public:
  NetworkLink() = default;
  NetworkLink(const NetworkLink&) = delete;
  NetworkLink& operator=(const NetworkLink&) = delete;

  ~NetworkLink() { disconnect(); }

  bool connectTo(const std::string& host, int port, const std::string& room,
                 int timeoutSeconds = 10) {
    disconnect();
    clearState();

    const int sock = connectSocket(host, port);
    if (sock < 0) {
      pushInfo("[net] connect failed: " + lastError());
      return false;
    }

    m_sock = sock;
    m_connected.store(true);

    if (!sendLine("JOIN " + room)) {
      pushInfo("[net] failed to send JOIN");
      disconnect();
      return false;
    }

    std::string line;
    if (!recvLineBlocking(line, timeoutSeconds)) {
      pushInfo("[net] handshake timed out or failed");
      disconnect();
      return false;
    }

    if (!parseWelcome(line)) {
      pushInfo("[net] bad handshake: " + line);
      disconnect();
      return false;
    }

    m_readerStop.store(false);
    m_reader = std::thread([this]() { readerLoop(); });
    return true;
  }

  void disconnect() {
    m_readerStop.store(true);

    if (m_sock >= 0) {
      ::shutdown(m_sock, SHUT_RDWR);
    }

    if (m_reader.joinable()) {
      m_reader.join();
    }

    if (m_sock >= 0) {
      ::close(m_sock);
      m_sock = -1;
    }

    m_connected.store(false);
  }

  bool isConnected() const { return m_connected.load(); }
  bool peerReady() const { return m_peerReady.load(); }
  bool hasFatalError() const { return m_fatalError.load(); }

  std::string lastError() const {
    std::lock_guard<std::mutex> lock(m_infoMutex);
    return m_lastError;
  }

  Side mySide() const { return m_mySide; }
  Side remoteSide() const {
    return (m_mySide == Side::Player1) ? Side::Player2 : Side::Player1;
  }

  bool sendMove(int turn, Coord c) { return sendAction("MOVE", turn, c); }

  bool sendBreak(int turn, Coord c) { return sendAction("BREAK", turn, c); }

  bool tryPopMove(NetAction& out) {
    return tryPopAction(NetAction::Kind::Move, out);
  }
  bool tryPopBreak(NetAction& out) {
    return tryPopAction(NetAction::Kind::Break, out);
  }

  bool popInfo(std::string& out) {
    std::lock_guard<std::mutex> lock(m_infoMutex);
    if (m_infoLines.empty()) return false;
    out = m_infoLines.front();
    m_infoLines.pop_front();
    return true;
  }

 private:
  int m_sock = -1;
  std::thread m_reader;
  std::atomic<bool> m_readerStop{false};
  std::atomic<bool> m_connected{false};
  std::atomic<bool> m_peerReady{false};
  std::atomic<bool> m_fatalError{false};

  Side m_mySide = Side::Player1;

  mutable std::mutex m_sendMutex;
  mutable std::mutex m_queueMutex;
  mutable std::mutex m_infoMutex;

  std::deque<NetAction> m_moves;
  std::deque<NetAction> m_breaks;
  std::deque<std::string> m_infoLines;
  std::string m_lastError;

  void clearState() {
    std::lock_guard<std::mutex> qlock(m_queueMutex);
    std::lock_guard<std::mutex> ilock(m_infoMutex);
    m_moves.clear();
    m_breaks.clear();
    m_infoLines.clear();
    m_lastError.clear();
    m_peerReady.store(false);
    m_fatalError.store(false);
    m_mySide = Side::Player1;
  }

  static int connectSocket(const std::string& host, int port) {
    struct addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo* result = nullptr;
    const std::string portStr = std::to_string(port);

    if (::getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result) != 0) {
      return -1;
    }

    int sock = -1;
    for (struct addrinfo* rp = result; rp != nullptr; rp = rp->ai_next) {
      sock = ::socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
      if (sock < 0) continue;

      if (::connect(sock, rp->ai_addr, rp->ai_addrlen) == 0) {
        break;
      }

      ::close(sock);
      sock = -1;
    }

    ::freeaddrinfo(result);
    return sock;
  }

  bool recvLineBlocking(std::string& out, int timeoutSeconds) {
    out.clear();

    if (m_sock < 0) return false;

    timeval tv{};
    tv.tv_sec = timeoutSeconds;
    tv.tv_usec = 0;
    ::setsockopt(m_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    char ch = '\0';
    while (true) {
      const ssize_t n = ::recv(m_sock, &ch, 1, 0);
      if (n == 0) return false;
      if (n < 0) return false;
      if (ch == '\n') break;
      if (ch != '\r') out.push_back(ch);
    }

    tv.tv_sec = 0;
    tv.tv_usec = 0;
    ::setsockopt(m_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    return true;
  }

  bool sendAll(const std::string& payload) {
    const char* data = payload.data();
    size_t left = payload.size();

    while (left > 0) {
      const ssize_t n = ::send(m_sock, data, left, 0);
      if (n <= 0) return false;
      data += n;
      left -= static_cast<size_t>(n);
    }
    return true;
  }

  bool sendLine(const std::string& line) {
    std::lock_guard<std::mutex> lock(m_sendMutex);
    if (m_sock < 0) return false;

    const std::string payload = line + "\n";
    if (!sendAll(payload)) {
      setError("send failed");
      return false;
    }
    return true;
  }

  bool parseWelcome(const std::string& line) {
    std::istringstream iss(line);
    std::string tag;
    int side = 0;
    if (!(iss >> tag >> side)) return false;
    if (tag != "WELCOME") return false;
    if (side == 1) {
      m_mySide = Side::Player1;
    } else if (side == 2) {
      m_mySide = Side::Player2;
    } else {
      return false;
    }

    pushInfo("[net] connected as Player " + std::to_string(side));
    return true;
  }

  void setError(const std::string& message) {
    {
      std::lock_guard<std::mutex> lock(m_infoMutex);
      m_lastError = message;
    }
    m_fatalError.store(true);
    pushInfo("[net] ERROR: " + message);
  }

  void pushInfo(const std::string& line) {
    std::lock_guard<std::mutex> lock(m_infoMutex);
    m_infoLines.push_back(line);
    if (m_infoLines.size() > 100) {
      m_infoLines.pop_front();
    }
  }

  void pushAction(const NetAction& action) {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    if (action.kind == NetAction::Kind::Move) {
      m_moves.push_back(action);
    } else {
      m_breaks.push_back(action);
    }
  }

  bool tryPopAction(NetAction::Kind kind, NetAction& out) {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    std::deque<NetAction>& q =
        (kind == NetAction::Kind::Move) ? m_moves : m_breaks;
    if (q.empty()) return false;
    out = q.front();
    q.pop_front();
    return true;
  }

  bool sendAction(const char* kind, int turn, Coord c) {
    std::ostringstream oss;
    oss << kind << ' ' << turn << ' ' << c.row << ' ' << c.col;
    const bool ok = sendLine(oss.str());
    if (ok) {
      pushInfo(std::string("[net] >> ") + oss.str());
    }
    return ok;
  }

  void readerLoop() {
    std::string line;
    while (!m_readerStop.load()) {
      if (!recvLineForever(line)) {
        if (!m_readerStop.load()) {
          setError("connection closed");
        }
        m_connected.store(false);
        return;
      }

      if (!handleIncoming(line)) {
        pushInfo("[net] ignored line: " + line);
      }
    }
  }

  bool recvLineForever(std::string& out) {
    out.clear();
    if (m_sock < 0) return false;

    char ch = '\0';
    while (true) {
      const ssize_t n = ::recv(m_sock, &ch, 1, 0);
      if (n == 0) return false;
      if (n < 0) {
        if (errno == EINTR) continue;
        return false;
      }
      if (ch == '\n') break;
      if (ch != '\r') out.push_back(ch);
    }
    return true;
  }

  bool handleIncoming(const std::string& line) {
    std::istringstream iss(line);
    std::string tag;
    if (!(iss >> tag)) return false;

    if (tag == "WAITING") {
      pushInfo("[net] waiting for second player...");
      return true;
    }

    if (tag == "START") {
      m_peerReady.store(true);
      pushInfo("[net] peer joined; match can start");
      return true;
    }

    if (tag == "INFO") {
      std::string rest;
      std::getline(iss, rest);
      if (!rest.empty() && rest[0] == ' ') rest.erase(0, 1);
      pushInfo("[net] " + rest);
      return true;
    }

    if (tag == "ERROR") {
      std::string rest;
      std::getline(iss, rest);
      if (!rest.empty() && rest[0] == ' ') rest.erase(0, 1);
      setError(rest.empty() ? "unknown server error" : rest);
      return true;
    }

    if (tag == "MOVE" || tag == "BREAK") {
      NetAction action;
      action.kind =
          (tag == "MOVE") ? NetAction::Kind::Move : NetAction::Kind::Break;
      if (!(iss >> action.turn >> action.coord.row >> action.coord.col)) {
        return false;
      }
      pushAction(action);
      pushInfo("[net] << " + line);
      return true;
    }

    return false;
  }
};

// Remote player: waits for network packets and exposes them through Player.
class NetworkPlayer : public Player {
 public:
  explicit NetworkPlayer(NetworkLink& link) : m_link(link) {}
  ~NetworkPlayer() override = default;

  void beginMovePhase(const GameState&) override {
    m_moveReady = false;
    m_breakReady = false;
    m_stagedMove.reset();
    m_stagedBreak.reset();
  }

  void beginBreakPhase(const GameState&) override {
    m_breakReady = false;
    m_stagedBreak.reset();
  }

  void update(int, const GameState& state) override {
    if (!m_link.isConnected()) return;

    if (state.phase() == TurnPhase::Move && !m_moveReady) {
      NetAction action;
      if (m_link.tryPopMove(action)) {
        m_stagedMove = action.coord;
        m_moveReady = true;
      }
      return;
    }

    if (state.phase() == TurnPhase::Break && !m_breakReady) {
      NetAction action;
      if (m_link.tryPopBreak(action)) {
        m_stagedBreak = action.coord;
        m_breakReady = true;
      }
    }
  }

  bool hasMoveReady() const override { return m_moveReady; }

  Coord consumeMove() override {
    m_moveReady = false;
    return m_stagedMove.value_or(Coord{});
  }

  bool hasBreakReady() const override { return m_breakReady; }

  Coord consumeBreak() override {
    m_breakReady = false;
    return m_stagedBreak.value_or(Coord{});
  }

 private:
  NetworkLink& m_link;
  bool m_moveReady = false;
  bool m_breakReady = false;
  std::optional<Coord> m_stagedMove;
  std::optional<Coord> m_stagedBreak;
};

// Local human player: behaves exactly like HumanPlayer, but sends the chosen
// move/break out over the same NetworkLink when consumed by MatchSession.
class NetworkHumanPlayer : public HumanPlayer {
 public:
  explicit NetworkHumanPlayer(NetworkLink& link) : m_link(link) {}
  ~NetworkHumanPlayer() override = default;

  void beginMovePhase(const GameState& state) override {
    HumanPlayer::beginMovePhase(state);
    ++m_turnNumber;
  }

  void beginBreakPhase(const GameState& state) override {
    HumanPlayer::beginBreakPhase(state);
  }

  Coord consumeMove() override {
    const Coord c = HumanPlayer::consumeMove();
    m_link.sendMove(m_turnNumber, c);
    return c;
  }

  Coord consumeBreak() override {
    const Coord c = HumanPlayer::consumeBreak();
    m_link.sendBreak(m_turnNumber, c);
    return c;
  }

 private:
  NetworkLink& m_link;
  int m_turnNumber = 0;
};

}  // namespace netplay

/*
Quick integration sketch
------------------------

#include "players/network_player.hpp"

netplay::NetworkLink link;
if (!link.connectTo("YOUR_SERVER_IP", 5050, "room-123")) {
  // print link.lastError()
}

// Wait for second player before starting the match if you want a cleaner UX.
// while (!link.peerReady()) { ... draw "waiting" screen ... }

Player* p1 = nullptr;
Player* p2 = nullptr;
if (link.mySide() == Side::Player1) {
  p1 = new netplay::NetworkHumanPlayer(link);
  p2 = new netplay::NetworkPlayer(link);
} else {
  p1 = new netplay::NetworkPlayer(link);
  p2 = new netplay::NetworkHumanPlayer(link);
}

MatchSession session(9, 11, p1, p2);

// Inside main loop, surface network log lines into the HUD:
// std::string msg;
// while (link.popInfo(msg)) session.postUiMessage(msg);
*/
