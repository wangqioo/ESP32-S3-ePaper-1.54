#include "assistant_model.h"

#include <sstream>

namespace {

bool IsSpace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

size_t Utf8CharBytes(unsigned char lead) {
    if ((lead & 0x80) == 0) {
        return 1;
    }
    if ((lead & 0xE0) == 0xC0) {
        return 2;
    }
    if ((lead & 0xF0) == 0xE0) {
        return 3;
    }
    if ((lead & 0xF8) == 0xF0) {
        return 4;
    }
    return 1;
}

size_t VisibleChars(const std::string &text) {
    size_t count = 0;
    for (size_t i = 0; i < text.size();) {
        i += Utf8CharBytes(static_cast<unsigned char>(text[i]));
        ++count;
    }
    return count;
}

std::string TruncateVisible(const std::string &text, size_t max_chars) {
    std::string out;
    size_t chars = 0;
    for (size_t i = 0; i < text.size() && chars < max_chars;) {
        size_t bytes = Utf8CharBytes(static_cast<unsigned char>(text[i]));
        out.append(text, i, bytes);
        i += bytes;
        ++chars;
    }
    return out;
}

bool ContainsSpaces(const std::string &text) {
    for (char c : text) {
        if (IsSpace(c)) {
            return true;
        }
    }
    return false;
}

std::vector<std::string> WrapWords(const std::string &text, size_t max_chars, size_t max_lines) {
    std::vector<std::string> words;
    std::istringstream stream(text);
    std::string word;
    while (stream >> word) {
        words.push_back(word);
    }

    std::vector<std::string> lines;
    size_t index = 0;
    while (index < words.size() && lines.size() < max_lines) {
        std::string line = words[index++];
        while (index < words.size() && VisibleChars(line) + 1 + VisibleChars(words[index]) <= max_chars) {
            line += " " + words[index++];
        }
        if (lines.size() + 1 == max_lines && index < words.size() && max_chars > 3) {
            while (index < words.size() && VisibleChars(line) + 1 + VisibleChars(words[index]) <= max_chars - 3) {
                line += " " + words[index++];
            }
            line = TruncateVisible(line, max_chars - 3) + "...";
            lines.push_back(line);
            break;
        }
        lines.push_back(line);
    }
    return lines;
}

std::vector<std::string> WrapChars(const std::string &text, size_t max_chars, size_t max_lines) {
    std::vector<std::string> lines;
    std::string line;
    size_t chars = 0;
    for (size_t i = 0; i < text.size();) {
        size_t bytes = Utf8CharBytes(static_cast<unsigned char>(text[i]));
        if (chars >= max_chars) {
            lines.push_back(line);
            line.clear();
            chars = 0;
            if (lines.size() >= max_lines) {
                break;
            }
        }
        line.append(text, i, bytes);
        i += bytes;
        ++chars;
    }
    if (!line.empty() && lines.size() < max_lines) {
        lines.push_back(line);
    }
    if (lines.size() == max_lines && VisibleChars(text) > max_chars * max_lines && max_chars > 3) {
        lines.back() = TruncateVisible(lines.back(), max_chars - 3) + "...";
    }
    return lines;
}

}  // namespace

std::string AssistantStatusLabel(AssistantState state) {
    switch (state) {
        case AssistantState::Idle:
            return "Ready";
        case AssistantState::Recording:
            return "Recording";
        case AssistantState::Uploading:
            return "Uploading";
        case AssistantState::Thinking:
            return "Thinking";
        case AssistantState::Answer:
            return "Answer";
        case AssistantState::Error:
            return "Error";
        case AssistantState::ShuttingDown:
            return "Power off";
    }
    return "Ready";
}

std::string AssistantErrorLabel(AssistantError error) {
    switch (error) {
        case AssistantError::None:
            return "";
        case AssistantError::WifiUnavailable:
            return "Wi-Fi unavailable";
        case AssistantError::ProxyUnavailable:
            return "Proxy unavailable";
        case AssistantError::UploadFailed:
            return "Upload failed";
        case AssistantError::AiFailed:
            return "AI failed";
        case AssistantError::RecordFailed:
            return "Record failed";
        case AssistantError::StorageFailed:
            return "Storage failed";
        case AssistantError::Busy:
            return "Busy";
    }
    return "Error";
}

std::vector<std::string> WrapAssistantText(const std::string &text, size_t max_chars, size_t max_lines) {
    if (max_chars == 0 || max_lines == 0) {
        return {};
    }
    if (ContainsSpaces(text)) {
        return WrapWords(text, max_chars, max_lines);
    }
    return WrapChars(text, max_chars, max_lines);
}

void ApplyAssistantEvent(AssistantSession &session, AssistantEvent event) {
    switch (event) {
        case AssistantEvent::BootDown:
            if (session.state == AssistantState::Idle || session.state == AssistantState::Answer || session.state == AssistantState::Error) {
                session = AssistantSession{};
                session.state = AssistantState::Recording;
            } else {
                session.error = AssistantError::Busy;
            }
            break;
        case AssistantEvent::BootUp:
            if (session.state == AssistantState::Recording) {
                session.state = AssistantState::Uploading;
            }
            break;
        case AssistantEvent::UploadStarted:
            session.state = AssistantState::Uploading;
            break;
        case AssistantEvent::UploadSucceeded:
            session.state = AssistantState::Answer;
            session.error = AssistantError::None;
            break;
        case AssistantEvent::UploadFailed:
            ApplyAssistantError(session, AssistantError::UploadFailed);
            break;
        case AssistantEvent::PowerShort:
            if (session.state == AssistantState::Answer || session.state == AssistantState::Error) {
                session = AssistantSession{};
            }
            break;
        case AssistantEvent::PowerLong:
            if (session.state == AssistantState::Recording || session.state == AssistantState::Uploading || session.state == AssistantState::Thinking) {
                session.error = AssistantError::Busy;
            } else {
                session.state = AssistantState::ShuttingDown;
            }
            break;
    }
}

void ApplyAssistantError(AssistantSession &session, AssistantError error) {
    session.state = AssistantState::Error;
    session.error = error;
}

void ApplyAssistantAnswer(AssistantSession &session, const std::string &answer, const std::string &request_id) {
    session.state = AssistantState::Answer;
    session.error = AssistantError::None;
    session.answer = answer;
    session.request_id = request_id;
}
