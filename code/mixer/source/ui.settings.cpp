#include "ui.hpp"
#include "logger.hpp"
#include "string_manager.hpp"
#include "config.hpp"

namespace dm
{
    namespace details
    {
        void init_log_filename(const auto& value, auto& widgetData)
        {
            widgetData = imgui::text_box_data(value, "");
        }

        void draw_log_filename(auto& widgetData)
        {
            mxTextBox(&widgetData, self.tags = {"settingsEntry"};
                self.on_submit = [&](const utf8_string& val) -> void
                {
                    g_config->set_log_filename(utf8_string{val});
                    widgetData = imgui::text_box_data(g_config->get_log_filename(), "");
                });
        }

        void init_log_level(const auto& value, auto& widgetData)
        {
            widgetData = imgui::text_box_data(fmt::format("{}", value), "");
        }

        void draw_log_level(auto& widgetData)
        {
            mxTextBox(&widgetData, self.tags = {"settingsEntry"});
        }

        void init_skin(const auto& value, auto& widgetData)
        {
            MX_LOG_INFO("log_skin: {}", value);
            widgetData = imgui::text_box_data(value, "");
        }

        void draw_skin(auto& widgetData)
        {
            mxTextBox(&widgetData, self.tags = {"settingsEntry"};
                self.on_submit = [&](const utf8_string& val) -> void
                {
                    g_config->set_skin(utf8_string{val});
                    widgetData = imgui::text_box_data(g_config->get_skin(), "");
                });
        }
    }

    void ui::on_set_screen_settings()
    {
        worker_ui.enqueue([]() -> void
        {
            // init all our widgets to values from config service
#define CONFIG_ENTRY(NAME, DEFAULT_VALUE)\
            {\
                const auto& val = g_config->get_##NAME();\
                auto& widgetData = g_ui->root.settings.widget_data.NAME;\
                details::init_##NAME(val, widgetData);\
            }
#include "config.inl"
#undef CONFIG_ENTRY

            g_ui->set_screen(screen::settings);
        });
    }

    void ui::draw_settings(float dt)
    {
        mxVBox(self.tags = {"content"})
        {
            mxSpacer();
            mxLabel(g_strings[mixer_string::settings_screen], self.tags= {"header", "title", "lightText"});
            mxSpacer();
            mxVScroll(&root.settings.scroll)
            {
#define CONFIG_ENTRY(NAME, DEFAULT_VALUE)\
                {\
                    mxHBox(self.tags = {"settingsRow"})\
                    {\
                        mxLabel({#NAME});\
                        auto& widgetData = root.settings.widget_data.NAME;\
                        details::draw_##NAME(widgetData);\
                    }\
                }
#include "config.inl"
#undef CONFIG_ENTRY
            }
        }
    }
}