#pragma once

enum class Side {
    Player1,
    Player2
};

enum class TileState {
    Intact,
    Broken
};

enum class TurnPhase {
    NewTurn,
    Move,
    Break,
    Finished
};

enum class SessionStatus {
    Running,
    Finished
};

enum class CpuDifficulty {
    Easy,
    Medium,
    Hard
};
