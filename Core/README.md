# ARCOS (Core)
This folder contains all of the files and systems required to drive ARCOS.

## Components
- hal
- drivers
- ai (artificial intelligence)
- inpr (input process)
- robo (robotics)
- nav (navigation)
- impr (image process)
- ui (user interface)
- gfx (graphics)
    - shader
    - engine
    - vector
    - raster
- math

### Explaination
| Component | Explaination|
|-----------|-------------|
| hal       | Hardware abstraction including pins, registers, protocals and internal timers             |
| drivers   | Predefined registers, addresses and protocals to drive external or internal components    |
| ai        | Tools and systems required to create and manipulate neural networks                       |
| inpr      | Filtering, isolation and other processing for sensor inputs                               |
| robo      | Robotic algorithms such as kinematics and collision detection                             |
| nav       | Navigation and path finding algorithms such as slam (mainly used in robotics)             |
| impr      | Image processing for computer vision, analysis, filtering and mathematical transforms     |
| ui        | User interface (UI) and user experience (ux), menus and other human interface devices     |
| gfx       | Graphical engines, shaders and other effects for compositing and overlaying               |
| math      | Custom functions, algorithms and data/structure conversions for other processing          |


# To Do List
- ESP32 S2 HAL
- HUB75 Protocal