#include <iostream>

#include "BeastHttpWebSocketServer.h"
#include "NetworkMessageHandler.h"

namespace bt
{

    NetworkMessageHandler::NetworkMessageHandler()
    {
        std::cout << "NetworkMessageHandler 생성됨" << std::endl;
    }

    NetworkMessageHandler::~NetworkMessageHandler()
    {
        std::cout << "NetworkMessageHandler 소멸됨" << std::endl;
    }

    void NetworkMessageHandler::HandleMessage(std::shared_ptr<GameMessage> message)
    {
        if (!message)
            return;

        switch (message->GetType())
        {
            case MessageType::MONSTER_SPAWN:
                if (auto spawn_msg = std::dynamic_pointer_cast<MonsterSpawnMessage>(message))
                {
                    HandleMonsterSpawn(spawn_msg);
                }
                break;

            case MessageType::MONSTER_MOVE:
                if (auto move_msg = std::dynamic_pointer_cast<MonsterMoveMessage>(message))
                {
                    HandleMonsterMove(move_msg);
                }
                break;

            case MessageType::MONSTER_DEATH:
                if (auto death_msg = std::dynamic_pointer_cast<MonsterDeathMessage>(message))
                {
                    HandleMonsterDeath(death_msg);
                }
                break;

            case MessageType::PLAYER_JOIN:
                if (auto join_msg = std::dynamic_pointer_cast<PlayerJoinMessage>(message))
                {
                    HandlePlayerJoin(join_msg);
                }
                break;

            case MessageType::PLAYER_MOVE:
                if (auto move_msg = std::dynamic_pointer_cast<PlayerMoveMessage>(message))
                {
                    HandlePlayerMove(move_msg);
                }
                break;

            case MessageType::GAME_STATE_UPDATE:
                if (auto state_msg = std::dynamic_pointer_cast<GameStateUpdateMessage>(message))
                {
                    HandleGameStateUpdate(state_msg);
                }
                break;

            case MessageType::NETWORK_BROADCAST:
                if (auto broadcast_msg = std::dynamic_pointer_cast<NetworkBroadcastMessage>(message))
                {
                    HandleNetworkBroadcast(broadcast_msg);
                }
                break;

            case MessageType::SYSTEM_SHUTDOWN:
                if (auto shutdown_msg = std::dynamic_pointer_cast<SystemShutdownMessage>(message))
                {
                    HandleSystemShutdown(shutdown_msg);
                }
                break;

            default:
                // 다른 메시지 타입은 무시
                break;
        }
    }

    void NetworkMessageHandler::SetWebSocketServer(std::shared_ptr<BeastHttpWebSocketServer> server)
    {
        websocket_server_ = server;
    }

    void NetworkMessageHandler::AddClient(const std::string& client_id)
    {
        connected_clients_.insert(client_id);
        std::cout << "클라이언트 연결됨: " << client_id << " (총 " << connected_clients_.size() << "개)" << std::endl;
    }

    void NetworkMessageHandler::RemoveClient(const std::string& client_id)
    {
        connected_clients_.erase(client_id);
        std::cout << "클라이언트 연결 해제됨: " << client_id << " (총 " << connected_clients_.size() << "개)"
                  << std::endl;
    }

    size_t NetworkMessageHandler::GetConnectedClientCount() const
    {
        return connected_clients_.size();
    }

    void NetworkMessageHandler::HandleMonsterSpawn(std::shared_ptr<MonsterSpawnMessage> message)
    {
        nlohmann::json data = {
            {"event",        "monster_spawn"                                                                           },
            {"monster_id",   message->monster_id                                                                       },
            {"monster_name", message->monster_name                                                                     },
            {"monster_type", message->monster_type                                                                     },
            {"position",     {{"x", message->x}, {"y", message->y}, {"z", message->z}, {"rotation", message->rotation}}},
            {"health",       message->health                                                                           },
            {"max_health",   message->max_health                                                                       }
        };

        BroadcastToClients("monster_spawn", data);
    }

    void NetworkMessageHandler::HandleMonsterMove(std::shared_ptr<MonsterMoveMessage> message)
    {
        nlohmann::json data = {
            {"event",      "monster_move"                                                          },
            {"monster_id", message->monster_id                                                     },
            {"from",       {{"x", message->from_x}, {"y", message->from_y}, {"z", message->from_z}}},
            {"to",         {{"x", message->to_x}, {"y", message->to_y}, {"z", message->to_z}}      }
        };

        BroadcastToClients("monster_move", data);
    }

    void NetworkMessageHandler::HandleMonsterDeath(std::shared_ptr<MonsterDeathMessage> message)
    {
        nlohmann::json data = {
            {"event",        "monster_death"                                          },
            {"monster_id",   message->monster_id                                      },
            {"monster_name", message->monster_name                                    },
            {"position",     {{"x", message->x}, {"y", message->y}, {"z", message->z}}}
        };

        BroadcastToClients("monster_death", data);
    }

    void NetworkMessageHandler::HandlePlayerJoin(std::shared_ptr<PlayerJoinMessage> message)
    {
        nlohmann::json data = {
            {"event",       "player_join"                                                                             },
            {"player_id",   message->player_id                                                                        },
            {"player_name", message->player_name                                                                      },
            {"position",    {{"x", message->x}, {"y", message->y}, {"z", message->z}, {"rotation", message->rotation}}}
        };

        BroadcastToClients("player_join", data);
    }

    void NetworkMessageHandler::HandlePlayerMove(std::shared_ptr<PlayerMoveMessage> message)
    {
        nlohmann::json data = {
            {"event",     "player_move"                                                           },
            {"player_id", message->player_id                                                      },
            {"from",      {{"x", message->from_x}, {"y", message->from_y}, {"z", message->from_z}}},
            {"to",        {{"x", message->to_x}, {"y", message->to_y}, {"z", message->to_z}}      }
        };

        BroadcastToClients("player_move", data);
    }

    void NetworkMessageHandler::HandleGameStateUpdate(std::shared_ptr<GameStateUpdateMessage> message)
    {
        nlohmann::json data = {
            {"event",      "game_state_update"},
            {"game_state", message->game_state}
        };

        BroadcastToClients("game_state_update", data);
    }

    void NetworkMessageHandler::HandleNetworkBroadcast(std::shared_ptr<NetworkBroadcastMessage> message)
    {
        BroadcastToClients(message->event_type, message->data);
    }

    void NetworkMessageHandler::HandleSystemShutdown(std::shared_ptr<SystemShutdownMessage> message)
    {
        nlohmann::json data = {
            {"event",  "system_shutdown"},
            {"reason", message->reason  }
        };

        BroadcastToClients("system_shutdown", data);
        std::cout << "시스템 셧다운 메시지 브로드캐스트: " << message->reason << std::endl;
    }

    void NetworkMessageHandler::BroadcastToClients(const std::string& event_type, const nlohmann::json& data)
    {
        if (!websocket_server_)
        {
            std::cerr << "WebSocket 서버가 설정되지 않음" << std::endl;
            return;
        }

        if (connected_clients_.empty())
        {
            return; // 연결된 클라이언트가 없으면 브로드캐스트하지 않음
        }

        nlohmann::json message = {
            {"type",      event_type},
            {"timestamp",
             std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
                 .count()           },
            {"data",      data      }
        };

        std::string message_str = message.dump();

        try
        {
            websocket_server_->broadcast(message_str);
            std::cout << "브로드캐스트 전송: " << event_type << " to " << connected_clients_.size() << " clients"
                      << std::endl;
        }
        catch (const std::exception& e)
        {
            std::cerr << "브로드캐스트 전송 실패: " << e.what() << std::endl;
        }
    }

} // namespace bt
