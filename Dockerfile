FROM debian:bookworm AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    libasio-dev \
    libomp-dev \
    pkg-config \
    zlib1g-dev \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

RUN cmake -S . -B build/intermediate/build \
    -DCMAKE_BUILD_TYPE=Release \
    -DXTRACER_ENABLE_WEB=ON \
    -DXTRACER_ENABLE_WASM=OFF \
    -DXTRACER_ENABLE_WASM_DIST=OFF \
    -DXTRACER_ENABLE_VIZ=OFF \
 && cmake --build build/intermediate/build -j"$(nproc)"

FROM debian:bookworm-slim AS runtime

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    curl \
    libgomp1 \
    zlib1g \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=builder /src/build/intermediate/build/xtracer_web /usr/local/bin/xtracer_web
COPY --from=builder /src/scene /app/scene
COPY --from=builder /src/src/frontend/web-client /app/web-client

EXPOSE 8080

ENTRYPOINT ["/usr/local/bin/xtracer_web"]
CMD ["--host", "0.0.0.0", "--port", "8080", "--scene-dir", "/app/scene", "--web-root", "/app/web-client"]
