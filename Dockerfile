FROM debian:bookworm-slim AS builder
WORKDIR /app

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libssl-dev \
    zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

COPY CMakeLists.txt .

# tylko do zaleznosci
RUN cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF || true

COPY . .

RUN cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF
RUN cmake --build build --target cipereusz -j$(nproc)

# final
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
