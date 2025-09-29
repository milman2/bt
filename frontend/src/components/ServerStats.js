import React from 'react';
import styled from 'styled-components';
import { motion } from 'framer-motion';

const StatsContainer = styled(motion.div)`
  background: white;
  border-radius: 15px;
  padding: 25px;
  box-shadow: 0 10px 30px rgba(0,0,0,0.1);
`;

const Title = styled.h3`
  margin: 0 0 20px 0;
  color: #333;
  font-size: 1.3rem;
  display: flex;
  align-items: center;
  gap: 10px;
`;

const StatGrid = styled.div`
  display: grid;
  grid-template-columns: 1fr;
  gap: 15px;
`;

const StatItem = styled.div`
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 12px 16px;
  background: #f8f9fa;
  border-radius: 10px;
  border-left: 4px solid ${props => props.color || '#667eea'};
`;

const StatLabel = styled.span`
  font-size: 0.9rem;
  color: #666;
  font-weight: 500;
`;

const StatValue = styled.span`
  font-size: 1.1rem;
  color: #333;
  font-weight: bold;
`;

const StatusIndicator = styled.div`
  width: 8px;
  height: 8px;
  border-radius: 50%;
  background-color: ${props => props.color};
  margin-right: 8px;
`;

const Icon = styled.span`
  font-size: 1.2rem;
`;

function ServerStats({ stats }) {
  const statItems = [
    {
      label: '총 몬스터',
      value: stats.totalMonsters,
      color: '#667eea',
      icon: '🤖'
    },
    {
      label: '활성 몬스터',
      value: stats.activeMonsters,
      color: '#4CAF50',
      icon: '⚡'
    },
    {
      label: '총 플레이어',
      value: stats.totalPlayers,
      color: '#2196F3',
      icon: '👥'
    },
    {
      label: '등록된 BT',
      value: stats.registeredBTTrees,
      color: '#FF9800',
      icon: '🌳'
    },
    {
      label: '연결된 클라이언트',
      value: stats.connectedClients,
      color: '#9C27B0',
      icon: '🔗'
    }
  ];

  return (
    <StatsContainer
      initial={{ opacity: 0, y: 20 }}
      animate={{ opacity: 1, y: 0 }}
      transition={{ duration: 0.5 }}
    >
      <Title>
        <Icon>📊</Icon>
        서버 통계
      </Title>
      
      <StatGrid>
        {statItems.map((item, index) => (
          <motion.div
            key={item.label}
            initial={{ opacity: 0, x: -20 }}
            animate={{ opacity: 1, x: 0 }}
            transition={{ duration: 0.3, delay: index * 0.1 }}
          >
            <StatItem color={item.color}>
              <StatLabel>
                <StatusIndicator color={item.color} />
                {item.icon} {item.label}
              </StatLabel>
              <StatValue>{item.value}</StatValue>
            </StatItem>
          </motion.div>
        ))}
      </StatGrid>
    </StatsContainer>
  );
}

export default ServerStats;
