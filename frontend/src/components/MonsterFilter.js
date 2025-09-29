import React from 'react';
import styled from 'styled-components';

const FilterContainer = styled.div`
  display: flex;
  gap: 15px;
  margin-bottom: 20px;
  flex-wrap: wrap;
  align-items: center;
`;

const FilterGroup = styled.div`
  display: flex;
  flex-direction: column;
  gap: 5px;
`;

const FilterLabel = styled.label`
  font-size: 0.8rem;
  color: #666;
  font-weight: 500;
  text-transform: uppercase;
  letter-spacing: 0.5px;
`;

const FilterSelect = styled.select`
  padding: 8px 12px;
  border: 1px solid #ddd;
  border-radius: 6px;
  background: white;
  font-size: 0.9rem;
  color: #333;
  cursor: pointer;
  transition: border-color 0.2s ease;

  &:focus {
    outline: none;
    border-color: #667eea;
    box-shadow: 0 0 0 2px rgba(102, 126, 234, 0.1);
  }
`;

const SearchInput = styled.input`
  padding: 8px 12px;
  border: 1px solid #ddd;
  border-radius: 6px;
  font-size: 0.9rem;
  color: #333;
  min-width: 200px;
  transition: border-color 0.2s ease;

  &:focus {
    outline: none;
    border-color: #667eea;
    box-shadow: 0 0 0 2px rgba(102, 126, 234, 0.1);
  }

  &::placeholder {
    color: #999;
  }
`;

const ClearButton = styled.button`
  padding: 8px 16px;
  background: #f8f9fa;
  border: 1px solid #ddd;
  border-radius: 6px;
  font-size: 0.9rem;
  color: #666;
  cursor: pointer;
  transition: all 0.2s ease;

  &:hover {
    background: #e9ecef;
    border-color: #adb5bd;
  }

  &:active {
    transform: translateY(1px);
  }
`;

function MonsterFilter({ filter, onFilterChange }) {
  const handleFilterChange = (key, value) => {
    onFilterChange(prev => ({
      ...prev,
      [key]: value
    }));
  };

  const clearFilters = () => {
    onFilterChange({
      type: 'all',
      state: 'all',
      search: ''
    });
  };

  const hasActiveFilters = filter.type !== 'all' || filter.state !== 'all' || filter.search !== '';

  return (
    <FilterContainer>
      <FilterGroup>
        <FilterLabel>타입</FilterLabel>
        <FilterSelect
          value={filter.type}
          onChange={(e) => handleFilterChange('type', e.target.value)}
        >
          <option value="all">모든 타입</option>
          <option value="GOBLIN">고블린</option>
          <option value="ORC">오크</option>
          <option value="DRAGON">드래곤</option>
          <option value="SKELETON">스켈레톤</option>
          <option value="ZOMBIE">좀비</option>
          <option value="NPC_MERCHANT">상인</option>
          <option value="NPC_GUARD">경비</option>
        </FilterSelect>
      </FilterGroup>

      <FilterGroup>
        <FilterLabel>상태</FilterLabel>
        <FilterSelect
          value={filter.state}
          onChange={(e) => handleFilterChange('state', e.target.value)}
        >
          <option value="all">모든 상태</option>
          <option value="IDLE">대기</option>
          <option value="PATROL">순찰</option>
          <option value="CHASE">추적</option>
          <option value="ATTACK">공격</option>
          <option value="FLEE">도주</option>
          <option value="DEAD">사망</option>
        </FilterSelect>
      </FilterGroup>

      <FilterGroup>
        <FilterLabel>검색</FilterLabel>
        <SearchInput
          type="text"
          placeholder="몬스터 이름 검색..."
          value={filter.search}
          onChange={(e) => handleFilterChange('search', e.target.value)}
        />
      </FilterGroup>

      {hasActiveFilters && (
        <FilterGroup>
          <FilterLabel>&nbsp;</FilterLabel>
          <ClearButton onClick={clearFilters}>
            필터 초기화
          </ClearButton>
        </FilterGroup>
      )}
    </FilterContainer>
  );
}

export default MonsterFilter;
