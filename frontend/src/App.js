import React, { useState, useEffect, useCallback } from 'react';
import styled from 'styled-components';
import { motion, AnimatePresence } from 'framer-motion';
import MonsterDashboard from './components/MonsterDashboard';
import PlayerDashboard from './components/PlayerDashboard';
import ServerStats from './components/ServerStats';
import ConnectionStatus from './components/ConnectionStatus';
import { WebSocketProvider } from './context/WebSocketContext';

const AppContainer = styled.div`
  min-height: 100vh;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  padding: 20px;
  font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
`;

const Header = styled(motion.div)`
  text-align: center;
  color: white;
  margin-bottom: 30px;
`;

const Title = styled.h1`
  font-size: 2.5rem;
  margin: 0;
  text-shadow: 2px 2px 4px rgba(0,0,0,0.3);
`;

const Subtitle = styled.p`
  font-size: 1.2rem;
  margin: 10px 0 0 0;
  opacity: 0.9;
`;

const DashboardGrid = styled.div`
  display: grid;
  grid-template-columns: 1fr 300px;
  gap: 20px;
  max-width: 1400px;
  margin: 0 auto;
  
  @media (max-width: 1200px) {
    grid-template-columns: 1fr;
  }
`;

const MainContent = styled.div`
  display: flex;
  flex-direction: column;
  gap: 20px;
`;

const Sidebar = styled.div`
  display: flex;
  flex-direction: column;
  gap: 20px;
`;

function App() {
  const [connectionStatus, setConnectionStatus] = useState('disconnected');
  const [serverStats, setServerStats] = useState({
    totalMonsters: 0,
    activeMonsters: 0,
    totalPlayers: 0,
    registeredBTTrees: 0,
    connectedClients: 0
  });

  const handleConnectionChange = useCallback((status) => {
    setConnectionStatus(status);
  }, []);

  const handleServerStatsUpdate = useCallback((stats) => {
    setServerStats(stats);
  }, []);

  return (
    <WebSocketProvider
      onConnectionChange={handleConnectionChange}
      onServerStatsUpdate={handleServerStatsUpdate}
    >
      <AppContainer>
        <Header
          initial={{ opacity: 0, y: -20 }}
          animate={{ opacity: 1, y: 0 }}
          transition={{ duration: 0.6 }}
        >
          <Title>🎮 몬스터 AI 모니터링 대시보드</Title>
          <Subtitle>실시간 Behavior Tree AI 상태 모니터링</Subtitle>
        </Header>

        <DashboardGrid>
          <MainContent>
            <MonsterDashboard />
            <PlayerDashboard />
          </MainContent>
          
          <Sidebar>
            <ConnectionStatus status={connectionStatus} />
            <ServerStats stats={serverStats} />
          </Sidebar>
        </DashboardGrid>
      </AppContainer>
    </WebSocketProvider>
  );
}

export default App;
