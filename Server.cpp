
#include "Server.h"

#include "Persistence.h"

Server::Server(const Config &config) {
    this->mode=config.getMode();
    this->ip=config.getServerIp();
    this->port=config.getServerPort();
    this->threadPoolSize=config.getThreadPoolSize();
    this->receivePort=config.getReceivePort();
}

Server::Server(const std::string &configFile):Server(Config(configFile)) {
}
void Server::init() {
    threadPool=std::make_unique<ThreadPool>(threadPoolSize);
    messageQueue=std::make_shared<Queue<json>>();
    receiver=std::make_unique<Receiver>(ip,receivePort,messageQueue,threadPool);
    this->consumerManager=std::make_shared<ConsumerManager>(ip,port,threadPool);
    this->dispatcher=std::make_unique<ClusterDispatcher>(consumerManager,threadPool,messageQueue);
}
void Server::start() const
{
    this->threadPool->enqueue([this](){receiver->receive();}) ;
    this->threadPool->enqueue([this](){consumerManager->start();});
    dispatcher->start();
}
void Server::stop() const
{
    receiver->stop();
    dispatcher->stop();
    consumerManager->stop();
    const Persistence persistence(messageQueue);
    persistence.save();
}
Server::~Server()
{

};
