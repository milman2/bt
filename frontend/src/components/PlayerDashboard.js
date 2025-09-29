import React from 'react';
import styled from 'styled-components';
import { motion } from 'framer-motion';
import { useWebSocket } from '../context/WebSocketContext';

const DashboardContainer = styled(motion.div)`
  background: rgba(255, 255, 255, 0.95);
  border-radius: 15px;
  padding: 25px;
  box-shadow: 0 8px 32px rgba(0, 0, 0, 0.1);
  backdrop-filter: blur(10px);
  border: 1px solid rgba(255, 255, 255, 0.2);
`;

const Title = styled.h2`
  color: #333;
  margin: 0 0 20px 0;
  font-size: 1.5rem;
  display: flex;
  align-items: center;
  gap: 10px;
`;

const PlayerGrid = styled.div`
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(300px, 1fr));
  gap: 15px;
  margin-top: 20px;
`;

const PlayerCard = styled(motion.div)`
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: white;
  padding: 20px;
  border-radius: 12px;
  box-shadow: 0 4px 15px rgba(0, 0, 0, 0.1);
`;

const PlayerName = styled.h3`
  margin: 0 0 10px 0;
  font-size: 1.2rem;
  display: flex;
  align-items: center;
  gap: 8px;
`;

const PlayerInfo = styled.div`
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 10px;
  margin-top: 15px;
`;

const InfoItem = styled.div`
  background: rgba(255, 255, 255, 0.2);
  padding: 8px 12px;
  border-radius: 8px;
  font-size: 0.9rem;
`;

const InfoLabel = styled.span`
  font-weight: 600;
  margin-right: 5px;
`;

const HealthBar = styled.div`
  width: 100%;
  height: 8px;
  background: rgba(255, 255, 255, 0.3);
  border-radius: 4px;
  overflow: hidden;
  margin-top: 10px;
`;

const HealthFill = styled.div`
  height: 100%;
  background: ${props => props.health > 50 ? '#4CAF50' : props.health > 25 ? '#FF9800' : '#F44336'};
  width: ${props => props.health}%;
  transition: width 0.3s ease;
`;

const PositionInfo = styled.div`
  background: rgba(255, 255, 255, 0.2);
  padding: 10px;
  border-radius: 8px;
  margin-top: 10px;
  font-size: 0.85rem;
`;

const NoPlayersMessage = styled.div`
  text-align: center;
  color: #666;
  font-style: italic;
  padding: 40px 20px;
  background: rgba(0, 0, 0, 0.05);
  border-radius: 10px;
  margin-top: 20px;
`;

const getPlayerStateText = (state) => {
  const stateMap = {
    0: '오프라인',
    1: '온라인',
    2: '게임 중',
    3: '전투 중',
    4: '사망'
  };
  return stateMap[state] || '알 수 없음';
};

const getPlayerStateColor = (state) => {
  const colorMap = {
    0: '#9E9E9E',
    1: '#4CAF50',
    2: '#2196F3',
    3: '#FF9800',
    4: '#F44336'
  };
  return colorMap[state] || '#9E9E9E';
};

const PlayerDashboard = () => {
  const { players } = useWebSocket();

  return (
    <DashboardContainer
      initial={{ opacity: 0, y: 20 }}
      animate={{ opacity: 1, y: 0 }}
      transition={{ duration: 0.6 }}
    >
      <Title>
        👥 플레이어 현황
        <span style={{ 
          background: '#667eea', 
          color: 'white', 
          padding: '4px 8px', 
          borderRadius: '12px', 
          fontSize: '0.8rem',
          fontWeight: 'normal'
        }}>
          {players.length}명
        </span>
      </Title>

      {players.length === 0 ? (
        <NoPlayersMessage>
          현재 활성화된 플레이어가 없습니다.
        </NoPlayersMessage>
      ) : (
        <PlayerGrid>
          {players.map((player) => (
            <PlayerCard
              key={player.id}
              initial={{ opacity: 0, scale: 0.9 }}
              animate={{ opacity: 1, scale: 1 }}
              transition={{ duration: 0.3 }}
              whileHover={{ scale: 1.02 }}
            >
              <PlayerName>
                🎮 {player.name}
                <span style={{
                  background: getPlayerStateColor(player.state),
                  color: 'white',
                  padding: '2px 6px',
                  borderRadius: '8px',
                  fontSize: '0.7rem',
                  fontWeight: 'normal'
                }}>
                  {getPlayerStateText(player.state)}
                </span>
              </PlayerName>

              <PlayerInfo>
                <InfoItem>
                  <InfoLabel>레벨:</InfoLabel>
                  {player.stats?.level || 1}
                </InfoItem>
                <InfoItem>
                  <InfoLabel>경험치:</InfoLabel>
                  {player.stats?.experience || 0}
                </InfoItem>
                <InfoItem>
                  <InfoLabel>마나:</InfoLabel>
                  {player.stats?.mana || 0}/{player.stats?.max_mana || 0}
                </InfoItem>
                <InfoItem>
                  <InfoLabel>맵 ID:</InfoLabel>
                  {player.current_map_id || 0}
                </InfoItem>
              </PlayerInfo>

              <HealthBar>
                <HealthFill 
                  health={player.stats ? (player.stats.health / player.stats.max_health) * 100 : 0}
                />
              </HealthBar>

              <PositionInfo>
                <div><strong>위치:</strong> ({player.position?.x?.toFixed(2) || 0}, {player.position?.y?.toFixed(2) || 0}, {player.position?.z?.toFixed(2) || 0})</div>
                <div><strong>회전:</strong> {player.position?.rotation?.toFixed(2) || 0}°</div>
                <div><strong>생존:</strong> {player.is_alive ? '✅ 살아있음' : '❌ 사망'}</div>
              </PositionInfo>
            </PlayerCard>
          ))}
        </PlayerGrid>
      )}
    </DashboardContainer>
  );
};

export default PlayerDashboard;
