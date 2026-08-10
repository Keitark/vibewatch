#include "command_mapper.h"

namespace morse {

CommandMapping mapCommand(char character) {
    switch (character) {
        case 'F': return {CommandType::Action, "ACT06", "FAST"};
        case 'O': return {CommandType::Action, "ACT07", "OK"};
        case 'N': return {CommandType::Action, "ACT08", "NG"};
        case 'P': return {CommandType::Action, "ACT09", "PLAN"};
        case 'A': return {CommandType::Action, "ACT12", "AI"};
        case 'M': return {CommandType::MicrophoneToggle, nullptr, "HOST MIC"};
        case '1': return {CommandType::Agent, "AG00", "AGENT 1"};
        case '2': return {CommandType::Agent, "AG01", "AGENT 2"};
        case '3': return {CommandType::Agent, "AG02", "AGENT 3"};
        case '4': return {CommandType::Agent, "AG03", "AGENT 4"};
        case '5': return {CommandType::Agent, "AG04", "AGENT 5"};
        case '6': return {CommandType::Agent, "AG05", "AGENT 6"};
        default: return {};
    }
}

}  // namespace morse
