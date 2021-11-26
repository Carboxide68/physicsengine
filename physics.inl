#include "PhysicsHandler.h"

//This file is appended at the end of PhysicsHandler.cpp

const char _physics_frame[] = "Physics Tick";

void PhysicsHandler::EngineMain(PhysicsCtx ctx) {

    std::deque<double> tick_times(30, 0.0);
    std::atomic<uint> copyings = {0};
    thread_pool personal_threads = thread_pool();
    personal_threads.sleep_duration = 0;

    using clock = std::chrono::high_resolution_clock;
    while (!ctx.stop) {

        while (ctx.run && !ctx.stop) {
            
            FrameMarkStart(_physics_frame);
            auto tick_time = clock::now();
            ctx.run--;

            ctx.executing.lock();
            
            while (copyings != 0) std::this_thread::yield();
            for (auto& body : ctx.Bodies) {
                body->copying.lock();
                body->copying.unlock();
                std::atomic<uint> counter = {0};
                std::atomic<uint> tasks = {0};
                if (body->worknodes.size() < 1000) {
                    SimulateTick(ctx, body, 0.f, 1.f, 1, counter);
                } else {
                    const uint tmp = personal_threads.get_thread_count();
                    const uint thread_count = (tmp < 6) ? 1 : tmp - 4;
                    {
                    ZoneScopedN("Launch Threads")
                    for (uint i = 0; i < thread_count; i++) {
                        tasks += 1;
                        personal_threads.push_task([&ctx, body, start=(float)(i)/(float)thread_count, end=(float)(i+1)/(float)thread_count, thread_count, &counter, &tasks]() {
                            PhysicsHandler::SimulateTick(ctx, body, start, end, thread_count, counter);
                            tasks -= 1;
                        });
                    }
                    }
                    while (tasks != 0) std::this_thread::sleep_for(std::chrono::microseconds(15));
                }
                copyings+=1;
                PhysicsThreadPool.push_task([body, &copyings](){body->Swap(); copyings-=1;});
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
            FrameMarkEnd(_physics_frame);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(7));
    }

    personal_threads.wait_for_tasks();
    PhysicsThreadPool.wait_for_tasks();

}

void PhysicsHandler::SimulateTick(PhysicsCtx& ctx, Ref<SoftBody> body, float start, float end, uint count, std::atomic<uint>& counter) {
    
    ZoneScoped;
    
    static std::vector<Node> derivative;
    static std::vector<Node> result;
    static std::vector<Node> combined;
    static std::condition_variable cv;
    static std::mutex cv_m;

    size_t nodecount = body->worknodes.size();
    if (start == 0.0) {
        ZoneScopedN("Resizing");
        derivative.resize(nodecount);
        combined.resize(nodecount);
        result.resize(nodecount);
    }

    counter += 1;
    {
    ZoneScopedN("Waiting")
    while (counter < count * 1) std::this_thread::sleep_for(std::chrono::microseconds(15));
    }
    CalculateVelocities(body, body->worknodes, start, end);
    for (uint i = (uint)(nodecount * start); i < (uint)(nodecount * end); i++) {
        derivative[i] = body->worknodes[i];
        result[i] = body->worknodes[i];
        combined[i] = body->worknodes[i];
    }
 
    counter += 1;
    {
    ZoneScopedN("Waiting")
    while (counter < count * 2) std::this_thread::sleep_for(std::chrono::microseconds(15));
    }

    //K2
    EulerIntegration(body, body->worknodes, body->worknodes, result, ctx.TS/2.f, start, end);
    CalculateVelocities(body, result, start, end);
    for (uint i = (uint)(nodecount * start); i < (uint)(nodecount * end); i++) {
        combined[i].velocity += result[i].velocity * 2.f;
        combined[i].acceleration += result[i].acceleration * 2.f;
    }

    counter += 1;
    {
    ZoneScopedN("Waiting")
    while (counter < count * 3) std::this_thread::sleep_for(std::chrono::microseconds(15));
    }

    if (start == 0.0) {
        std::swap(derivative, result);
    }

    //K3
    EulerIntegration(body, body->worknodes, derivative, result, ctx.TS/2.f, start, end);
    CalculateVelocities(body, result, start, end);
    for (uint i = (uint)(nodecount * start); i < (uint)(nodecount * end); i++) {
        combined[i].velocity += result[i].velocity * 2.f;
        combined[i].acceleration += result[i].acceleration * 2.f;
    }

    counter += 1;

    {
    ZoneScopedN("Waiting")
    while (counter < count * 4) std::this_thread::sleep_for(std::chrono::microseconds(15));
    }

    if (start == 0.0) {
        std::swap(derivative, result);
    }

    //K4
    EulerIntegration(body, body->worknodes, derivative, result, ctx.TS, start, end);
    CalculateVelocities(body, result, start, end);
    for (uint i = (uint)(nodecount * start); i < (uint)(nodecount * end); i++) {
        combined[i].velocity += result[i].velocity;
        combined[i].acceleration += result[i].acceleration;
    }

    counter += 1;

    {
    ZoneScopedN("Waiting")
    while (counter < count * 5) std::this_thread::sleep_for(std::chrono::microseconds(15));
    }

    if (start == 0.0) {
        std::swap(derivative, result);
    }

    //yn+1
    EulerIntegration(body, body->worknodes, combined, result, ctx.TS/6.f, start, end);

    counter += 1;
    {
    ZoneScopedN("Waiting")
    while (counter < count * 6) std::this_thread::sleep_for(std::chrono::microseconds(15));
    }

    if (start == 0.0) {
        std::swap(result, body->worknodes);
    }
}

void PhysicsHandler::EulerIntegration(Ref<SoftBody> host, std::vector<Node>& base, std::vector<Node>& derivative, std::vector<Node>& out, float TS, float start, float end) {

    ZoneScoped

    const size_t nodesize = base.size();
    for (uint i = (uint)(nodesize * start); i < (uint)(nodesize * end); i++) {
        if (host->nodedata[i].is_locked.load()) continue;
        out[i].position = base[i].position + derivative[i].velocity * TS;
        out[i].velocity = base[i].velocity + derivative[i].acceleration * TS;
    }

}

void PhysicsHandler::CalculateVelocities(Ref<SoftBody> host, std::vector<Node>& nodes, float start, float end) {

    ZoneScoped

    const float d = host->drag.load();

    const size_t nodesize = nodes.size();
    for (uint i = (uint)(nodesize * start); i < (uint)(nodesize * end); i++) {
        auto& n = nodes[i];
        n.acceleration = n.velocity * d * -1.0f/n.mass;
        for (auto& acc : host->global_accelerations) {
            n.acceleration += acc;
        }
        for (auto& force : host->global_forces) {
            n.acceleration += force/n.mass;
        }
    }

    const size_t connectionsize = host->connections.size();
    for (size_t i = (uint)(connectionsize * start); i < uint(connectionsize * end); i++) {
        auto& c = host->connections[i];
        auto& node1 = nodes[c.node1];
        auto& node2 = nodes[c.node2];

        glm::vec3 force = host->ForceCalculator(&c, nodes);
        node1.acceleration += force/node1.mass;
        node2.acceleration += -1.0f * force/node2.mass;
    }
}