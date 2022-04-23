#pragma once
#include "Oxide.h"
#include "SoftBody.h"
#include "Drawer.h"
#include <atomic>
#include <thread_pool/thread_pool.hpp>

extern thread_pool PhysicsThreadPool;

struct PhysicsCtx {

    std::vector<Ref<SoftBody>>& Bodies;
    std::mutex& executing;

    std::atomic<int>& run;
    std::atomic<bool>& stop;
    std::atomic<bool>& singlethreaded;
    std::atomic<float>& TS;
    std::atomic<float>& time_passed;

    std::atomic<float>& tick_time;

};

#define TSf ((float)TS)
class PhysicsHandler : public Oxide::Actor {
public:

    ~PhysicsHandler();

    void OnStart() override;
    void EachFrame() override;

    void DoTicks(size_t count);

    std::array<float, 3> CalculateEnergies();

    void AddSoftBody(Ref<SoftBody> body);
    void ClearSoftBodies();
	Ref<SoftBody> GetSoftBody(uint index) {return m_Softbodies[index];}

    std::atomic<float> time_passed {0.0};
    std::atomic<float> TS {0.0001};
    std::atomic<float> average_tick_time {1.0f};
    std::atomic<int> run {0};
    std::atomic<bool> stop {true};
    std::atomic<bool> singlethreaded {false};

    std::mutex executing;

private:


    void DrawUI();
    void DrawNodeUI(NodeData& nodedata, const Node& node);

    void DrawNodes();

    static std::array<float, 3> CalculateEnergy(const SoftBody& body);

    std::vector<Ref<SoftBody>> m_Softbodies;

    Mesh m_NodeProp;
    Ref<MeshRenderer> m_MeshRenderer;
    Ref<GizmoDrawer> m_GizmoDrawer;
    uint m_MeshNodeHandle = -1;

    std::thread m_PhysicsThread;

    float m_DrawScale = 0.02f;

    static void EngineMain(PhysicsCtx ctx);
    static void SimulateTick(PhysicsCtx& ctx, Ref<SoftBody> body, float start, float end, uint count, std::atomic<uint>& counter);

    static void EulerIntegration(Ref<SoftBody> host, std::vector<Node>& base, std::vector<Node>& derivative, std::vector<Node>& out, float TS, float start, float end);
    static void CalculateVelocities(Ref<SoftBody> host, std::vector<Node>& nodes, float start, float end);

ACTOR_ESSENTIALS(PhysicsHandler)

};
