# Kenos rendering engine

Kenos is a rendering engine that implements a novel light transport model. It is written in c++ and uses the DirectX API for rendering. The documentation for usage and the theory behind it can be found [pending]

The Kenos engine will eventually be released as an SDK that can be integrated into existing projects, but for now it is still in development.

Time i've sunk into this godforesaken engine:

[![wakatime](https://wakatime.com/badge/user/d180c526-abf9-476f-8a23-3c9d61319011/project/3ab00ff3-2a48-416a-a5b0-89a931215c7a.svg)](https://wakatime.com/badge/user/d180c526-abf9-476f-8a23-3c9d61319011/project/3ab00ff3-2a48-416a-a5b0-89a931215c7a)

## Gallery
![Area light](https://cdn.discordapp.com/attachments/962154339709890571/1141293540106850355/image.png)

Realtime mesh light rendering (excluding acceleration structure build time at startup). Shadows are not shown as I am still working on an efficient implementation, it is possible to use the same method to render them as the light but the shader is not optimized for it yet.

## Requirments
- VS 2019 or later
- A graphics device supporting at least DirectX 11
- Windows 10

NuGet packages:
- DirectX Tool Kit (directxtk_desktop_2019)
- Assest importer library (Assimp)
