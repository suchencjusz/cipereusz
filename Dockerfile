FROM debian:bookworm-slim AS builder
WORKDIR /app

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libssl-dev \
    zlib1g-dev \
    ccache \
    && rm -rf /var/lib/apt/lists/*

ENV CCACHE_DIR=/root/.ccache
ENV PATH="/usr/lib/ccache:$PATH"

COPY CMakeLists.txt .

RUN cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DCMAKE_CXX_COMPILER_LAUNCHER=ccache || true

COPY . .

RUN --mount=type=cache,target=/root/.ccache \
    cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DCMAKE_CXX_COMPILER_LAUNCHER=ccache

RUN --mount=type=cache,target=/root/.ccache \
    cmake --build build --target cipereusz -j$(nproc)

FROM debian:bookworm-slim
LABEL org.opencontainers.image.description="cipereusz discord bot"

RUN apt-get update && apt-get install -y \
    libssl3 \
    zlib1g \
    && rm -rf /var/lib/apt/lists/*

RUN useradd -m -s /bin/bash nonroot

WORKDIR /app

COPY --from=builder --chown=nonroot:nonroot /app/build/cipereusz .
COPY --chown=nonroot:nonroot entrypoint.sh /app/entrypoint.sh

RUN chmod +x /app/cipereusz /app/entrypoint.sh

USER nonroot

ENTRYPOINT ["/app/entrypoint.sh"]