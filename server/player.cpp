#include "player.h"
#include <iostream>
#include <chrono>

namespace bt {

Player::Player(uint32_t id, const std::string& name) 
    : id_(id), name_(name), state_(PlayerState::ONLINE), 
      current_map_id_(1), socket_fd_(-1) {
    
    // 기본 위치 설정
    position_.x = 0.0f;
    position_.y = 0.0f;
    position_.z = 0.0f;
    position_.rotation = 0.0f;
    
    // 기본 통계 설정
    stats_.level = 1;
    stats_.experience = 0;
    stats_.health = 100;
    stats_.max_health = 100;
    stats_.mana = 50;
    stats_.max_mana = 50;
    stats_.strength = 10;
    stats_.agility = 10;
    stats_.intelligence = 10;
    stats_.vitality = 10;
    
    // 활동 시간 초기화
    last_activity_ = std::chrono::steady_clock::now();
    last_update_time_ = std::chrono::steady_clock::now();
    
    std::cout << "플레이어 생성: " << name_ << " (ID: " << id_ << ")" << std::endl;
}

Player::~Player() {
    std::cout << "플레이어 소멸: " << name_ << " (ID: " << id_ << ")" << std::endl;
}

void Player::set_position(float x, float y, float z, float rotation) {
    position_.x = x;
    position_.y = y;
    position_.z = z;
    position_.rotation = rotation;
    update_activity();
}

void Player::move_to(float x, float y, float z, float rotation) {
    set_position(x, y, z, rotation);
    std::cout << "플레이어 " << name_ << " 이동: (" << x << ", " << y << ", " << z << ")" << std::endl;
}

void Player::add_experience(uint32_t exp) {
    stats_.experience += exp;
    update_activity();
    
    // 레벨업 체크 (간단한 구현)
    uint32_t required_exp = stats_.level * 100; // 레벨당 100 경험치 필요
    if (stats_.experience >= required_exp) {
        level_up();
    }
    
    std::cout << "플레이어 " << name_ << " 경험치 획득: " << exp 
              << " (총: " << stats_.experience << ")" << std::endl;
}

void Player::level_up() {
    stats_.level++;
    stats_.experience = 0;
    
    // 레벨업 시 능력치 증가
    stats_.max_health += 10;
    stats_.health = stats_.max_health; // 레벨업 시 체력 회복
    stats_.max_mana += 5;
    stats_.mana = stats_.max_mana; // 레벨업 시 마나 회복
    stats_.strength += 2;
    stats_.agility += 2;
    stats_.intelligence += 2;
    stats_.vitality += 2;
    
    update_activity();
    std::cout << "플레이어 " << name_ << " 레벨업! 새 레벨: " << stats_.level << std::endl;
}

void Player::take_damage(uint32_t damage) {
    if (damage >= stats_.health) {
        stats_.health = 0;
        state_ = PlayerState::DEAD;
        std::cout << "플레이어 " << name_ << " 사망!" << std::endl;
    } else {
        stats_.health -= damage;
        std::cout << "플레이어 " << name_ << " 데미지 받음: " << damage 
                  << " (남은 체력: " << stats_.health << ")" << std::endl;
    }
    update_activity();
}

void Player::heal(uint32_t amount) {
    if (state_ == PlayerState::DEAD) {
        return; // 죽은 플레이어는 치료할 수 없음
    }
    
    uint32_t old_health = stats_.health;
    stats_.health += amount;
    
    if (stats_.health > stats_.max_health) {
        stats_.health = stats_.max_health;
    }
    
    uint32_t actual_heal = stats_.health - old_health;
    if (actual_heal > 0) {
        std::cout << "플레이어 " << name_ << " 치료됨: " << actual_heal 
                  << " (현재 체력: " << stats_.health << ")" << std::endl;
        update_activity();
    }
}

void Player::update(float delta_time) {
    // 플레이어 상태 업데이트
    auto current_time = std::chrono::steady_clock::now();
    
    // 마나 자동 회복 (초당 1)
    if (stats_.mana < stats_.max_mana) {
        stats_.mana += static_cast<uint32_t>(delta_time);
        if (stats_.mana > stats_.max_mana) {
            stats_.mana = stats_.max_mana;
        }
    }
    
    // 체력 자동 회복 (초당 0.5, 전투 중이 아닐 때만)
    if (state_ != PlayerState::IN_COMBAT && stats_.health < stats_.max_health) {
        stats_.health += static_cast<uint32_t>(delta_time * 0.5f);
        if (stats_.health > stats_.max_health) {
            stats_.health = stats_.max_health;
        }
    }
    
    last_update_time_ = current_time;
}

void Player::respawn() {
    if (state_ != PlayerState::DEAD) {
        return;
    }
    
    // 부활 처리
    stats_.health = stats_.max_health;
    stats_.mana = stats_.max_mana;
    state_ = PlayerState::ONLINE;
    
    // 기본 위치로 이동
    set_position(0.0f, 0.0f, 0.0f, 0.0f);
    
    std::cout << "플레이어 " << name_ << " 부활!" << std::endl;
}

} // namespace bt
