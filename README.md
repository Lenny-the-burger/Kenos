# Kenos rendering engine

Kenos is a rendering engine that implements polygon tracing. It is written in c++ and uses the DirectX API for rendering. The documentation for usage and the theory behind it can be found [here](https://kenos-engine.notion.site/5517d573d2e547f8938a017df21f82f8?v=bce46534e4b545d9915899267490a21b&pvs=4)

The Kenos engine will eventually be released as an SDK that can be integrated into existing projects, but for now it is still in development.

## Requirments
- VS 2019 or later
- A graphics device supporting at least DirectX 11
- Windows 10

NuGet packages:
- DirectX Tool Kit (directxtk_desktop_2019)
- Assest importer library (Assimp)