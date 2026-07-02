#pragma once

#include "assistant_types.h"

#include <string>
#include <vector>

std::string AssistantStatusLabel(AssistantState state);
std::string AssistantErrorLabel(AssistantError error);
std::vector<std::string> WrapAssistantText(const std::string &text, size_t max_chars, size_t max_lines);
void ApplyAssistantEvent(AssistantSession &session, AssistantEvent event);
void ApplyAssistantError(AssistantSession &session, AssistantError error);
void ApplyAssistantAnswer(AssistantSession &session, const std::string &answer, const std::string &request_id);
