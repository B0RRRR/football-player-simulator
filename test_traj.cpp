#include <iostream>
#include <cmath>

struct Vector2f { float x, y; };

int main() {
    float dt = 0.016f; // 60 FPS
    
    int m_attackPhase = 0;
    float m_stateTimer = 0.f;
    int m_ballCarrierIdx = 9;
    Vector2f m_ballTarget = {700.f, 290.f};
    Vector2f m_visualBall = {700.f, 290.f};
    bool isHome = true;
    bool isGoal = false;
    bool isSave = false;
    float m_shotTargetY = 290.f;
    
    for (int frame = 0; frame < 150; ++frame) {
        m_stateTimer += dt;
        
        if (m_attackPhase == 0) {
            if (m_stateTimer > 1.5f) {
                m_attackPhase = 2;
                m_stateTimer = 0.f;
                std::cout << "Transition to Phase 2 at frame " << frame << "\n";
            }
        } else if (m_attackPhase == 2) {
            m_ballCarrierIdx = -1;
            
            if (isGoal) {
                m_ballTarget = isHome ? Vector2f{860.f, m_shotTargetY} : Vector2f{20.f, m_shotTargetY};
            } else if (isSave) {
                m_ballTarget = isHome ? Vector2f{810.f, m_shotTargetY} : Vector2f{70.f, m_shotTargetY};
            } else {
                m_ballTarget = isHome ? Vector2f{860.f, m_shotTargetY > 290.f ? 360.f : 220.f} : Vector2f{20.f, m_shotTargetY > 290.f ? 360.f : 220.f};
            }
            
            float ballDist = std::hypot(m_ballTarget.x - m_visualBall.x, m_ballTarget.y - m_visualBall.y);
            
            if (m_stateTimer > 1.0f || (isSave && ballDist < 10.f)) {
                m_attackPhase = 3;
                m_stateTimer = 0.f;
                std::cout << "Transition to Phase 3 at frame " << frame << "\n";
            }
        } else if (m_attackPhase == 3) {
            std::cout << "In Phase 3 at frame " << frame << "\n";
            break;
        }
        
        // updateVisuals
        if (m_ballCarrierIdx != -1) {
            m_ballTarget = m_visualBall; // Simplified
        }
        
        float globalBDist = std::hypot(m_ballTarget.x - m_visualBall.x, m_ballTarget.y - m_visualBall.y);
        if (m_ballCarrierIdx != -1 && globalBDist < 10.f) {
            m_visualBall = m_ballTarget;
        } else if (globalBDist > 0.f) {
            float bdirX = m_ballTarget.x - m_visualBall.x;
            float bdirY = m_ballTarget.y - m_visualBall.y;
            float bspeed = 500.f;
            
            float moveX = (bdirX / globalBDist) * bspeed * dt;
            float moveY = (bdirY / globalBDist) * bspeed * dt;
            
            if (std::abs(moveX) > std::abs(bdirX)) moveX = bdirX;
            if (std::abs(moveY) > std::abs(bdirY)) moveY = bdirY;
            
            m_visualBall.x += moveX;
            m_visualBall.y += moveY;
        }
        
        if (m_attackPhase == 2) {
            std::cout << "Phase 2 Frame " << frame << " Ball: " << m_visualBall.x << ", " << m_visualBall.y << " Target: " << m_ballTarget.x << ", " << m_ballTarget.y << "\n";
        }
    }
    return 0;
}
