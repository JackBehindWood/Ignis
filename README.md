# Ignis
I am currently creating **Ignis**, a real-time explosion simulator and physics sandbox in C++ with destructible objects, particles, and tweakable parameters.

---

## 🎯 Project Overview

**Ignis** is a work-in-progress project aimed at building a modular simulation framework for:
- Physics-based particle systems  
- Destruction and debris simulation  
- GPU-accelerated visual effects  
- Interactive scene editing and scripting  

Built for macOS using **Metal** for high-performance GPU rendering.

---

## 🧰 Tech Stack

| Category | Technology |
|-----------|-------------|
| Language | C++17 |
| Graphics API | Metal |
| UI | Dear ImGui |
| Scripting | Python (via pybind11) |
| Architecture | Entity Component System (ECS) |
| Build System | Premake / CMake |
| Platform | macOS (Metal backend) |

---

## 🗺️ Roadmap

### 🚀 Phase 1: Core Simulation (Foundations)
- [x] Project setup (window, Metal init, render loop)
- [x] Basic camera controls and input
- [ ] Particle system (position, velocity, lifetime)
- [ ] Simple explosion physics (radial impulse, decay)
- [ ] Basic lighting and color gradient shaders
- [ ] Configurable explosion parameters (radius, power, duration)

---

### 💡 Phase 2: Visuals & Interaction
- [ ] Integrate Dear ImGui for parameter tweaking
- [ ] Add post-processing (bloom, motion blur)
- [ ] Camera shake and dynamic lighting
- [ ] Multiple simultaneous explosions
- [ ] Display performance metrics (FPS, particles)

---

### 🧱 Phase 3: Destruction System
- [ ] Add destructible objects (meshes or voxel fields)
- [ ] Apply explosion forces to fragments
- [ ] Collision and debris simulation
- [ ] Smoke and dust particles

---

### 🧩 Phase 4: Engine Architecture
- [ ] Introduce Entity Component System (ECS)
- [ ] Create component types (Particle, RigidBody, Mesh)
- [ ] Implement system updates for rendering, physics, scripting
- [ ] Resource management (shaders, textures, buffers)

---

### 🧠 Phase 5: Scripting & Automation
- [ ] Embed Python via pybind11
- [ ] Control scenes and explosions through scripts
- [ ] Batch tests and parameter sweeps
- [ ] Log simulation data for analysis

---

### 🧰 Phase 6: Sandbox & Editor
- [ ] ImGui-based scene editor (explosion controls, object placement)
- [ ] Save/load scene configurations (JSON)
- [ ] Real-time preview and playback controls
- [ ] “Play Mode” for interactive simulation

---

### 🌈 Phase 7: Advanced Features
- [ ] GPU compute for particle physics
- [ ] Fluid-based smoke simulation
- [ ] Volumetric lighting and shockwave effects
- [ ] AI-driven explosion placement or reaction systems
- [ ] Export as a mini simulation engine

---

## 🧩 Development Overview

| Phase | Name | Focus | Status |
|-------|------|--------|--------|
| 1️⃣ | Core Simulation | Rendering, physics, particles | 🟡 In Progress |
| 2️⃣ | Visuals & Interaction | Lighting, ImGui, effects | ⚪ Planned |
| 3️⃣ | Destruction System | Mesh destruction, debris | ⚪ Planned |
| 4️⃣ | Engine Architecture | ECS, modular systems | ⚪ Planned |
| 5️⃣ | Scripting & Automation | Python scripting | ⚪ Planned |
| 6️⃣ | Sandbox & Editor | Scene editor | ⚪ Planned |
| 7️⃣ | Advanced Features | Fluids, AI, compute | ⚪ Planned |

---

## 🧠 Vision

> “Explosions are complex systems — physics, light, heat, and motion interacting in milliseconds.  
> Ignis is my active project to simulate that chaos and learn how GPU physics, graphics, and real-time systems come together.”

Over time, Ignis will evolve into a modular simulation engine that blends **science, performance, and interactivity** — useful for research, visualization, or just creative experimentation.

---

## 🪪 License

This project is licensed under the **MIT License** — see [LICENSE](./LICENSE) for details.
