#pragma once
#include "Oxide.h"
#include "SoftBody.h"
#include "Drawer.h"
#include <atomic>

struct PhysicsCtx {

    std::mutex& BodyMtx;
    std::vector<Ref<SoftBody>>& Bodies;

    std::atomic_int& pause;
    std::atomic_bool& stop;
    std::atomic<double>& TS;
    std::atomic<double>& time_passed;

};

#define TSf ((float)TS)
class PhysicsHandler : public Oxide::Actor {
public:

    ~PhysicsHandler();

    void OnStart() override;
    void EachFrame() override;

    void AddSoftBody(Ref<SoftBody> body);
    std::atomic<double> time_passed = 0.0;
    std::atomic<double> TS = 0.0001;

private:

    void DrawBodies();
    void DrawUI();
    float CalculateEnergy(const SoftBody& body) const;

    std::vector<Ref<SoftBody>> m_Bodies;
    std::vector<float> m_DrawScales;
    std::vector<int> m_Collapsed;
    Mesh m_NodeProp;

    Ref<MeshRenderer> meshrenderer;
    uint m_NodeMeshHandel = 0;

    std::thread physicsthread;
    std::atomic<int> pause = 0;
    std::atomic<bool> stop = 0;

    std::mutex usingBodiesArray;

    Ref<GizmoDrawer> gizmodrawer;

    static void DoPhysics(PhysicsCtx ctx);
    static void TickUpdate(SoftBody& body, const double ts);
    static void CalculateForces(const SoftBody& body, SoftRepresentation& source);

ACTOR_ESSENTIALS(PhysicsHandler)

};