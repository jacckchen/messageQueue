//
// Created by 13717 on 2025/10/4.
//

#ifndef LIST_PERSISTENCE_H
#define LIST_PERSISTENCE_H
#include "Server.h"

class Persistence
{
    std::shared_ptr<Queue<json>> messageQueue;
public:
    explicit Persistence(const std::shared_ptr<Queue<json>> &messageQueue):messageQueue(messageQueue)
    {

    }
    void save() const
    {
        std::cout << "Saving messages to list.txt... please wait" << std::endl;
        while (!messageQueue->Empty())
        {
            json message = std::move(messageQueue->deQueue_front());
            std::ofstream ofs("./list.txt", std::ios::app);
            ofs << message.dump() << "\n";
        }
    }
    ~Persistence()=default;
};


#endif //LIST_PERSISTENCE_H