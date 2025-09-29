import React, { createContext, useContext, useEffect, useState, useCallback } from 'react';

const WebSocketContext = createContext();

export const useWebSocket = () => {
  const context = useContext(WebSocketContext);
  if (!context) {
    throw new Error('useWebSocket must be used within a WebSocketProvider');
  }
  return context;
};

export const WebSocketProvider = ({ children, onConnectionChange, onServerStatsUpdate }) => {
  const [socket, setSocket] = useState(null);
  const [connectionStatus, setConnectionStatus] = useState('disconnected');
  const [monsters, setMonsters] = useState(new Map());
  const [players, setPlayers] = useState(new Map());
  const [messages, setMessages] = useState([]);

  const connect = useCallback(() => {
    if (socket && socket.readyState === WebSocket.OPEN) {
      return;
    }

    const ws = new WebSocket('ws://localhost:8082');
    
    ws.onopen = () => {
      console.log('WebSocket 연결됨');
      setConnectionStatus('connected');
      setSocket(ws);
      onConnectionChange?.('connected');
    };

    ws.onmessage = (event) => {
      try {
        const data = JSON.parse(event.data);
        console.log('WebSocket 메시지 수신:', data);
        
        switch (data.type) {
          case 'monster_update':
            setMonsters(prev => {
              const newMonsters = new Map(prev);
              if (data.monsters && Array.isArray(data.monsters)) {
                data.monsters.forEach(monster => {
                  if (monster && monster.id !== undefined) {
                    newMonsters.set(monster.id, {
                      ...monster,
                      lastUpdate: Date.now()
                    });
                  }
                });
              }
              return newMonsters;
            });
            break;
          case 'player_update':
            setPlayers(prev => {
              const newPlayers = new Map(prev);
              if (data.players && Array.isArray(data.players)) {
                data.players.forEach(player => {
                  if (player && player.id !== undefined) {
                    newPlayers.set(player.id, {
                      ...player,
                      lastUpdate: Date.now()
                    });
                  }
                });
              }
              return newPlayers;
            });
            break;
            
          case 'monster_spawn':
            setMonsters(prev => {
              const newMonsters = new Map(prev);
              newMonsters.set(data.data.id, {
                ...data.data,
                lastUpdate: Date.now()
              });
              return newMonsters;
            });
            setMessages(prev => [...prev, {
              type: 'info',
              message: `몬스터 ${data.data.name}이(가) 스폰되었습니다.`,
              timestamp: data.timestamp
            }]);
            break;
            
          case 'monster_death':
            setMonsters(prev => {
              const newMonsters = new Map(prev);
              newMonsters.delete(data.data.id);
              return newMonsters;
            });
            setMessages(prev => [...prev, {
              type: 'warning',
              message: `몬스터 ${data.data.name}이(가) 사망했습니다.`,
              timestamp: data.timestamp
            }]);
            break;
            
          case 'server_stats':
            onServerStatsUpdate?.(data.data);
            break;
            
          case 'system_message':
            setMessages(prev => [...prev, {
              type: data.data.level || 'info',
              message: data.data.message,
              timestamp: data.timestamp
            }]);
            break;
            
          case 'bt_execution':
            setMessages(prev => [...prev, {
              type: 'info',
              message: `몬스터 ${data.data.monster_name}이(가) ${data.data.action}을(를) 실행했습니다.`,
              timestamp: data.timestamp
            }]);
            break;
            
          default:
            console.log('알 수 없는 메시지 타입:', data.type);
        }
      } catch (error) {
        console.error('WebSocket 메시지 파싱 오류:', error);
      }
    };

    ws.onclose = () => {
      console.log('WebSocket 연결 종료');
      setConnectionStatus('disconnected');
      setSocket(null);
      onConnectionChange?.('disconnected');
      
      // 3초 후 재연결 시도
      setTimeout(() => {
        if (connectionStatus !== 'connected') {
          connect();
        }
      }, 3000);
    };

    ws.onerror = (error) => {
      console.error('WebSocket 오류:', error);
      setConnectionStatus('error');
      onConnectionChange?.('error');
    };
  }, [socket, connectionStatus, onConnectionChange, onServerStatsUpdate]);

  const disconnect = useCallback(() => {
    if (socket) {
      socket.close();
      setSocket(null);
    }
  }, [socket]);

  const sendMessage = useCallback((message) => {
    if (socket && socket.readyState === WebSocket.OPEN) {
      socket.send(JSON.stringify(message));
    }
  }, [socket]);

  useEffect(() => {
    connect();
    
    return () => {
      disconnect();
    };
  }, []);

  const value = {
    socket,
    connectionStatus,
    monsters: Array.from(monsters.values()),
    players: Array.from(players.values()),
    messages,
    connect,
    disconnect,
    sendMessage
  };

  return (
    <WebSocketContext.Provider value={value}>
      {children}
    </WebSocketContext.Provider>
  );
};
