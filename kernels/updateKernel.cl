kernel void updateKernel(global float *positions, global float *velocities, int nrBodies) {
    // updates positions based on velocities of a body and timestep
    float timestep = 86400.0f; // seconds in a day
    int i = get_global_id(0);

    if (i < nrBodies) {
        if (i == 0) {
            positions[0] = 0.0f;
            positions[1] = 0.0f;
            positions[2] = 0.0f;
            return;
        }
        positions[i * 3] += velocities[i * 3] * timestep;
        positions[i * 3 + 1] += velocities[i * 3 + 1] * timestep;
        positions[i * 3 + 2] += velocities[i * 3 + 2] * timestep;
    }
}