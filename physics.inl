#include "PhysicsHandler.h"

//This file is appended at the end of PhysicsHandler.cpp

const char _physics_frame[] = "Physics Tick";

void PhysicsHandler::EngineMain(PhysicsCtx ctx) {

    std::deque<double> tick_times(30, 0.0);

    using clock = std::chrono::high_resolution_clock;
    while (!ctx.stop) {

        while (ctx.run && !ctx.stop) {
            
            FrameMarkStart(_physics_frame);
            auto tick_time = clock::now();
            ctx.run--;

            ctx.executing.lock();

            for (auto& body : ctx.Bodies) {
                body->copying.lock();
                body->copying.unlock();
                SimulateTick(ctx, body);
                PhysicsThreadPool.push_task(std::bind(&SoftBody::Swap, body.get()));
            }

            ctx.executing.unlock();

            tick_times.pop_back();
            double tdiff = std::chrono::duration_cast<std::chrono::microseconds>(clock::now().time_since_epoch() - tick_time.time_since_epoch()).count();
            tick_times.push_front(tdiff/(1000.0 * 1000.0));
            double tottime = 0.0;
            for (auto& time : tick_times) {
                tottime += time;
            }
            ctx.tick_time.store(tottime/30.0);
            ctx.time_passed.store(ctx.time_passed.load() + ctx.TS);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(7));
    }

    PhysicsThreadPool.wait_for_tasks();

}

void PhysicsHandler::SimulateTick(PhysicsCtx& ctx, Ref<SoftBody> body) {
    
    ZoneScoped;
    
    static std::vector<Node> derivative;
    static std::vector<Node> result;
    static std::vector<Node> combined;

    size_t nodecount = body->worknodes.size();


    CalculateVelocities(body, body->worknodes);
    derivative = body->worknodes;
    result = body->worknodes;
    combined = body->worknodes;
 
    //K2
    EulerIntegration(body, body->worknodes, body->worknodes, result, ctx.TS/2.f);
    CalculateVelocities(body, result);
    for (size_t i = 0; i < nodecount; i++) {
        combined[i].velocity += result[i].velocity * 2.f;
        combined[i].acceleration += result[i].acceleration * 2.f;
    }
    std::swap(derivative, result);

    //K3
    EulerIntegration(body, body->worknodes, derivative, result, ctx.TS/2.f);
    CalculateVelocities(body, result);
    for (size_t i = 0; i < nodecount; i++) {
        combined[i].velocity += result[i].velocity * 2.f;
        combined[i].acceleration += result[i].acceleration * 2.f;
    }
    std::swap(derivative, result);

    //K4
    EulerIntegration(body, body->worknodes, derivative, result, ctx.TS);
    CalculateVelocities(body, result);
    for (size_t i = 0; i < nodecount; i++) {
        combined[i].velocity += result[i].velocity;
        combined[i].acceleration += result[i].acceleration;
    }
    std::swap(derivative, result);

    //yn+1
    EulerIntegration(body, body->worknodes, combined, result, ctx.TS/6.f);
    std::swap(result, body->worknodes);
}

void PhysicsHandler::EulerIntegration(Ref<SoftBody> host, std::vector<Node>& base, std::vector<Node>& derivative, std::vector<Node>& out, float TS) {

    ZoneScoped

    for (size_t i = 0; i < base.size(); i++) {
        if (host->nodedata[i].is_locked.load()) continue;
        out[i].position = base[i].position + derivative[i].velocity * TS;
        out[i].velocity = base[i].velocity + derivative[i].acceleration * TS;
    }

}

void PhysicsHandler::CalculateVelocities(Ref<SoftBody> host, std::vector<Node>& nodes) {

    ZoneScoped

    const float d = host->drag.load();

    for (auto& n : nodes) {
        n.acceleration = n.velocity * d * -1.0f/n.mass;
        for (auto& acc : host->global_accelerations) {
            n.acceleration += acc;
        }
        for (auto& force : host->global_forces) {
            n.acceleration += force/n.mass;
        }
    }

    PhysicsThreadPool.parallelize_loop(0, host->connections.size(), [&host, &nodes](uint start, uint end) -> void {
        ZoneScopedN("loopchunk");
        for (uint i = start;i < end; i++) {
            auto& c = host->connections[i];
            auto& node1 = nodes[c.node1];
            auto& node2 = nodes[c.node2];

            glm::vec3 force = host->ForceCalculator(&c, nodes);
            node1.acceleration += force/node1.mass;
            node2.acceleration += -1.0f * force/node2.mass;
        }
    }, PhysicsThreadPool.get_thread_count() - 2);
}