
struct NodeConfig {

    uint box_size;
    float box_extent;
    float mass;

};

void GeneratePoints(std::vector<glm::vec3>& points_out, uint box_size, float box_extent) {

    float step = box_extent/(box_size-1) * 2;

    float pos_x = -box_extent;
    float pos_y = -box_extent;
    float pos_z = -box_extent;
    points_out.clear();

    for (size_t step_x = 0; step_x < box_size; step_x++) {
        pos_y = -box_extent;
        for (size_t step_y = 0; step_y < box_size; step_y++) {
            pos_z = -box_extent;
            for (size_t step_z = 0; step_z < box_size; step_z++) {
                points_out.push_back(glm::vec3(pos_x, pos_y, pos_z));
                pos_z += step;
            }
            pos_y += step;
        }
        pos_x += step;
    }
}

void GenerateConnections(std::vector<std::pair<uint, uint>>& connections, uint node_count) {

    connections.clear();
    for (uint x = 0; x < node_count; x++) {
        for (uint y = 0; y < node_count; y++) {
            for (uint z = 0; z < node_count; z++) {
                const uint now_node = x * node_count * node_count + y * node_count + z;
                if (x < node_count - 1) {
                    const uint node = (x+1) * node_count * node_count + (y+0) * node_count + (z+0);
                    connections.emplace_back(now_node, node);
                }
                if (y < node_count - 1) {
                    const uint node = (x+0) * node_count * node_count + (y+1) * node_count + (z+0);
                    connections.emplace_back(now_node, node);
                }
                if (z < node_count - 1) {
                    const uint node = (x+0) * node_count * node_count + (y+0) * node_count + (z+1);
                    connections.emplace_back(now_node, node);
                }
            }
        }
    }
    //2D Sides
    //XZ
    for (int zstep = -1; zstep < 2; zstep+=2) {
        for (int xstep = -1; xstep < 2; xstep+=2) {
            // printf("[Step Combination] (%*d, %*d, %*d) [XZ]\n", 2, xstep, 2, 0, 2, zstep);
            for (int xs = 0; xs < node_count; xs++) {
                for (int ys = 0; ys < node_count; ys++) {
                    if (((xs == 0 || xs == node_count-1)) && zstep < 0) continue; //Make sure the corners aren't done multiple times
                    int ystep = 0;
                    int x = xs;
                    int y = ys;
                    int z = (zstep<0) ? (node_count-1) : 0;
                    while (true) {
                        if (x + xstep < 0 || y + ystep < 0 || z + zstep < 0) break;
                        if (x + xstep >= node_count || y + ystep >= node_count || z + zstep >= node_count) break;
                        connections.push_back({x * node_count * node_count + y * node_count + z, (x+xstep) * node_count * node_count + (y+ystep) * node_count + (z+zstep)});
                        x += xstep;
                        y += ystep;
                        z += zstep;
                    }
                }
            }
        }
    }
    //XY
    for (int ystep = -1; ystep < 2; ystep+=2) {
        for (int xstep = -1; xstep < 2; xstep+=2) {
            for (int xs = 0; xs < node_count; xs++) {
                for (int zs = 0; zs < node_count; zs++) {
                    if (((xs == 0 || xs == node_count-1)) && ystep < 0) continue; //Make sure the corners aren't done multiple times
                    int zstep = 0;
                    int x = xs;
                    int y = (ystep<0) ? (node_count-1) : 0;
                    int z = zs;
                    while (true) {
                        if (x + xstep < 0 || y + ystep < 0 || z + zstep < 0) break;
                        if (x + xstep >= node_count || y + ystep >= node_count || z + zstep >= node_count) break;
                        connections.push_back({x * node_count * node_count + y * node_count + z, (x+xstep) * node_count * node_count + (y+ystep) * node_count + (z+zstep)});
                        x += xstep;
                        y += ystep;
                        z += zstep;
                    }
                }
            }
        }
    }
    //ZY
    for (int ystep = -1; ystep < 2; ystep+=2) {
        for (int zstep = -1; zstep < 2; zstep+=2) {
            for (int xs = 0; xs < node_count; xs++) {
                for (int zs = 0; zs < node_count; zs++) {
                    if (((zs == 0 || zs == node_count-1)) && ystep < 0) continue; //Make sure the corners aren't done multiple times
                    int xstep = 0;
                    int x = xs;
                    int y = (ystep<0) ? (node_count-1) : 0;
                    int z = zs;
                    while (true) {
                        if (x + xstep < 0 || y + ystep < 0 || z + zstep < 0) break;
                        if (x + xstep >= node_count || y + ystep >= node_count || z + zstep >= node_count) break;
                        connections.push_back({x * node_count * node_count + y * node_count + z, (x+xstep) * node_count * node_count + (y+ystep) * node_count + (z+zstep)});
                        x += xstep;
                        y += ystep;
                        z += zstep;
                    }
                }
            }
        }
    }
    //Diagonals
    for (int ystep = -1; ystep < 2; ystep+=2) {
        for (int xstep = -1; xstep < 2; xstep+=2) {
            for (int zstep = -1; zstep < 2; zstep+=2) {
                for (int xs = 0; xs < node_count; xs++) {
                    for (int zs = 0; zs < node_count; zs++) {
                        if ((xs == 0 || xs == node_count-1) && (zs == 0 || zs == node_count-1) && ystep<0) continue; //Make sure the corners aren't done multiple times
                        int x = xs;
                        int z = zs;
                        int y = (ystep < 0) ? (node_count - 1) : 0;
                        while (true) {
                            if (x + xstep < 0 || y + ystep < 0 || z + zstep < 0) break;
                            if (x + xstep >= node_count || y + ystep >= node_count || z + zstep >= node_count) break;
                            connections.push_back({x * node_count * node_count + y * node_count + z, (x+xstep) * node_count * node_count + (y+ystep) * node_count + (z+zstep)});
                            x += xstep;
                            y += ystep;
                            z += zstep;
                        }
                    }
                }
            }
        }
    }
    //Extra diagonals
    for (int ystep = -1; ystep < 2; ystep+=2) {
        for (int xstep = -1; xstep < 2; xstep+=2) {
            for (int zstep = -1; zstep < 2; zstep+=2) {
                for (int ys = 1; ys < node_count-1; ys++) {
                    for (int xs = 0; xs < node_count; xs++) {
                        int ystepsuntil = (ystep<0) ? ys : node_count - ys - 1; //Make sure that it isn't covered by the previous diagonals, as to not have duplicates
                        int xstepsuntil = (xstep<0) ? xs : node_count - xs - 1;
                        if (!(xstepsuntil<ystepsuntil)) continue;
                        int x = xs;
                        int z = (zstep < 0) ? node_count-1 : 0;
                        int y = ys;
                        while (true) {
                            if (x + xstep < 0 || y + ystep < 0 || z + zstep < 0) break;
                            if (x + xstep >= node_count || y + ystep >= node_count || z + zstep >= node_count) break;
                            connections.push_back({x * node_count * node_count + y * node_count + z, (x+xstep) * node_count * node_count + (y+ystep) * node_count + (z+zstep)});
                            x += xstep;
                            y += ystep;
                            z += zstep;
                        }
                    }
                }
            }
        }
    }
}

void GenerateNodes (
        std::vector<Node>& nodes, 
        std::vector<Connection>& connections,
        NodeConfig config) {

    std::vector<glm::vec3> points;
    GeneratePoints(points, config.box_size, config.box_extent);
    std::vector<std::pair<uint,uint>> tmp_connections;
    GenerateConnections(tmp_connections, config.box_size);

    nodes.clear();
    for (auto& point : points) {
        auto p4 = glm::vec4(point.x, point.y, point.z, 0);
        nodes.emplace_back(p4, glm::vec4(0), config.mass);
        for (uint i = 0; i < 30; i++) {
            nodes.back().connections[i] = -1;
        }
    }
    connections.resize(tmp_connections.size());
    for (size_t i = 0; i < connections.size(); i++) {
        auto& connection = connections[i];
        connection.first = tmp_connections[i].first;
        connection.second = tmp_connections[i].second;
        auto& node1 = nodes[connection.first];
        auto& node2 = nodes[connection.second];
        connection.neutral_length = glm::length(node1.pos - node2.pos);

        uint head;
        for (head = 0; node1.connections[head] != -1; head++);
        if (head >= 30) continue;
        node1.connections[head] = i;

        for (head = 0; node2.connections[head] != -1; head++);
        if (head >= 30) continue;
        node2.connections[head] = i;
    }
}
