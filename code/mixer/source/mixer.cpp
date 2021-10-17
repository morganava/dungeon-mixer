#include "globals.hpp"
#include "audio.hpp"
#include "work_queue.hpp"
#include "logger.hpp"
#include "string_manager.hpp"
#include "graphics.hpp"
#include "input.hpp"
#include "nanovg.hpp"
#include "service.hpp"
#include "imgui.hpp"
#include "ui.hpp"

using namespace smol;

void test()
{
    utf8_stringbuffer sb;

    const char* funny_words[] =
    {
        "",
        "Hello World",
        "cote",
        "coté",
        "côte",
        "côté",
        "grüßen",
        "ὈΔΥΣΣΕΎΣ",
        "[一-龠々〆ヵヶ]",
        "大変、　よ　ろし　くお願い申し上げます。",
    };

    for(const auto& str : funny_words)
    {
        utf8_string utf8(str);
        utf8_string norm = utf8.normalized();

        MX_LOG_INFO("\n str  '{}', {}\n utf8 '{}', {}\n norm '{}', {}", str, std::strlen(str),utf8, utf8.size(), norm, norm.size());
        MX_LOG_INFO("'{}' == '{}' => {}", utf8, norm, utf8 == norm);

        for(auto cp : norm)
        {
            char buffer[5] = {0};
            utf8proc_encode_char(cp.value(), (utf8proc_uint8_t*)buffer);
            MX_LOG_INFO("0x{:x} : '{}' whitespace ? {}", cp, buffer, cp.is_whitespace());
            sb.push_back(cp);
        }
    }

    utf8_codepoint cp = {0x2007};

    MX_LOG_INFO("FULL STRING : '{}'", sb.str());
    sb.clear();
    sb.push_back(cp);
    MX_LOG_INFO("FULL STRING : '{}'", sb.str());

    utf8_string splitme = "hello this is a string all about how my life got twist　turned 大変、　よ　ろし　くお願い申し上げます。";

    for(auto str : splitme.split())
    {
        MX_LOG_INFO("'{}'", str);
    }
}

namespace dm
{
    void worker_thread(worker_id_t id);
}

int main() try
{
    dm::init_globals();
    dm::worker_thread(dm::worker_id::ui);
    dm::free_globals();

    return 0;
}
catch(const std::exception& ex)
{
    MX_LOG_ERROR(ex.what());
}
