import React from 'react';
import styled from 'styled-components';
import { motion } from 'framer-motion';

const Card = styled(motion.div)`
  background: white;
  border: 1px solid #e1e5e9;
  border-radius: 12px;
  padding: 20px;
  box-shadow: 0 4px 12px rgba(0,0,0,0.05);
  transition: all 0.3s ease;
  position: relative;
  overflow: hidden;

  &:hover {
    box-shadow: 0 8px 25px rgba(0,0,0,0.1);
    transform: translateY(-2px);
  }
`;

const CardHeader = styled.div`
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 15px;
`;

const MonsterName = styled.h3`
  margin: 0;
  font-size: 1.2rem;
  color: #333;
  display: flex;
  align-items: center;
  gap: 10px;
`;

const StatusIndicator = styled.div`
  width: 12px;
  height: 12px;
  border-radius: 50%;
  background-color: ${props => {
    switch (props.status) {
      case 'IDLE': return '#FFC107';
      case 'PATROL': return '#2196F3';
      case 'CHASE': return '#FF9800';
      case 'ATTACK': return '#F44336';
      case 'FLEE': return '#9C27B0';
      case 'DEAD': return '#9E9E9E';
      default: return '#4CAF50';
    }
  }};
  animation: ${props => props.status === 'PATROL' ? 'pulse 2s infinite' : 'none'};

  @keyframes pulse {
    0% { opacity: 1; }
    50% { opacity: 0.5; }
    100% { opacity: 1; }
  }
`;

const MonsterType = styled.span`
  background: #667eea;
  color: white;
  padding: 4px 8px;
  border-radius: 12px;
  font-size: 0.8rem;
  font-weight: 500;
`;

const CardContent = styled.div`
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 15px;
`;

const InfoGroup = styled.div`
  display: flex;
  flex-direction: column;
  gap: 8px;
`;

const InfoLabel = styled.span`
  font-size: 0.8rem;
  color: #666;
  font-weight: 500;
  text-transform: uppercase;
  letter-spacing: 0.5px;
`;

const InfoValue = styled.span`
  font-size: 0.9rem;
  color: #333;
  font-weight: 600;
`;

const PositionInfo = styled.div`
  grid-column: 1 / -1;
  background: #f8f9fa;
  padding: 12px;
  border-radius: 8px;
  margin-top: 10px;
`;

const HealthBar = styled.div`
  width: 100%;
  height: 6px;
  background: #e9ecef;
  border-radius: 3px;
  overflow: hidden;
  margin-top: 5px;
`;

const HealthFill = styled.div`
  height: 100%;
  background: ${props => {
    const percentage = (props.$health / props.$maxHealth) * 100;
    if (percentage > 60) return '#4CAF50';
    if (percentage > 30) return '#FF9800';
    return '#F44336';
  }};
  width: ${props => (props.$health / props.$maxHealth) * 100}%;
  transition: width 0.3s ease;
`;

const HealthText = styled.div`
  display: flex;
  justify-content: space-between;
  font-size: 0.8rem;
  color: #666;
  margin-top: 4px;
`;

const LastUpdate = styled.div`
  position: absolute;
  top: 10px;
  right: 10px;
  font-size: 0.7rem;
  color: #999;
`;

function MonsterCard({ monster }) {
  const formatPosition = (pos) => {
    return `(${pos.x.toFixed(1)}, ${pos.y.toFixed(1)}, ${pos.z.toFixed(1)})`;
  };

  const formatRotation = (rotation) => {
    return `${rotation.toFixed(1)}°`;
  };

  const getTimeSinceUpdate = () => {
    if (!monster.lastUpdate) return '방금 전';
    
    const now = Date.now();
    const diff = now - monster.lastUpdate;
    const seconds = Math.floor(diff / 1000);
    
    if (seconds < 60) return `${seconds}초 전`;
    const minutes = Math.floor(seconds / 60);
    if (minutes < 60) return `${minutes}분 전`;
    const hours = Math.floor(minutes / 60);
    return `${hours}시간 전`;
  };

  return (
    <Card
      whileHover={{ scale: 1.02 }}
      whileTap={{ scale: 0.98 }}
    >
      <LastUpdate>{getTimeSinceUpdate()}</LastUpdate>
      
      <CardHeader>
        <MonsterName>
          <StatusIndicator status={monster.state} />
          {monster.name}
        </MonsterName>
        <MonsterType>{monster.type}</MonsterType>
      </CardHeader>

      <CardContent>
        <InfoGroup>
          <InfoLabel>상태</InfoLabel>
          <InfoValue>{monster.state}</InfoValue>
        </InfoGroup>

        <InfoGroup>
          <InfoLabel>체력</InfoLabel>
          <HealthBar>
            <HealthFill 
              $health={monster.health} 
              $maxHealth={monster.max_health}
            />
          </HealthBar>
          <HealthText>
            <span>{monster.health}/{monster.max_health}</span>
            <span>{Math.round((monster.health / monster.max_health) * 100)}%</span>
          </HealthText>
        </InfoGroup>

        <InfoGroup>
          <InfoLabel>AI</InfoLabel>
          <InfoValue>{monster.ai_name || '없음'}</InfoValue>
        </InfoGroup>

        <InfoGroup>
          <InfoLabel>Behavior Tree</InfoLabel>
          <InfoValue>{monster.bt_name || '없음'}</InfoValue>
        </InfoGroup>
      </CardContent>

      <PositionInfo>
        <InfoLabel>위치</InfoLabel>
        <InfoValue>{formatPosition(monster.position)}</InfoValue>
        <InfoLabel style={{ marginTop: '8px' }}>회전</InfoLabel>
        <InfoValue>{formatRotation(monster.position.rotation)}</InfoValue>
      </PositionInfo>
    </Card>
  );
}

export default MonsterCard;
