#include <sstream>
#include <cassert>

#include "mutils/ui/ui_manager.h"
#include "mutils/logger/logger.h"

#include "tinyfiledialogs/tinyfiledialogs.h"

namespace MUtils
{

// Statics
uint64_t UIMessage::CurrentID = 1;

namespace
{
uint64_t InvalidMessageID = 0;
}


UIManager::UIManager()
{

}

UIManager& UIManager::Instance()
{
    static UIManager manager;
    return manager;
}

uint64_t UIManager::AddMessage(uint32_t type, const std::string& message, const fs::path& file, int32_t line, const ColumnRange& column)
{
    auto spMessage = std::make_shared<UIMessage>(type, message, file, line, column);
    if (type & MessageType::Task ||
        !file.empty())
    {
        m_taskMessages[spMessage->m_id] = spMessage;
    }

    if (!file.empty())
    {
        m_fileMessages[spMessage->m_file].push_back(spMessage->m_id);
    }

    if (type & MessageType::System)
    {
        if (type & MessageType::Error)
        {
            tinyfd_messageBox("Error", message.c_str(), "ok", "error", 0);
        }
        else if (type & MessageType::Warning)
        {
            tinyfd_messageBox("Warning", message.c_str(), "ok", "warning", 0);
        }
        else if (type & MessageType::Info)
        {
            tinyfd_messageBox("Info", message.c_str(), "ok", "info", 0);
        }

    }
    return spMessage->m_id;
}

// Remove a message for a given ID
void UIManager::RemoveMessage(uint64_t id)
{
    auto itrFound = m_taskMessages.find(id);
    if (itrFound != m_taskMessages.end())
    {
        if (!itrFound->second->m_file.empty())
        {
            m_fileMessages.erase(itrFound->second->m_file);
        }
    }
}

// Remove all messages associated with a file
void UIManager::ClearFileMessages(fs::path path)
{
    while (!m_fileMessages[path].empty())
    {
        RemoveMessage(m_fileMessages[path][0]);
    }
}

} // MUtils
