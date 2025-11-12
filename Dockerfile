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

RUN cmake -B build -DCMAKE_BUILD_TYPE=Release

COPY . .

RUN cmake --build build -j$(nproc)


FROM debian:bookworm-slim
LABEL org.opencontainers.image.description="cipereusz discord bot"

RUN apt-get update && apt-get install -y \
    libssl3 \
    zlib1g \
    && rm -rf /var/lib/apt/lists/*

RUN useradd -m -s /bin/bash nonroot
USER nonroot
WORKDIR /app

COPY --from=builder --chown=nonroot:nonroot /app/build/cipereusz .

CMD ["./cipereusz"]