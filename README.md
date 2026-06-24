# mympd & mympc

Daemon de audio y cliente CLI en C++20 con integración nativa para PipeWire y FFmpeg. Ligero, asíncrono y sin dependencias pesadas.

## Dependencias
* `pipewire` (`libpipewire-0.3`)
* `ffmpeg` (`libavcodec`, `libswresample`, `libavformat`)
* `cmake`

## Compilación
```bash
cd build
cmake ..
make
