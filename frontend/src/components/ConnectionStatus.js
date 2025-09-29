import React from 'react';
import styled from 'styled-components';
import { motion } from 'framer-motion';

const StatusContainer = styled(motion.div)`
  background: white;
  border-radius: 15px;
  padding: 20px;
  box-shadow: 0 10px 30px rgba(0,0,0,0.1);
  margin-bottom: 20px;
`;

const StatusHeader = styled.div`
  display: flex;
  align-items: center;
  gap: 10px;
  margin-bottom: 15px;
`;

const StatusTitle = styled.h3`
  margin: 0;
  color: #333;
  font-size: 1.2rem;
`;

const StatusIndicator = styled.div`
  width: 12px;
  height: 12px;
  border-radius: 50%;
  background-color: ${props => {
    switch (props.status) {
      case 'connected': return '#4CAF50';
      case 'connecting': return '#FF9800';
      case 'disconnected': return '#F44336';
      case 'error': return '#F44336';
      default: return '#9E9E9E';
    }
  }};
  animation: ${props => props.status === 'connecting' ? 'pulse 1.5s infinite' : 'none'};

  @keyframes pulse {
    0% { opacity: 1; }
    50% { opacity: 0.3; }
    100% { opacity: 1; }
  }
`;

const StatusText = styled.span`
  font-size: 0.9rem;
  color: ${props => {
    switch (props.status) {
      case 'connected': return '#4CAF50';
      case 'connecting': return '#FF9800';
      case 'disconnected': return '#F44336';
      case 'error': return '#F44336';
      default: return '#9E9E9E';
    }
  }};
  font-weight: 600;
  text-transform: uppercase;
  letter-spacing: 0.5px;
`;

const StatusMessage = styled.p`
  margin: 0;
  font-size: 0.8rem;
  color: #666;
  line-height: 1.4;
`;

const ReconnectButton = styled.button`
  margin-top: 10px;
  padding: 8px 16px;
  background: #667eea;
  color: white;
  border: none;
  border-radius: 6px;
  font-size: 0.8rem;
  cursor: pointer;
  transition: all 0.2s ease;

  &:hover {
    background: #5a6fd8;
    transform: translateY(-1px);
  }

  &:active {
    transform: translateY(0);
  }
`;

function ConnectionStatus({ status }) {
  const getStatusInfo = () => {
    switch (status) {
      case 'connected':
        return {
          text: '연결됨',
          message: 'WebSocket 서버와 실시간 연결이 활성화되었습니다.',
          showButton: false
        };
      case 'connecting':
        return {
          text: '연결 중',
          message: 'WebSocket 서버에 연결을 시도하고 있습니다...',
          showButton: false
        };
      case 'disconnected':
        return {
          text: '연결 끊김',
          message: 'WebSocket 서버와의 연결이 끊어졌습니다. 자동으로 재연결을 시도합니다.',
          showButton: true
        };
      case 'error':
        return {
          text: '연결 오류',
          message: 'WebSocket 서버 연결 중 오류가 발생했습니다.',
          showButton: true
        };
      default:
        return {
          text: '알 수 없음',
          message: '연결 상태를 확인할 수 없습니다.',
          showButton: true
        };
    }
  };

  const statusInfo = getStatusInfo();

  return (
    <StatusContainer
      initial={{ opacity: 0, scale: 0.95 }}
      animate={{ opacity: 1, scale: 1 }}
      transition={{ duration: 0.3 }}
    >
      <StatusHeader>
        <StatusIndicator status={status} />
        <StatusTitle>연결 상태</StatusTitle>
      </StatusHeader>
      
      <StatusText status={status}>
        {statusInfo.text}
      </StatusText>
      
      <StatusMessage>
        {statusInfo.message}
      </StatusMessage>
      
      {statusInfo.showButton && (
        <ReconnectButton onClick={() => window.location.reload()}>
          재연결 시도
        </ReconnectButton>
      )}
    </StatusContainer>
  );
}

export default ConnectionStatus;
