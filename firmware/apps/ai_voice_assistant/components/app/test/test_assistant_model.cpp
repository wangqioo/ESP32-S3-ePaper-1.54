#include "../assistant_model.h"

#include <cassert>
#include <string>
#include <vector>

int main() {
    assert(AssistantStatusLabel(AssistantState::Idle) == std::string("Ready"));
    assert(AssistantStatusLabel(AssistantState::Recording) == std::string("Recording"));
    assert(AssistantStatusLabel(AssistantState::Uploading) == std::string("Uploading"));
    assert(AssistantErrorLabel(AssistantError::WifiUnavailable) == std::string("Wi-Fi unavailable"));

    auto wrapped = WrapAssistantText("Shanghai weather and clock are ready", 12, 3);
    assert(wrapped.size() == 3);
    assert(wrapped[0] == "Shanghai");
    assert(wrapped[1] == "weather and");
    assert(wrapped[2] == "clock are...");

    auto chinese = WrapAssistantText("上海天气不错，适合出门散步", 9, 2);
    assert(chinese.size() == 2);
    assert(chinese[0] == "上海天气不错，适合");
    assert(chinese[1] == "出门散步");

    AssistantSession session;
    assert(session.state == AssistantState::Idle);
    ApplyAssistantEvent(session, AssistantEvent::BootDown);
    assert(session.state == AssistantState::Recording);
    ApplyAssistantEvent(session, AssistantEvent::BootUp);
    assert(session.state == AssistantState::Uploading);
    ApplyAssistantAnswer(session, "你好，我在。", "abc123");
    assert(session.state == AssistantState::Answer);
    assert(session.answer == "你好，我在。");
    assert(session.request_id == "abc123");
    ApplyAssistantEvent(session, AssistantEvent::PowerShort);
    assert(session.state == AssistantState::Idle);

    ApplyAssistantEvent(session, AssistantEvent::BootDown);
    ApplyAssistantEvent(session, AssistantEvent::PowerLong);
    assert(session.state == AssistantState::Recording);
    assert(session.error == AssistantError::Busy);
    return 0;
}
