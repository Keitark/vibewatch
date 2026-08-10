#pragma once

namespace morse {

enum class CommandType {
    None,
    Action,
    Agent,
    MicrophoneToggle,
};

struct CommandMapping {
    CommandType type = CommandType::None;
    const char* eventKey = nullptr;
    const char* label = "UNMAPPED";
};

CommandMapping mapCommand(char character);

}  // namespace morse
