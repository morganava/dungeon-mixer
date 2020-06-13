#include "ui.hpp"

namespace tabletopify
{
    // Edit Screen Methods

    void ui::on_do_audio_search(const utf8_string& query)
    {
        worker_background.enqueue([=]() -> void
        {
            auto audioHandles = g_audio_directory.get_audio_handles_by_query(query);
            std::vector<audio_query_result_widget> queryWidgets;
            for(auto handle : audioHandles) {
                const auto& entry = g_audio_directory.get_entry(handle);
                queryWidgets.push_back({handle, {entry.title}});
            }

            worker_ui.enqueue([queryWidgets=std::move(queryWidgets)]() mutable -> void
            {
                g_ui.root.edit.search.queryResults = std::move(queryWidgets);
            });
        });
    }

    void ui::draw_edit(float dt)
    {
        mxVBox(&root.edit.vflex, self.tags = {"content"})
        {
            mxLabel(&root.edit.heading, self.tags= {"header", "darkText"});
            mxButton(g_ui.root.edit.tracks.title, self.tags = {"header", "title", "lightText"});
            mxHBox(&root.edit.hflex)
            {
                mxSpacer();
                mxTextBox(&root.edit.searchBox, self.tags = {"search"};
                self.on_submit = [](const utf8_string& query) -> void
                {
                    g_ui.on_do_audio_search(query);
                });
            }
            mxHBox(&root.edit.hflex)
            {
                // track edit screen
                mxVScroll(&root.edit.tracks.scroll)
                {
                    for(auto& editWidget : root.edit.tracks.widgets)
                    {
                        mxHBox(&root.edit.tracks.title_hflex, self.tags={"atmosphereTitle"})
                        {
                            mxCheckbox(&editWidget.visible, self.tags={"entryTitle"});
                            mxLabel(editWidget.audio_title, self.tags={"entryTitle"});
                        }
                    }
                }
                // track search results
                mxVScroll(&root.edit.search.scroll)
                {
                    for(auto& queryResult : root.edit.search.queryResults)
                    {
                        mxButton(queryResult.title, self.tags={"audioResult"};
                        self.on_click = [&]() -> void
                        {
                            MX_LOG_INFO("CLICKED : {}, {}", queryResult.title, queryResult.handle);
                        });
                        mxSpacer();
                    }
                }
            }
        }
    }
}