import React, { useState, useMemo } from 'react';
import styled from 'styled-components';
import { motion, AnimatePresence } from 'framer-motion';
import { useWebSocket } from '../context/WebSocketContext';
import MonsterCard from './MonsterCard';
import MonsterFilter from './MonsterFilter';

const DashboardContainer = styled.div`
  background: white;
  border-radius: 15px;
  padding: 25px;
  box-shadow: 0 10px 30px rgba(0,0,0,0.1);
`;

const Header = styled.div`
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 25px;
  flex-wrap: wrap;
  gap: 15px;
`;

const Title = styled.h2`
  margin: 0;
  color: #333;
  font-size: 1.8rem;
`;

const StatsBar = styled.div`
  display: flex;
  gap: 20px;
  align-items: center;
  flex-wrap: wrap;
`;

const StatItem = styled.div`
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 8px 16px;
  background: #f8f9fa;
  border-radius: 20px;
  font-size: 0.9rem;
  color: #666;
`;

const StatNumber = styled.span`
  font-weight: bold;
  color: #667eea;
`;

const MonsterGrid = styled.div`
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(350px, 1fr));
  gap: 20px;
  margin-top: 20px;
`;

const EmptyState = styled(motion.div)`
  text-align: center;
  padding: 60px 20px;
  color: #666;
`;

const EmptyIcon = styled.div`
  font-size: 4rem;
  margin-bottom: 20px;
  opacity: 0.5;
`;

const EmptyText = styled.p`
  font-size: 1.1rem;
  margin: 0;
`;

const LoadingState = styled(motion.div)`
  text-align: center;
  padding: 40px;
  color: #667eea;
`;

const LoadingSpinner = styled(motion.div)`
  width: 40px;
  height: 40px;
  border: 3px solid #f3f3f3;
  border-top: 3px solid #667eea;
  border-radius: 50%;
  margin: 0 auto 20px;
`;

function MonsterDashboard() {
  const { monsters, connectionStatus } = useWebSocket();
  const [filter, setFilter] = useState({
    type: 'all',
    state: 'all',
    search: ''
  });

  const filteredMonsters = useMemo(() => {
    return monsters.filter(monster => {
      const matchesType = filter.type === 'all' || monster.type === filter.type;
      const matchesState = filter.state === 'all' || monster.state === filter.state;
      const matchesSearch = filter.search === '' || 
        monster.name.toLowerCase().includes(filter.search.toLowerCase());
      
      return matchesType && matchesState && matchesSearch;
    });
  }, [monsters, filter]);

  const monsterStats = useMemo(() => {
    const stats = {
      total: monsters.length,
      byType: {},
      byState: {}
    };

    monsters.forEach(monster => {
      // 타입별 통계
      stats.byType[monster.type] = (stats.byType[monster.type] || 0) + 1;
      
      // 상태별 통계
      stats.byState[monster.state] = (stats.byState[monster.state] || 0) + 1;
    });

    return stats;
  }, [monsters]);

  if (connectionStatus === 'disconnected') {
    return (
      <DashboardContainer>
        <LoadingState
          initial={{ opacity: 0 }}
          animate={{ opacity: 1 }}
          transition={{ duration: 0.5 }}
        >
          <LoadingSpinner
            animate={{ rotate: 360 }}
            transition={{ duration: 1, repeat: Infinity, ease: "linear" }}
          />
          <p>서버에 연결 중...</p>
        </LoadingState>
      </DashboardContainer>
    );
  }

  return (
    <DashboardContainer>
      <Header>
        <Title>몬스터 상태</Title>
        <StatsBar>
          <StatItem>
            총 몬스터: <StatNumber>{monsterStats.total}</StatNumber>
          </StatItem>
          {Object.entries(monsterStats.byType).map(([type, count]) => (
            <StatItem key={type}>
              {type}: <StatNumber>{count}</StatNumber>
            </StatItem>
          ))}
        </StatsBar>
      </Header>

      <MonsterFilter filter={filter} onFilterChange={setFilter} />

      <AnimatePresence mode="wait">
        {filteredMonsters.length === 0 ? (
          <EmptyState
            key="empty"
            initial={{ opacity: 0, y: 20 }}
            animate={{ opacity: 1, y: 0 }}
            exit={{ opacity: 0, y: -20 }}
            transition={{ duration: 0.3 }}
          >
            <EmptyIcon>🤖</EmptyIcon>
            <EmptyText>
              {monsters.length === 0 
                ? '현재 활성화된 몬스터가 없습니다.'
                : '필터 조건에 맞는 몬스터가 없습니다.'
              }
            </EmptyText>
          </EmptyState>
        ) : (
          <MonsterGrid>
            <AnimatePresence>
              {filteredMonsters.map((monster) => (
                <MonsterCard
                  key={monster.id}
                  monster={monster}
                  layout
                  initial={{ opacity: 0, scale: 0.9 }}
                  animate={{ opacity: 1, scale: 1 }}
                  exit={{ opacity: 0, scale: 0.9 }}
                  transition={{ duration: 0.3 }}
                />
              ))}
            </AnimatePresence>
          </MonsterGrid>
        )}
      </AnimatePresence>
    </DashboardContainer>
  );
}

export default MonsterDashboard;
