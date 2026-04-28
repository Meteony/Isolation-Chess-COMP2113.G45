#include <ncurses.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <vector>

#include "core/replay_data.hpp"
#include "misc/blizzard_transition.hpp"
#include "misc/settings_io.hpp"
#include "scenes/live_match_scene.hpp"
#include "scenes/netplay_scene.hpp"
#include "scenes/replay_browser.hpp"
#include "scenes/replay_scene.hpp"
#include "scenes/scene_common.hpp"
#include "sessions/replay_session.hpp"
#include "ui/board_renderer.hpp"
#include "ui/ui_colors.hpp"

namespace {

constexpr int kCanvasMaxRows = 21;
constexpr int kCanvasMaxCols = 71;
constexpr int kTopAreaMaxRows = 6;
constexpr int kLeftBoxMaxCols = 32;
constexpr int kRightBoxMaxCols = 39;
constexpr int kPreviewMaxRows = 14;
constexpr int kPreviewMaxCols = 39;
constexpr int kButtonWidth = 28;

struct Layout {
  int rows = kCanvasMaxRows;
  int cols = kCanvasMaxCols;

  int topRows = kTopAreaMaxRows;
  int infoLeft = 47;
  int infoWidth = 24;
  int middleTop = 6;
  int middleRows = 14;
  int leftWidth = 32;
  int rightLeft = 32;
  int rightWidth = 39;
  int keyTipRow = 20;

  int previewRows = 14;
  int previewCols = 39;
};

enum class FocusPane { Hud, Replay };

enum class MenuPage { Main, ReplayBrowser, Settings };

enum class MenuAction {
  None,
  HumanVsHuman,
  CpuEasy,
  CpuMedium,
  CpuHard,
  Netplay,
  Replay,
  Exit,
  OpenReplayBrowser,
  OpenSettings,
  EditRoom,
  EditTag,
  EditServerIp,
  EditServerPort,
  Back,
  Spacer,
};

struct MenuEntry {
  std::string label;
  MenuAction action = MenuAction::None;
  bool selectable = true;
};

struct LaunchRequest {
  MenuAction action = MenuAction::None;
  std::string replayPath;
};

std::string padRight(std::string s, int width) {
  if (width <= 0) return "";
  if (static_cast<int>(s.size()) >= width) return s.substr(0, width);
  s.append(width - static_cast<int>(s.size()), ' ');
  return s;
}

std::string truncateToWidth(const std::string& s, int width) {
  if (width <= 0) return "";
  if (static_cast<int>(s.size()) <= width) return s;
  if (width <= 3) return s.substr(0, width);
  return s.substr(0, width - 3) + "...";
}

bool parseIntStrict(const std::string& s, int& outValue) {
  if (s.empty()) return false;
  try {
    std::size_t pos = 0;
    int value = std::stoi(s, &pos);
    if (pos != s.size()) return false;
    outValue = value;
    return true;
  } catch (...) {
    return false;
  }
}

bool isValidIpv4(const std::string& ip) {
  if (ip.empty()) return false;
  int parts = 0;
  std::string cur;
  for (char ch : ip) {
    if (ch == '.') {
      int value = 0;
      if (cur.empty() || !parseIntStrict(cur, value) || value < 0 ||
          value > 255) {
        return false;
      }
      cur.clear();
      ++parts;
      continue;
    }
    if (!std::isdigit(static_cast<unsigned char>(ch))) return false;
    cur.push_back(ch);
  }
  int value = 0;
  if (cur.empty() || !parseIntStrict(cur, value) || value < 0 || value > 255) {
    return false;
  }
  ++parts;
  return parts == 4;
}

bool isValidServerIp(const std::string& ip) {
  return ip == "localhost" || isValidIpv4(ip);
}

bool isValidServerPort(int port) { return port >= 1 && port <= 65535; }

bool isValidTag(const std::string& tag) {
  if (tag.empty() || tag.size() > 24) return false;
  for (char ch : tag) {
    const unsigned char uch = static_cast<unsigned char>(ch);
    if (!(std::isalnum(uch) || ch == '_' || ch == '-' || ch == '.')) {
      return false;
    }
  }
  return true;
}

bool isValidRoomCode(const std::string& room) {
  if (room.empty() || room.size() > 32) return false;
  for (char ch : room) {
    const unsigned char uch = static_cast<unsigned char>(ch);
    if (!(std::isalnum(uch) || ch == '_' || ch == '-' || ch == '.')) {
      return false;
    }
  }
  return true;
}

Layout computeLayout() {
  Layout layout;
  int maxRows = 0;
  int maxCols = 0;
  getmaxyx(stdscr, maxRows, maxCols);

  layout.rows = std::min(kCanvasMaxRows, std::max(12, maxRows));
  layout.cols = std::min(kCanvasMaxCols, std::max(40, maxCols));

  layout.topRows = std::clamp(layout.rows - 15, 5, kTopAreaMaxRows);
  layout.middleTop = layout.topRows;
  layout.middleRows = std::max(6, layout.rows - layout.topRows - 1);
  layout.keyTipRow = layout.rows - 1;

  layout.infoWidth = std::clamp(layout.cols / 3, 18, 24);
  layout.infoLeft = std::max(0, layout.cols - layout.infoWidth);

  layout.rightWidth = std::clamp(layout.cols - kLeftBoxMaxCols, 18, 39);
  layout.leftWidth = layout.cols - layout.rightWidth;
  if (layout.leftWidth < 24) {
    layout.leftWidth = 24;
    layout.rightWidth = std::max(16, layout.cols - layout.leftWidth);
  }
  layout.rightLeft = layout.leftWidth;

  layout.previewRows = std::min(layout.middleRows, kPreviewMaxRows);
  layout.previewCols = std::min(layout.rightWidth, kPreviewMaxCols);
  return layout;
}

void drawFrameBox(int top, int left, int height, int width,
                  const std::string& title, bool focused = false,
                  bool dimWhenUnfocused = true) {
  if (height < 2 || width < 2) return;

  ensureUiColorsInitialized();
  const int pair = focused ? CP_FRAME_FOCUSED : CP_FRAME;
  if (!focused && dimWhenUnfocused) attron(A_DIM);
  attron(COLOR_PAIR(pair));

  mvaddstr(top, left, "╭");
  mvaddstr(top, left + width - 1, "╮");
  mvaddstr(top + height - 1, left, "╰");
  mvaddstr(top + height - 1, left + width - 1, "╯");

  for (int c = 1; c < width - 1; ++c) {
    mvaddstr(top, left + c, "─");
    mvaddstr(top + height - 1, left + c, "─");
  }
  for (int r = 1; r < height - 1; ++r) {
    mvaddstr(top + r, left, "│");
    mvaddstr(top + r, left + width - 1, "│");
  }

  if (!title.empty() && width > 4) {
    mvaddnstr(top, left + 2, title.c_str(), width - 4);
  }

  attroff(COLOR_PAIR(pair));
  if (!focused && dimWhenUnfocused) attroff(A_DIM);
}

void drawScrollbar(int top, int col, int height, int visibleTop,
                   int visibleCount, int totalLines) {
  if (height <= 0) return;
  if (totalLines <= visibleCount || totalLines <= 0) {
    for (int i = 0; i < height; ++i) {
      mvaddch(top + i, col, ' ');
    }
    return;
  }

  const int maxScroll = totalLines - visibleCount;
  const int thumbHeight = std::max(1, (height * visibleCount) / totalLines);
  const int thumbTravel = std::max(0, height - thumbHeight);
  const int thumbTop =
      top + (maxScroll == 0 ? 0 : (thumbTravel * visibleTop) / maxScroll);

  attron(COLOR_PAIR(CP_FRAME));
  attron(A_DIM);
  for (int i = 0; i < height; ++i) mvaddch(top + i, col, ' ');
  attroff(A_DIM);
  for (int i = 0; i < thumbHeight; ++i) mvaddch(thumbTop + i, col, ACS_VLINE);
  attroff(COLOR_PAIR(CP_FRAME));
}

// Unused
/*
std::vector<std::string> logoLines() {
  return {
      "  ________            ______                    ",
      "  ____  _/_______________  /     __  _          ",
      "   __  / __  ___/  __ \\_  /__ _/ /_(_)__  ___   ",
      "  __/ /  _(__  )/ /_/ /  / _ `/ __/ / _ \\/ _ \\  ",
      "  /___/  /____/ \\____//_/\\_,_/\\__/_/\\___/_//_/ ",
      "                                                ",
  };
}
*/
std::vector<std::string> logoLinesA() {
  return {
      "                                                ",
      "                             ______  _      -   ",
      "                            __ _/ /_(_▒▒▒▒ _░░___",
      "                            _ `/▒__/ ▒▒▒░    ░░▒\\",
      "                          \\_,_/\\__/_/   ▒▒░  ░",
      "                                                ",
  };
}

std::vector<std::string> logoLinesB() {
  return {
      "  ________            ______",
      "  ____  _/_______________▒ /",
      "   __  / __  ___/▓▓__ \\_  /",
      "  __/ /  _(__  )/ /_/ /  /",
      "  /___/  /____/ \\____//_/",
      "                                                ",
  };
}

void drawLogoArea(const Layout& layout) {
  const auto lines = logoLinesA();
  const int maxLogoCols = std::max(0, layout.infoLeft - 1);
  const int rowCount = std::min(layout.topRows, static_cast<int>(lines.size()));
  for (int i = 0; i < rowCount; ++i) {
    mvaddnstr(i, 0, lines[i].c_str(), maxLogoCols);
  }

  const auto linesB = logoLinesB();
  const int maxLogoColsB = std::max(0, layout.infoLeft - 1);
  const int rowCountB =
      std::min(layout.topRows, static_cast<int>(linesB.size()));
  attron(COLOR_PAIR(CP_P1));
  for (int i = 0; i < rowCountB; ++i) {
    mvaddnstr(i, 0, linesB[i].c_str(), maxLogoColsB);
  }
  attroff(COLOR_PAIR(CP_P1));
}

void drawInfoBox(const Layout& layout, const Settings& settings,
                 const std::string& roomCode, MenuPage page,
                 const std::optional<scenes::ReplaySummary>& hoveredReplay,
                 const ReplayMetadata* hoveredMeta) {
  const int infoHeight = layout.topRows;
  drawFrameBox(0, layout.infoLeft, infoHeight, layout.infoWidth, "Info");

  const int x = layout.infoLeft + 2;
  const int innerWidth = std::max(0, layout.infoWidth - 4);
  for (int r = 1; r < infoHeight - 1; ++r) {
    mvaddnstr(r, x, std::string(innerWidth, " "[0]).c_str(), innerWidth);
  }

  int row = 1;
  if (page == MenuPage::ReplayBrowser && hoveredReplay && hoveredMeta) {
    const std::string p1 = hoveredMeta->player1Name.empty()
                               ? "Player 1"
                               : hoveredMeta->player1Name;
    const std::string p2 = hoveredMeta->player2Name.empty()
                               ? "Player 2"
                               : hoveredMeta->player2Name;

    const std::string p1Label =
        std::string("P1") + ((hoveredMeta->winner == 0) ? ": (winner)" : ":");
    const std::string p2Label =
        std::string("P2") + ((hoveredMeta->winner == 1) ? ": (winner)" : ":");

    mvaddnstr(row++, x, truncateToWidth(p1Label, innerWidth).c_str(),
              innerWidth);
    attron(COLOR_PAIR(CP_P1));
    mvaddnstr(row++, x, truncateToWidth(p1, innerWidth).c_str(), innerWidth);
    attroff(COLOR_PAIR(CP_P1));

    if (row < infoHeight - 1) {
      mvaddnstr(row++, x, truncateToWidth(p2Label, innerWidth).c_str(),
                innerWidth);
    }
    if (row < infoHeight - 1) {
      attron(COLOR_PAIR(CP_P2));
      mvaddnstr(row++, x, truncateToWidth(p2, innerWidth).c_str(), innerWidth);
      attroff(COLOR_PAIR(CP_P2));
    }
  } else {
    mvaddnstr(row++, x, "Tag:", innerWidth);
    attron(COLOR_PAIR(CP_P1));
    mvaddnstr(
        row++, x,
        padRight(truncateToWidth(settings.gameTag, innerWidth), innerWidth)
            .c_str(),
        innerWidth);
    attroff(COLOR_PAIR(CP_P1));

    mvaddnstr(row++, x, "Netplay room:", innerWidth);
    mvaddnstr(
        row++, x,
        padRight(truncateToWidth(roomCode, innerWidth), innerWidth).c_str(),
        innerWidth);
  }
}

std::vector<MenuEntry> makeMainEntries() {
  return {{"Human VS Human", MenuAction::HumanVsHuman, true},
          {"CPU Match (Easy)", MenuAction::CpuEasy, true},
          {"CPU Match (Medium)", MenuAction::CpuMedium, true},
          {"CPU Match (Hard)", MenuAction::CpuHard, true},
          {"Multiplayer", MenuAction::Netplay, true},
          {"", MenuAction::Spacer, false},
          {"Replay Browser", MenuAction::OpenReplayBrowser, true},
          {"Edit Room Code", MenuAction::EditRoom, true},
          {"Settings", MenuAction::OpenSettings, true},
          {"", MenuAction::Spacer, false},
          {"", MenuAction::Spacer, false},
          {"Exit", MenuAction::Exit, true}};
}

std::vector<MenuEntry> makeSettingsEntries(const Settings& settings) {
  return {{"Tag: " + settings.gameTag, MenuAction::EditTag, true},
          {"Server IP: " + settings.serverIp, MenuAction::EditServerIp, true},
          {"Server Port: " + std::to_string(settings.serverPort),
           MenuAction::EditServerPort, true},
          {"", MenuAction::Spacer, false},
          {"Return", MenuAction::Back, true}};
}

std::vector<MenuEntry> makeReplayEntries(
    const std::vector<scenes::ReplaySummary>& replays, int page, int pageSize,
    int& totalPages) {
  totalPages =
      std::max(1, (static_cast<int>(replays.size()) + pageSize - 1) / pageSize);
  page = std::clamp(page, 0, totalPages - 1);
  const int start = page * pageSize;
  const int end = std::min(start + pageSize, static_cast<int>(replays.size()));

  std::vector<MenuEntry> out;
  for (int i = start; i < end; ++i) {
    out.push_back({replays[i].fileName, MenuAction::Replay, true});
  }
  out.push_back({"", MenuAction::Spacer, false});
  out.push_back({"Return", MenuAction::Back, true});
  return out;
}

int firstSelectable(const std::vector<MenuEntry>& entries) {
  for (int i = 0; i < static_cast<int>(entries.size()); ++i) {
    if (entries[i].selectable) return i;
  }
  return 0;
}

int moveSelection(const std::vector<MenuEntry>& entries, int current,
                  int delta) {
  if (entries.empty()) return 0;
  int idx = current;
  for (int guard = 0; guard < static_cast<int>(entries.size()); ++guard) {
    idx += delta;
    if (idx < 0 || idx >= static_cast<int>(entries.size())) break;
    if (entries[idx].selectable) return idx;
  }
  return current;
}

void renderMenuList(const Layout& layout, const std::vector<MenuEntry>& entries,
                    int selection, FocusPane focus, int startIndex,
                    int totalLines, const std::string& title) {
  const int top = layout.middleTop;
  const int left = 0;
  const int height = layout.middleRows;
  const int width = layout.leftWidth;
  drawFrameBox(top, left, height, width, title, focus == FocusPane::Hud);

  const int innerTop = top + 1;
  const int innerRows = std::max(0, height - 2);
  const int innerWidth = std::max(0, width - 4);
  const int buttonWidth = std::min(kButtonWidth, innerWidth);

  for (int i = 0; i < innerRows; ++i) {
    mvaddnstr(innerTop + i, left + 1, std::string(width - 2, ' ').c_str(),
              width - 2);
  }

  for (int i = 0; i < innerRows && i < static_cast<int>(entries.size()); ++i) {
    const MenuEntry& e = entries[i];
    const int row = innerTop + i;
    if (!e.selectable) continue;
    std::string text = truncateToWidth(e.label, buttonWidth);
    text = padRight(text, buttonWidth);
    if (i == selection) attron(A_REVERSE);
    mvaddnstr(row, left + 2, text.c_str(), buttonWidth);
    if (i == selection) attroff(A_REVERSE);
  }

  drawScrollbar(innerTop, left + width - 2, innerRows, startIndex,
                std::max(1, static_cast<int>(entries.size())), totalLines);
}

void clearRect(int top, int left, int height, int width) {
  if (height <= 0 || width <= 0) return;
  const std::string blank(width, ' ');
  for (int r = 0; r < height; ++r) {
    mvaddnstr(top + r, left, blank.c_str(), width);
  }
}

std::optional<std::string> promptLineInput(const std::string& title,
                                           const std::string& currentValue,
                                           const std::string& hint,
                                           const Layout& layout) {
  const int width = std::min(48, std::max(32, layout.cols - 10));
  const int height = 8;
  const int top = std::max(1, (layout.rows - height) / 2);
  const int left = std::max(1, (layout.cols - width) / 2);

  clearRect(top, left, height, width);
  drawFrameBox(top, left, height, width, title, true, false);

  mvaddnstr(top + 2, left + 2, truncateToWidth(hint, width - 4).c_str(),
            width - 4);
  mvaddnstr(top + 3, left + 2,
            truncateToWidth("Current: " + currentValue, width - 4).c_str(),
            width - 4);
  mvprintw(top + 5, left + 2, "> ");
  refresh();

  timeout(-1);
  echo();
  curs_set(1);
  char buffer[128] = {0};
  mvgetnstr(top + 5, left + 4, buffer, static_cast<int>(sizeof(buffer) - 1));
  curs_set(0);
  noecho();
  timeout(0);
  return std::string(buffer);
}

void showMessageBox(const Layout& layout, const std::string& title,
                    const std::vector<std::string>& lines) {
  const int width = std::min(50, std::max(28, layout.cols - 8));
  const int height =
      std::min(layout.rows - 2, static_cast<int>(lines.size()) + 4);
  const int top = std::max(1, (layout.rows - height) / 2);
  const int left = std::max(1, (layout.cols - width) / 2);

  clearRect(top, left, height, width);
  drawFrameBox(top, left, height, width, title, true, false);

  for (int i = 0; i < height - 4 && i < static_cast<int>(lines.size()); ++i) {
    mvaddnstr(top + 2 + i, left + 2,
              truncateToWidth(lines[i], width - 4).c_str(), width - 4);
  }
  mvaddnstr(top + height - 2, left + 2, "Press any key.", width - 4);
  refresh();
  timeout(-1);
  getch();
  timeout(0);
}

struct PreviewState {
  std::vector<scenes::ReplaySummary> replays;
  std::unique_ptr<ReplaySession> session;
  BoardRenderer board;
  std::string currentPath;
  std::mt19937 rng{std::random_device{}()};

  std::optional<ReplayMetadata> hoveredMeta;
  std::string hoveredMetaPath;

  void scan() { replays = scenes::scanReplays(); }

  const ReplayMetadata* metadataFor(const std::string& path) {
    if (path.empty()) return nullptr;
    if (path != hoveredMetaPath) {
      hoveredMeta = ReplayIO::loadReplayMetadata(path);
      hoveredMetaPath = path;
    }
    return hoveredMeta ? &*hoveredMeta : nullptr;
  }

  /*Also sets speed for every new replay*/
  bool loadPath(const std::string& path) {
    if (path.empty() || path == currentPath) return session != nullptr;
    std::optional<ReplayData> data = ReplayIO::loadReplay(path);
    if (!data) return false;
    session = std::make_unique<ReplaySession>(*data);
    session->setAutoPlay(true);
    session->setPlaybackSpeed(4.0f);
    currentPath = path;
    return true;
  }

  bool loadRandom() {
    if (replays.empty()) return false;
    std::uniform_int_distribution<int> dist(
        0, static_cast<int>(replays.size()) - 1);
    for (int attempt = 0; attempt < 8; ++attempt) {
      const std::string& path = replays[dist(rng)].path;
      if (path != currentPath || replays.size() == 1) {
        return loadPath(path);
      }
    }
    return loadPath(replays.front().path);
  }

  void tickForMain() {
    if (!session) {
      loadRandom();
    }
    if (!session) return;
    session->update(ERR);
    if (session->isFinished()) {
      loadRandom();
    }
  }

  void tickForPinned(const std::string& path) {
    if (path.empty()) return;
    if (path != currentPath) {
      loadPath(path);
    }
    if (!session) return;
    session->update(ERR);
    if (session->isFinished()) {
      loadPath(path);
    }
  }
};

void redrawPreviewBorder(const Layout& layout, FocusPane focus) {
  drawFrameBox(layout.middleTop, layout.rightLeft, layout.previewRows,
               layout.previewCols, "", focus == FocusPane::Replay);
}

void drawPreview(const Layout& layout, PreviewState& preview, FocusPane focus,
                 int inputKey) {
  if (!preview.session) {
    drawFrameBox(layout.middleTop, layout.rightLeft, layout.previewRows,
                 layout.previewCols, "", focus == FocusPane::Replay);
    mvaddnstr(layout.middleTop + layout.previewRows / 2, layout.rightLeft + 2,
              "No replays found.", layout.previewCols - 4);
    return;
  }

  preview.board.moveTo(layout.middleTop, layout.rightLeft);
  preview.board.resize(layout.previewRows, layout.previewCols);
  preview.board.render(inputKey, focus == FocusPane::Replay, *preview.session);
  redrawPreviewBorder(layout, focus);
}

void drawLauncherKeyTips(const Layout& layout, FocusPane focus, MenuPage page) {
  const int uiBottom = layout.middleTop + layout.middleRows - 1;

  if (focus == FocusPane::Hud) {
    if (page == MenuPage::ReplayBrowser) {
      scenes::drawBottomKeyTip(
          uiBottom, layout.cols,
          {"[Tab] Replay", "[W/S] Select", "[C] Confirm", "[A/D] Page"});
    } else {
      scenes::drawBottomKeyTip(uiBottom, layout.cols,
                               {"[Tab] Replay", "[W/S] Select", "[C] Confirm"});
    }
  } else {
    scenes::drawBottomKeyTip(uiBottom, layout.cols,
                             {"[Tab] HUD", "[Arrows] Scroll"});
  }
}

LaunchRequest runLauncher(Settings& settings, std::string& roomCode,
                          BlizzardEffect* effect = nullptr) {
  MenuPage page = MenuPage::Main;
  FocusPane focus = FocusPane::Hud;
  int selection = 0;
  int replayPage = 0;
  PreviewState preview;
  preview.scan();
  preview.loadRandom();

  ensureUiColorsInitialized();
  if (effect) effect->startBlockingTransition();

  erase();

  while (true) {
    const Layout layout = computeLayout();
    // erase(); /*Purging once per frame is unnecessary*/

    std::vector<MenuEntry> entries;
    std::string boxTitle = "Options";
    std::optional<scenes::ReplaySummary> hoveredReplay;
    int replayPages = 1;
    int totalLinesForScrollbar = 0;
    int startLineForScrollbar = 0;
    const ReplayMetadata* hoveredMetaPtr = nullptr;

    if (page == MenuPage::Main) {
      entries = makeMainEntries();
      selection =
          std::clamp(selection, 0, static_cast<int>(entries.size()) - 1);
      if (!entries[selection].selectable) selection = firstSelectable(entries);
      preview.tickForMain();
      totalLinesForScrollbar = static_cast<int>(entries.size());
      startLineForScrollbar = 0;
    } else if (page == MenuPage::Settings) {
      entries = makeSettingsEntries(settings);
      selection =
          std::clamp(selection, 0, static_cast<int>(entries.size()) - 1);
      if (!entries[selection].selectable) selection = firstSelectable(entries);
      boxTitle = "Settings";
      preview.tickForMain();
      totalLinesForScrollbar = static_cast<int>(entries.size());
      startLineForScrollbar = 0;
    } else if (page == MenuPage::ReplayBrowser) {
      const int pageSize = std::max(1, layout.middleRows - 4);
      entries =
          makeReplayEntries(preview.replays, replayPage, pageSize, replayPages);
      replayPage = std::clamp(replayPage, 0, replayPages - 1);
      const int start = replayPage * pageSize;
      if (selection >= static_cast<int>(entries.size())) selection = 0;
      if (!entries[selection].selectable) selection = firstSelectable(entries);
      boxTitle = "Replay Browser";
      const int hoveredIndex = start + selection;

      if (selection < pageSize &&
          hoveredIndex < static_cast<int>(preview.replays.size())) {
        hoveredReplay = preview.replays[hoveredIndex];
        preview.tickForPinned(hoveredReplay->path);
        hoveredMetaPtr = preview.metadataFor(hoveredReplay->path);
      } else {
        preview.tickForMain();
      }
      totalLinesForScrollbar = static_cast<int>(preview.replays.size()) + 2;
      startLineForScrollbar = start;
    }

    drawLogoArea(layout);
    drawInfoBox(layout, settings, roomCode, page, hoveredReplay,
                hoveredMetaPtr);
    renderMenuList(layout, entries, selection, focus, startLineForScrollbar,
                   totalLinesForScrollbar, boxTitle);
    const int previewInput = (focus == FocusPane::Replay) ? getch() : ERR;
    drawPreview(layout, preview, focus, previewInput);
    drawLauncherKeyTips(layout, focus, page);

    if (effect) {
      effect->updateAndDraw();
    }

    refresh();

    int ch = ERR;
    if (focus != FocusPane::Replay) {
      ch = getch();
    } else {
      ch = previewInput;
    }

    if (ch == ERR) {
      napms(50);
      continue;
    }

    if (ch == KEY_RESIZE) {
      erase();
      continue;
    }

    if (ch == '\t') {
      focus = (focus == FocusPane::Hud) ? FocusPane::Replay : FocusPane::Hud;
      continue;
    }

    if (focus == FocusPane::Replay) {
      napms(50);
      continue;
    }

    if (ch == 'w' || ch == 'W') {
      selection = moveSelection(entries, selection, -1);
      continue;
    }
    if (ch == 's' || ch == 'S') {
      selection = moveSelection(entries, selection, +1);
      continue;
    }
    if ((ch == 'a' || ch == 'A') && page == MenuPage::ReplayBrowser) {
      if (replayPage > 0) {
        --replayPage;
        selection = 0;
      }
      continue;
    }
    if ((ch == 'd' || ch == 'D') && page == MenuPage::ReplayBrowser) {
      const int pageSize = std::max(1, layout.middleRows - 4);
      int totalPages = std::max(
          1,
          (static_cast<int>(preview.replays.size()) + pageSize - 1) / pageSize);
      if (replayPage + 1 < totalPages) {
        ++replayPage;
        selection = 0;
      }
      continue;
    }

    if (!(ch == 'c' || ch == 'C' || ch == '\n' || ch == ' ')) {
      continue;
    }

    const MenuAction action =
        entries.empty() ? MenuAction::None : entries[selection].action;
    switch (action) {
      case MenuAction::HumanVsHuman:
      case MenuAction::CpuEasy:
      case MenuAction::CpuMedium:
      case MenuAction::CpuHard:
      case MenuAction::Netplay:
      case MenuAction::Exit:
        return {action, ""};

      case MenuAction::OpenReplayBrowser:
        page = MenuPage::ReplayBrowser;
        replayPage = 0;
        selection = 0;
        break;

      case MenuAction::OpenSettings:
        page = MenuPage::Settings;
        selection = 0;
        break;

      case MenuAction::Replay: {
        const int pageSize = std::max(1, layout.middleRows - 4);
        const int idx = replayPage * pageSize + selection;
        if (idx >= 0 && idx < static_cast<int>(preview.replays.size())) {
          return {MenuAction::Replay, preview.replays[idx].path};
        }
        break;
      }

      case MenuAction::Back:
        page = MenuPage::Main;
        selection = 0;
        break;

      case MenuAction::EditRoom: {
        const std::optional<std::string> value = promptLineInput(
            "Edit Room Code", roomCode, "Allowed: A-Z a-z 0-9 _ - .", layout);
        if (value && !value->empty()) {
          if (isValidRoomCode(*value)) {
            roomCode = *value;
          } else {
            showMessageBox(layout, "Invalid Room Code",
                           {"Use 1-32 characters from A-Z a-z 0-9 _ - ."});
          }
        }
        break;
      }

      case MenuAction::EditTag: {
        const std::optional<std::string> value = promptLineInput(
            "Edit Tag", settings.gameTag, "Allowed: A-Z a-z 0-9 _ - .", layout);
        if (value && !value->empty()) {
          if (isValidTag(*value)) {
            settings.gameTag = *value;
            SettingsIO::saveSettings(settings);
          } else {
            showMessageBox(layout, "Invalid Tag",
                           {"Use 1-24 characters from A-Z a-z 0-9 _ - ."});
          }
        }
        break;
      }

      case MenuAction::EditServerIp: {
        const std::optional<std::string> value =
            promptLineInput("Edit Server IP", settings.serverIp,
                            "Use localhost or IPv4.", layout);
        if (value && !value->empty()) {
          if (isValidServerIp(*value)) {
            settings.serverIp = *value;
            SettingsIO::saveSettings(settings);
          } else {
            showMessageBox(layout, "Invalid Server IP",
                           {"Use localhost or a valid IPv4 address."});
          }
        }
        break;
      }

      case MenuAction::EditServerPort: {
        const std::optional<std::string> value = promptLineInput(
            "Edit Server Port", std::to_string(settings.serverPort),
            "Enter an integer from 1 to 65535.", layout);
        if (value && !value->empty()) {
          int port = 0;
          if (parseIntStrict(*value, port) && isValidServerPort(port)) {
            settings.serverPort = port;
            SettingsIO::saveSettings(settings);
          } else {
            showMessageBox(layout, "Invalid Server Port",
                           {"Enter an integer from 1 to 65535."});
          }
        }
        break;
      }

      case MenuAction::None:
      case MenuAction::Spacer:
        break;
    }
  }
}

}  // namespace

int main() {
  Settings settings = SettingsIO::loadSettings().value_or(Settings{});
  if (!SettingsIO::loadSettings()) {
    SettingsIO::saveSettings(settings);
  }

  scenes::NcursesGuard curses;

  std::string roomCode = "room-123";

  BlizzardEffect* effect = new BlizzardEffect;
  if (!effect) return -1;

  while (true) {
    LaunchRequest req = runLauncher(settings, roomCode, effect);

    if (req.action !=
        MenuAction::Netplay) { /*Netplay has its own blocking pop-ups. It draws
                                  it's own transition. */
      if (effect) effect->startBlockingTransition();
    }
    switch (req.action) {
      case MenuAction::HumanVsHuman:
        scenes::runHumanVsHuman(settings, effect);
        break;
      case MenuAction::CpuEasy:
        scenes::runHumanVsCpu(settings, AiDifficulty::Easy, effect);
        break;
      case MenuAction::CpuMedium:
        scenes::runHumanVsCpu(settings, AiDifficulty::Medium, effect);
        break;
      case MenuAction::CpuHard:
        scenes::runHumanVsCpu(settings, AiDifficulty::Hard, effect);
        break;
      case MenuAction::Netplay:
        scenes::runNetplay(settings, roomCode, effect);
        break;
      case MenuAction::Replay:
        if (!req.replayPath.empty()) scenes::runReplay(req.replayPath, effect);
        break;
      case MenuAction::Exit:
        if (effect) {
          effect->startBlockingTransition();
          for (int i = 0; i < 50; ++i) {
            effect->updateAndDraw();
            refresh();
            napms(kFrameMs);
          }
        }
        return 0;
      default:
        break;
    }

    // if (effect) effect->startBlockingTransition();
  }

  delete effect;
  endwin();
  std::_Exit(130);
  erase();
  return 0;
}
