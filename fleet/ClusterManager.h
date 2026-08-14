#pragma once
#include "FleetTypes.h"
#include <vector>
#include <memory>

namespace DroneFleet {

class ClusterManager {
public:
    ClusterManager();
    ~ClusterManager() = default;

    void Init();
    void SpawnCluster();
    void SpawnCustomCluster(bool forceArmed, int forceMag = 0);
    void Update(FleetContext& ctx, float dt);
    void Draw3D(const std::vector<std::unique_ptr<Drone>>& drones) const;

    EnemyCluster* GetClusterById(int id);
    std::vector<EnemyCluster>& GetClusters() { return clusters; }
    const std::vector<EnemyCluster>& GetClusters() const { return clusters; }

private:
    std::vector<EnemyCluster> clusters;
    int nextClusterId;
};

} // namespace DroneFleet
